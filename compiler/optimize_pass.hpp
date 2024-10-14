#ifndef BF_INTERPRETER_OPTIMIZE_PASS_HPP
#define BF_INTERPRETER_OPTIMIZE_PASS_HPP

#include "ir.hpp"

inst_stream pass0(const std::string& original_instruction_stream)
{
    inst_stream output_stream;
    unsigned long program_counter = 0;

    while(program_counter < original_instruction_stream.size())
    {
        char current_inst = original_instruction_stream[program_counter];
        switch (current_inst)
        {
        case '>':
            output_stream.push_back(inter_inst(OP_MV,1));
            break;
        case '<':
            output_stream.push_back(inter_inst(OP_MV,-1));
            break;
        case '+':
            output_stream.push_back(inter_inst(OP_INC,1));
            break;
        case '-':
            output_stream.push_back(inter_inst(OP_INC,-1));
            break;
        case '.':
            output_stream.push_back(inter_inst(OP_WRITE));
            break;
        case ',':
            output_stream.push_back(inter_inst(OP_READ));
            break;
        case '[':
            output_stream.push_back(inter_inst(OP_BRANCH));
            break;
        case ']':
            output_stream.push_back(inter_inst(OP_BACK));
            break;
        default:
            break;
        }
        program_counter++;
    }

    return output_stream;
}

inst_stream pass1(const inst_stream& input_stream)
{
    inst_stream opt_output_stream;
    for(const auto & inst : input_stream)
    {
        if(!opt_output_stream.empty())
        {
            if(inst.op_code == OP_MV)
            {
                if(opt_output_stream.back().op_code == OP_MV)
                {
                    opt_output_stream.back().operand1 += inst.operand1;
                    if(opt_output_stream.back().operand1 == 0)
                        opt_output_stream.pop_back();
                    continue;
                }
            }

            if(inst.op_code == OP_INC)
            {
                if(opt_output_stream.back().op_code == OP_INC)
                {
                    opt_output_stream.back().operand1 += inst.operand1;
                    if(opt_output_stream.back().operand1 == 0)
                        opt_output_stream.pop_back();
                    continue;
                }
            }
        }
        opt_output_stream.emplace_back(inst);
    }
    return opt_output_stream;
}

inst_stream pass2(const inst_stream& input_stream)
{
    inst_stream opt_output_stream;
    for(const auto & inst : input_stream)
    {
        if(opt_output_stream.size() > 2)
        {
            if(inst.op_code == OP_MV)
            {
                auto size = opt_output_stream.size();
                if(opt_output_stream[size - 2].op_code == OP_MV && opt_output_stream[size - 1].op_code == OP_INC)
                {
                    auto m1 = opt_output_stream[size - 2].operand1;
                    auto m2 = opt_output_stream[size - 1].operand1;
                    opt_output_stream.pop_back();
                    opt_output_stream.pop_back();
                    opt_output_stream.emplace_back(OP_INC_OFF,m1,m2);
                    if(m1 + inst.operand1 != 0)
                        opt_output_stream.emplace_back(OP_MV,m1 + inst.operand1);
                    continue;
                }
            }
        }
        opt_output_stream.emplace_back(inst);
    }
    return opt_output_stream;
}

inst_stream pass3(const inst_stream& input_stream)
{
    inst_stream opt_output_stream;
    for(const auto & inst : input_stream)
    {
        if(opt_output_stream.size() >= 2)
        {
            if(inst.op_code == OP_BACK)
            {
                auto size = opt_output_stream.size();
                if(opt_output_stream[size - 2].op_code == OP_BRANCH && opt_output_stream[size - 1].op_code == OP_MV )
                {
                    auto m = opt_output_stream[size - 1].operand1;
                    if(m == 1 || m == 2 || m == -1 || m == -2 )
                    {
                        //printf("trigger %d !\n",m);
                        auto m = opt_output_stream[size - 1].operand1;
                        opt_output_stream.pop_back();
                        opt_output_stream.pop_back();
                        opt_output_stream.emplace_back(OP_MEM_SCAN,m);
                        continue;
                    }
                }
            }
        }
        opt_output_stream.emplace_back(inst);
    }
    return opt_output_stream;
}

