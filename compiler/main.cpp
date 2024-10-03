#include <assert.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <algorithm>
/*
 * >  move the pointer right
 * <  move the pointer left
 * +  increment the current cell
 * -  decrement the current cell
 * .  output the value of the current cell
 * ,  replace the value of the current cell with the next byte from input stream
 * [  jump to the matching ] instruction if the current value is zero
 * ]  jump to the matching [ instruction if the current value is not zero
*/ 

#define OP_MV 0
#define OP_INC 1
#define OP_WRITE 2
#define OP_READ 3
#define OP_BRANCH 4
#define OP_BACK 5
#define OP_INC_OFF 6 //tape[op1] += op2
#define OP_MDA_OFF 7//tape[op1] += tape[0] * op2
#define OP_ST 8//tape[0] = op1
#define OP_MEM_SCAN 9

struct inter_inst
{
    char op_code;
    int operand1;
    int operand2;
    inter_inst(int _code) : op_code(_code),operand1(0),operand2(0){};
    inter_inst(int _code, int _operand1) : op_code(_code),operand1(_operand1),operand2(0){};
    inter_inst(int _code, int _operand1, int _operand2) : op_code(_code),operand1(_operand1),operand2(_operand2){};
};

using inst_stream = std::vector<inter_inst>;

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
                    if(inst2.op_code != OP_INC && inst2.op_code != OP_INC_OFF)
                    {
                        is_simple = false;
                        break;
                    }
                    if(inst2.op_code == OP_INC)
                    {
                        delta += inst2.operand1;
                    }
               }

               if(is_simple && (delta == 1 || delta == -1))
               {
                    for(int k = i + 1; k < j; k++)
                    {
                        if(input_stream[k].op_code == OP_INC_OFF)
                        {
                            opt_output_stream.emplace_back(OP_MDA_OFF,input_stream[k].operand1,input_stream[k].operand2);
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

inst_stream pass4(const inst_stream& input_stream)
{
    inst_stream opt_output_stream;
    for(const auto & inst : input_stream)
    {
        if(opt_output_stream.size() > 2)
        {
            if(inst.op_code == OP_BACK)
            {
                auto size = opt_output_stream.size();
                if(opt_output_stream[size - 2].op_code == OP_BRANCH && opt_output_stream[size - 1].op_code == OP_MV )
                {
                    auto m = opt_output_stream[size - 1].operand1;
                    if(m == 1 || m == 2 || m == 4 || m == -1 || m == -2 || m == -4)
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

inst_stream pass5(const inst_stream& input_stream)
{
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
                    if(opt_output_stream[i].op_code != OP_INC_OFF)
                    {
                        if(opt_output_stream[i].op_code == OP_ST)
                        {
                            find = true;
                            opt_output_stream[i].operand1 += inst.operand1;
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

inst_stream preprocess(const std::string& original_instruction_stream)
{
    auto stream = pass0(original_instruction_stream);
    stream = pass1(stream);
    stream = pass2(stream);
    stream = pass3(stream);
    stream = pass4(stream);
    return stream;
}

std::ostringstream compile(const inst_stream& input_stream)
{
    std::ostringstream asm_builder;
    // append assembly header
    asm_builder << "\t.section	__TEXT,__text,regular,pure_instructions\n";
    asm_builder << "\t.globl	_bf_main                        ; -- Begin function bf_main\n";
    asm_builder << "\t.p2align	2\n";
    asm_builder << "_bf_main:                               ; @bf_main\n";
    
    // push char* tape to stack
    asm_builder << " \tsub	sp, sp, #32\n";
    asm_builder << " \tstp	x29, x30, [sp, #16]\n";
    asm_builder << " \tadd	x29, sp, #16\n";
    asm_builder << " \tstr	x0, [sp, #8]\n";

    // initialize cell_pointer register
    asm_builder << "\tldr     x19, [sp, #8]\n";
    // initialize current data register
    asm_builder << "\tldrb    w20, [x19]\n";

    std::vector<unsigned int> loop_exit_label_stack;
    loop_exit_label_stack.push_back(0);

    unsigned int loop_header = 0;
    unsigned int loop_exit = 0;

    unsigned int current_label = 0;

    for(const auto & inst : input_stream)
    {
        switch (inst.op_code)
        {
        case OP_MV:
            assert(inst.operand1 != 0);
            //asm_builder << "\tstrb    w20, [x19]\t\t; MV " << inst.operand1 <<"\n";
            asm_builder << "\tldr    x19, [sp, #8]\t\t; MV " << inst.operand1 <<"\n";
            if(inst.operand1 > 0)
            {
                asm_builder << "\tadd     x19, x19, #" << abs(inst.operand1) << "\n";
            }else{
                asm_builder << "\tsubs     x19, x19, #" << abs(inst.operand1) << "\n";
            }
            asm_builder << "\tstr    x19, [sp, #8]\n";
            break;
        case OP_INC:
            assert(inst.operand1 != 0);
            asm_builder << "\tmov     w10, #" << abs(inst.operand1) << "\t\t; INC " << inst.operand1 <<"\n";
            asm_builder << "\tldr     x19, [sp, #8]\n";
            asm_builder << "\tldrb    w20, [x19]\n";
            if(inst.operand1 > 0)
            {
                asm_builder << "\tadd     w20, w20, w10\n";
            }else{
                asm_builder << "\tsubs     w20, w20, w10\n";
            }
            asm_builder << "\tstrb    w20, [x19]\n";
            asm_builder << "\tstr     x19, [sp, #8]\n";
            break;
        case OP_INC_OFF:
            // TODO: how large is op1 and op2?
            assert(inst.operand1 != 0);
            asm_builder << "\tldr     x19, [sp, #8]\n";
            if(inst.operand1 > 0)
            {
                asm_builder << "\tldr     w8, [x19, #" << inst.operand1 << "]\t\t; INC_OFF " << inst.operand1 << ", " << inst.operand2 <<"\n";
            }else{
                asm_builder << "\tsubs    x9, x19, #" << abs(inst.operand1) << "\t\t; INC_OFF " << inst.operand1 << ", " << inst.operand2 <<"\n";
                asm_builder << "\tldrb    w8, [x9]\n";
            }
            asm_builder << "\tmov    w10, #" << abs(inst.operand2) << "\n";
            assert(inst.operand2 != 0);
            if(inst.operand2 > 0)
            {
                asm_builder << "\tadd     w8, w8, w10\n";
            }else{
                asm_builder << "\tsubs     w8, w8, w10\n";
            }
            if(inst.operand1 > 0)
            {
                asm_builder << "\tstrb    w8, [x19, #" << inst.operand1 << " ]\n";
            }else{
                asm_builder << "\tstrb    w8, [x9]\n";
            }
            break;
        case OP_MDA_OFF:
            asm_builder << "\tldr     x19, [sp, #8]\n";
            asm_builder << "\tldrb    w20, [x19]\n";
            asm_builder << "\tmov     w10, #" << abs(inst.operand2) <<"\t\t; MDA_OFF " << inst.operand1 << ", " << inst.operand2 <<"\n";
            asm_builder << "\tmul     w11, w10, w20\n";
            assert(inst.operand1 != 0);
            if(inst.operand1 > 0)
            {
                asm_builder << "\tldrb    w8, [x19, #" << inst.operand1 << "]\n";
            }else{
                asm_builder << "\tsubs    x10, x19, #" << abs(inst.operand1) << "\n";
                asm_builder << "\tldrb    w8, [x10]\n";
            }
            assert(inst.operand2 != 0);
            if(inst.operand2 > 0)
            {
                asm_builder << "\tadd     w8, w8, w11\n";
            }else{
                asm_builder << "\tsubs    w8, w8, w11\n";
            }
            if(inst.operand1 > 0)
            {
                asm_builder << "\tstrb    w8, [x19, #" << inst.operand1 << "]\n";

            }else{
                asm_builder << "\tstrb    w8, [x10]\n";
            }
            break;
        case OP_ST:
            asm_builder << "\tldr     x19, [sp, #8]\n";
            //asm_builder << "\tldrb    w20, [x19]\n";
            if(inst.operand1 == 0)
            {
                asm_builder << "\tmov     w20, #0\t\t; ST 0\n";
            }else{
                char st = 0;
                st += inst.operand1;
                int c = (unsigned char)st;
                asm_builder << "\tmov     w20, #" << c << "\t\t; ST " << inst.operand1 <<"\n";
            }
            asm_builder << "\tstrb    w20, [x19]\n";
            break;
        case OP_WRITE:
            //asm_builder << "\tstrb w20, [x19]\t\t; WRITE \n";
            asm_builder << "\tldr     x19, [sp, #8]\n";
            asm_builder << "\tldrsb	w0, [x19]\n";
            //asm_builder << "\tstr	x19, [sp, #8]\n"; // save x19
            asm_builder << "\tbl	_putchar\n"; // call c library putchar function
            //asm_builder << "\tldr	x19, [sp, #8]\n"; // load x19
            //asm_builder << "\tldrb	w20, [x19]\n"; // load w20
            break;
        case OP_READ:
            //asm_builder << "\tstr	x19, [sp, #8]\t\t; READ\n"; // save x19
            asm_builder << "\tbl	_getchar\n"; // call c library getchar function
            //asm_builder << "\tldr	x19, [sp, #8]\n"; // load x19
            //asm_builder << "\tmov	w20, w0\n";
            asm_builder << "\tldr     x19, [sp, #8]\n";
            asm_builder << "\tstrb    w0, [x19]\n";
            break;
        case OP_BRANCH:
            loop_header = current_label++;
            loop_exit = current_label++;
            loop_exit_label_stack.push_back(loop_exit);
            asm_builder << "\tb     " << "LBB0_" << loop_header <<"\n";
            asm_builder << "LBB0_" << loop_header << ":\n";
            asm_builder << "\tldr     x19, [sp, #8]\n";
            asm_builder << "\tldrb    w20, [x19]\n";
            asm_builder << "\tcbz     w20, " << "LBB0_" << loop_exit << "\n";
            break;
        case OP_BACK:
            loop_exit = loop_exit_label_stack.back();
            loop_exit_label_stack.pop_back();
            asm_builder << "\tb     " << "LBB0_" << loop_exit - 1 << "\n";
            asm_builder << "LBB0_" << loop_exit << ":\n"; 
            break;
        case OP_MEM_SCAN:
            if(inst.operand1 == 1)
            {
                asm_builder << "\tadrp    x9, _indices@PAGE\n";
                asm_builder << "\tldr     q0, [x9, _indices@PAGEOFF]\n";
            }else if (inst.operand1 == 2) {
                asm_builder << "\tadrp    x9, _indices2@PAGE\n";
                asm_builder << "\tldr     q0, [x9, _indices2@PAGEOFF]\n";
                asm_builder << "\tadrp    x9, _filters2@PAGE\n";
                asm_builder << "\tldr     q3, [x9, _filters2@PAGEOFF]\n";
            }else if (inst.operand1 == 4) {
                asm_builder << "\tadrp    x9, _indices3@PAGE\n";
                asm_builder << "\tldr     q0, [x9, _indices3@PAGEOFF]\n";
                asm_builder << "\tadrp    x9, _filters3@PAGE\n";
                asm_builder << "\tldr     q3, [x9, _filters3@PAGEOFF]\n";
            }else if (inst.operand1 == -1) {
                asm_builder << "\tadrp    x9, _indices_r@PAGE\n";
                asm_builder << "\tldr     q0, [x9, _indices_r@PAGEOFF]\n";
            }else if (inst.operand1 == -2) {
                asm_builder << "\tadrp    x9, _indices2_r@PAGE\n";
                asm_builder << "\tldr     q0, [x9, _indices2_r@PAGEOFF]\n";
                asm_builder << "\tadrp    x9, _filters2_r@PAGE\n";
                asm_builder << "\tldr     q3, [x9, _filters2_r@PAGEOFF]\n";
            }else if (inst.operand1 == -4) {
                asm_builder << "\tadrp    x9, _indices3_r@PAGE\n";
                asm_builder << "\tldr     q0, [x9, _indices3_r@PAGEOFF]\n";
                asm_builder << "\tadrp    x9, _filters3_r@PAGE\n";
                asm_builder << "\tldr     q3, [x9, _filters3_r@PAGEOFF]\n";
            }
            loop_header = current_label++;
            loop_exit = current_label++;
            loop_exit_label_stack.push_back(loop_exit);
            asm_builder << "\tb     " << "LBB0_" << loop_header <<"\n";
            asm_builder << "LBB0_" << loop_header << ":\n";
            asm_builder << "\tldr     x19, [sp, #8]\n";
            asm_builder << "\tldrb    w20, [x19]\n";
            asm_builder << "\tcbz     w20, " << "LBB0_" << loop_exit << "\n";

            asm_builder << "\tldr     x19, [sp, #8]\n";
            if(inst.operand1 > 0)
            {
                asm_builder << "\tldr     q1, [x19]\n";
            }else{
                asm_builder << "\tldr     q1, [x19, #-15]\n";
            }

            if(inst.operand1 == 2 || inst.operand1 == 4 || inst.operand1 == -2 || inst.operand1 == -4)
            {
                asm_builder << "\tand.16b     v1, v3, v1\n";
            }

            if(inst.operand1 == 1 || inst.operand1 == -1)
            {
                asm_builder << "\tcmeq.16b     v1, v1, #0\n";
            }else if(inst.operand1 == 2 || inst.operand1 == -2)
            {
                asm_builder << "\tcmeq.8h     v1, v1, #0\n";
            }else if(inst.operand1 == 4 || inst.operand1 == -4)
            {
                asm_builder << "\tcmeq.4s     v1, v1, #0\n";
            }

            asm_builder << "\tand.16b     v2, v0, v1\n";
            asm_builder << "\torn.16b     v1, v2, v1\n";

            if(inst.operand1 == 1 || inst.operand1 == -1)
            {
                asm_builder << "\tuminv   b8, v1.16b\n";
            }else if(inst.operand1 == 2 || inst.operand1 == -2)
            {
                asm_builder << "\tuminv   h8, v1.8h\n";
            }else if(inst.operand1 == 4 || inst.operand1 == -4)
            {
                asm_builder << "\tuminv   s8, v1.4s\n";
            }   

            asm_builder << "\tfmov    w10, s8\n";
            asm_builder << "\tmov     w9, #16\n";

            if(inst.operand1 == 1 || inst.operand1 == -1)
            {
                asm_builder << "\tcmp     w10, #255\n";
            }else if(inst.operand1 == 2 || inst.operand1 == -2)
            {
                asm_builder << "\tmov     w25, #65535\n";
                asm_builder << "\tcmp     w10, w25\n";
            }else if(inst.operand1 == 4 || inst.operand1 == -4)
            {
                asm_builder << "\tmov     w25, #4294967295\n";
                asm_builder << "\tcmp     w10, w25\n";
            }   

            asm_builder << "\tcsel    w10, w9, w10, eq\n";
            if(inst.operand1 >0)
            {
                asm_builder << "\tadd     x19, x19, x10\n";
            }else{
                asm_builder << "\tsubs     x19, x19, x10\n";
            }
        
            asm_builder << "\tstr     x19, [sp, #8]\n";

            loop_exit = loop_exit_label_stack.back();
            loop_exit_label_stack.pop_back();
            asm_builder << "\tb     " << "LBB0_" << loop_exit - 1 << "\n";
            asm_builder << "LBB0_" << loop_exit << ":\n"; 
            break;
        default:
            break;
        }
    }

    // flush w20 to tape
    asm_builder << "\tstrb    w20, [x19]\t\t; Flush to tape\n";

    asm_builder << "\tldp	x29, x30, [sp, #16]\n";
    asm_builder << "\tadd	sp, sp, #32\n";
    asm_builder << "\tret\n";
    asm_builder << "                                    ; -- End function\n";

    asm_builder << "\t.section	__DATA,__data\n";
    asm_builder << "\t.globl	_indices                        ; @indices\n";
    asm_builder << "_indices:\n";
    asm_builder << "\t.byte 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0\n";

    asm_builder << "\t.globl	_indices_r                      ; @indices_r\n";
    asm_builder << "_indices_r:\n";
    asm_builder << "\t.byte 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0\n";

    asm_builder << "\t.globl	_indices2                       ; @indices2\n";
    asm_builder << "_indices2:\n";
    asm_builder << "\t.byte 0, 0, 2, 0, 4, 0, 6, 0, 8, 0, 10, 0, 12, 0, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0\n";

    asm_builder << "\t.globl	_indices2_r                     ; @indices2_r\n";
    asm_builder << "_indices2_r:\n";
    asm_builder << "\t.byte 14, 0, 12, 0, 10, 0, 8, 0, 6, 0, 4, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0\n";

    asm_builder << "\t.globl	_indices3                       ; @indices3\n";
    asm_builder << "_indices3:\n";
    asm_builder << "\t.byte 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 8, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0\n";  

    asm_builder << "\t.globl	_indices3_r                     ; @indices3_r\n";
    asm_builder << "_indices3_r:\n";
    asm_builder << "\t.byte 0, 0, 0, 12, 0, 0, 0, 8, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0\n";  

    asm_builder << "\t.globl	_filters2                       ; @filters2\n";
    asm_builder << "_filters2:\n";
    asm_builder << "\t.byte 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0\n";

    asm_builder << "\t.globl	_filters2_r                     ; @filters2_r\n";
    asm_builder << "_filters2_r:\n";
    asm_builder << "\t.byte 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0\n";

    asm_builder << "\t.globl	_filters3                       ; @filters3\n";
    asm_builder << "_filters3:\n";
    asm_builder << "\t.byte 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0\n";

    asm_builder << "\t.globl	_filters3_r                     ; @filters3_r\n";
    asm_builder << "_filters3_r:\n";
    asm_builder << "\t.byte 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0\n";

    asm_builder << ".subsections_via_symbols\n\n";
    return asm_builder;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1; 
    }

    std::string filename = argv[1];

    std::ifstream inFile(filename);  

    if (!inFile) {
        std::cerr << "Unable to open file: " << filename << std::endl;
        return 1; 
    }

    std::stringstream buffer;
    buffer << inFile.rdbuf();

    std::string instructions = buffer.str();  // Convert the buffer into a string

    auto inter_inst_stream = preprocess(instructions);

    auto asm_builder = compile(inter_inst_stream);

    //printf("compiled\n");

    // write result to file
    std::ofstream outputFile("output.s");
    if (outputFile.is_open()) {
        outputFile << asm_builder.str();  
        outputFile.close();
    } else {
        std::cerr << "Error creating file." << std::endl;
    }
}