inst_stream pass4(const inst_stream& input_stream, bool & fixed_point)
{
    inst_stream opt_output_stream;
    fixed_point = true;
    for(auto i = 0; i < input_stream.size();i++)
    {
        auto & inst = input_stream[i];
        if(inst.op_code == OP_BRANCH)
        {
            bool find_innermost_loop = false;
            auto flag = 0;
            auto j = i + 1;
            for(; j < input_stream.size(); j++)
            {
                const auto & inst2 = input_stream[j];
                if(inst2.op_code == OP_BRANCH)
                {
                    break;
                }else if (inst2.op_code == OP_BACK) {
                    find_innermost_loop = true;
                    break;
                }
            }

            if(find_innermost_loop)
            {
               int delta = 0;
               bool is_simple = true;
               for(int k = i + 1; k < j; k++)
               {    
                    const auto & inst2 = input_stream[k];
                    if(inst2.op_code != OP_INC && inst2.op_code != OP_INC_OFF && inst2.op_code != OP_ST_OFF)
                    {
                        is_simple = false;
                        break;
                    }
                    if(inst2.op_code == OP_INC)
                    {
                        delta += inst2.operand1;
                    }
               }

               if(is_simple && (delta == -1 || delta == 1))
               {
                    fixed_point = false;
                    for(int k = i + 1; k < j; k++)
                    {
                        if(input_stream[k].op_code == OP_INC_OFF)
                        {
                            opt_output_stream.emplace_back(OP_MDA_OFF,input_stream[k].operand1,input_stream[k].operand2,delta);
                        }
                        if(input_stream[k].op_code == OP_ST_OFF)
                        {
                            opt_output_stream.emplace_back(OP_ST_OFF,input_stream[k].operand1,input_stream[k].operand2);
                            //printf("trigger cascading innermost loop\n");
                        }
                    }
                    opt_output_stream.emplace_back(OP_ST,0);
                    i = j; //jump to loop end
                    continue;
               }
            }
        }
        opt_output_stream.emplace_back(inst);
    }
    return opt_output_stream;
}

inst_stream pass5(const inst_stream& input_stream)
{
    // ST merge
    inst_stream opt_output_stream;
    for(const auto & inst : input_stream)
    {
        if(!opt_output_stream.empty())
        {
            if(inst.op_code == OP_INC)
            {
                bool find = false;
                for(auto i = opt_output_stream.size() - 1; i >= 0; i--)
                {
                    if(opt_output_stream[i].op_code != OP_INC_OFF || opt_output_stream[i].op_code != OP_ST_OFF)
                    {
                        if(opt_output_stream[i].op_code == OP_ST)
                        {
                            find = true;
                            opt_output_stream[i].operand1 += inst.operand1;
                            //printf("trigger : ST %d\n",opt_output_stream[i].operand1);
                        }
                        break;
                    }
                }
                if(find)
                    continue;
            }

            if(inst.op_code == OP_ST)
            {
                bool find = false;
                for(auto i = opt_output_stream.size() - 1; i >= 0; i--)
                {
                    if(opt_output_stream[i].op_code != OP_INC_OFF || opt_output_stream[i].op_code != OP_ST_OFF)
                    {
                        if(opt_output_stream[i].op_code == OP_ST || opt_output_stream[i].op_code == OP_INC)
                        {
                            find = true;
                            opt_output_stream[i] = inst;
                            //printf("trigger : ST cover\n");
                        }
                        break;
                    }
                }
                if(find)
                    continue;
            }
        }
        opt_output_stream.emplace_back(inst);
    }
    return opt_output_stream;
}

inst_stream pass6(const inst_stream& input_stream)
{
    inst_stream opt_output_stream;
    for(const auto & inst : input_stream)
    {
        if(opt_output_stream.size() >= 2)
        {
            if(inst.op_code == OP_MV)
            {
                auto size = opt_output_stream.size();
                if(opt_output_stream[size - 2].op_code == OP_MV && opt_output_stream[size - 1].op_code == OP_ST)
                {
                    //printf("trigger ST_OFF\n");
                    auto m1 = opt_output_stream[size - 2].operand1;
                    auto m2 = opt_output_stream[size - 1].operand1;
                    opt_output_stream.pop_back();
                    opt_output_stream.pop_back();
                    opt_output_stream.emplace_back(OP_ST_OFF,m1,m2);
                    if(m1 + inst.operand1 != 0)
                        opt_output_stream.emplace_back(OP_MV,m1 + inst.operand1);
                    continue;
                }
            }
        }
        opt_output_stream.emplace_back(inst);
    }
    return opt_output_stream;
}

#endif

