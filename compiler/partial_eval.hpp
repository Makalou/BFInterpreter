#ifndef BF_INTERPRETER_PARTIAL_EVAL_HPP
#define BF_INTERPRETER_PARTIAL_EVAL_HPP

#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "ir.hpp"

struct cell_state
{
    char val;
    bool tainted;

    cell_state(){
        val = 0;
        tainted = false;
    }

    cell_state(char v, bool t) : val(v), tainted(t){}
};

struct partial_eval_state
{
    std::unordered_map<long,cell_state> cell_states;
    long pointer = 0;
    inst_stream instructions;
};

/*
 * return true if the loop can be partially evaluated.
 * We use the most conservative assumption here : a loop can be successfully partial evaluated 
 * when and only when all its nested loops can be partially evaluated.
 */
bool partial_eval_loop(const inst_stream &input_stream, int start, int n, int *end,partial_eval_state & current_state)
{
    for(int i = start; i < n; i++)
    {
        //printf("%d\n",i);
        const auto & inst = input_stream[i];
        switch (inst.op_code)
        {
        case OP_MV:
            {
                current_state.pointer += inst.operand1;
            }
            break;
        case OP_INC:
            {
                const auto & cell = current_state.cell_states.find(current_state.pointer);
                if(cell != current_state.cell_states.cend())
                {
                    if(!cell->second.tainted)
                    {
                        cell->second.val += inst.operand1;
                    }else{
                        current_state.instructions.emplace_back(OP_INC_ADDR,current_state.pointer,inst.operand1);
                    }
                }else{
                    cell_state c(0, false);
                    c.val += inst.operand1;
                    current_state.cell_states.emplace(current_state.pointer,c);
                }
            }
            break;
        case OP_INC_OFF:
            {
                const auto & cell = current_state.cell_states.find(current_state.pointer + inst.operand1);
                if(cell != current_state.cell_states.cend())
                {
                    if(!cell->second.tainted)
                    {
                        cell->second.val += inst.operand2;
                    }else{
                        current_state.instructions.emplace_back(OP_INC_ADDR,current_state.pointer + inst.operand1,inst.operand2);
                    }
                }else{
                    current_state.cell_states.emplace(current_state.pointer + inst.operand1,cell_state(inst.operand2,false));
                }
            }
            break;
        case OP_MDA_OFF:
            {
                const auto & cell = current_state.cell_states.find(current_state.pointer + inst.operand1);
                const auto & cell2 = current_state.cell_states.find(current_state.pointer);
                if(cell != current_state.cell_states.cend())
                {
                    if(cell2 != current_state.cell_states.cend())
                    {
                        if(cell->second.tainted || cell2->second.tainted)
                        {
                            if(!cell->second.tainted)
                            {
                                // tape[op1] is untainted, we need to flush the current value of tape[op1]
                                if(cell->second.val != 0)
                                {
                                    current_state.instructions.emplace_back(OP_ST_ADDR,current_state.pointer + inst.operand1,cell->second.val);
                                }
                                cell->second.tainted = true;
                            }
                            if(cell2->second.tainted)
                            {
                                // we don't know what is p[0]
                                current_state.instructions.emplace_back(OP_MDA_ADDR,current_state.pointer + inst.operand1,current_state.pointer,inst.operand2);
                            }else{
                                // we know what is p[0]
                                current_state.instructions.emplace_back(OP_INC_ADDR,current_state.pointer + inst.operand1,inst.operand2 * cell2->second.val);
                            }
                        }else{
                            // We know both p[op1] and p[0]
                            cell->second.val += inst.operand2 * cell2->second.val;
                        }
                    }else {
                        // p[0] = 0, no effect
                    }
                }else{
                    if(cell2 != current_state.cell_states.cend())
                    {
                        if(!cell2->second.tainted)
                        {
                            // p[op1] = p[0] * op2
                            current_state.cell_states.emplace(current_state.pointer + inst.operand1,cell_state(inst.operand2 * cell2->second.val,
                                                                                                               false));
                        }else{
                            // we don't know what is p[0]
                            current_state.cell_states.emplace(current_state.pointer + inst.operand1,cell_state(0,true));
                            current_state.instructions.emplace_back(OP_MDA_ADDR,current_state.pointer + inst.operand1,current_state.pointer,inst.operand2);
                        }
                    }else{
                        // p[0] = 0, no effect
                    }
                }
            }
            break;
        case OP_ST:
            {
                const auto & cell = current_state.cell_states.find(current_state.pointer);
                if(cell != current_state.cell_states.cend())
                {
                    cell->second.val = inst.operand1;
                    cell->second.tainted = false;
                }else{
                    current_state.cell_states.emplace(current_state.pointer,cell_state(inst.operand1,false));
                }
            }
            break;
        case OP_WRITE:
            {
                const auto & cell = current_state.cell_states.find(current_state.pointer);
                if(cell != current_state.cell_states.cend())
                {
                    if(!cell->second.tainted)
                    {
                        current_state.instructions.emplace_back(OP_WRITE_IMM,cell->second.val);
                        //putchar(cell->second.val);
                    }else{
                        current_state.instructions.emplace_back(OP_WRITE_ADDR,current_state.pointer);
                    }
                }else{
                    current_state.instructions.emplace_back(OP_WRITE_IMM,0);
                }
            }
            break;
        case OP_READ:
            {
                const auto & cell = current_state.cell_states.find(current_state.pointer);
                if(cell != current_state.cell_states.cend())
                {
                    cell->second.tainted = true;
                }else{
                    current_state.cell_states.emplace(current_state.pointer,cell_state(0,true));
                }
                current_state.instructions.emplace_back(OP_READ_ADDR,current_state.pointer);
            }
            break;
        case OP_BRANCH:
            {
                const auto & cell = current_state.cell_states.find(current_state.pointer);
                if(cell != current_state.cell_states.cend())
                {
                    if(cell->second.tainted)
                    {
                        // If we don't know what is p[0], just terminate partial evaluation
                        *end = i;
                        return false;
                    }else{
                        if(cell->second.val != 0)
                        {
                            partial_eval_state copy_state;
                            copy_state.pointer = current_state.pointer;
                            copy_state.cell_states = current_state.cell_states;
                            if(partial_eval_loop(input_stream,i + 1,n,end,copy_state))
                            {
                                // If success, we accept the partial evaluation result
                                i = *end;
                                current_state.pointer = copy_state.pointer;
                                current_state.cell_states = std::move(copy_state.cell_states);
                                current_state.instructions.insert(current_state.instructions.end(),copy_state.instructions.begin(),copy_state.instructions.end());
                            }else{
                                *end = i;
                                return false;
                            }
                        }else{
                            //skip loop body
                            int mark = 1;
                            int j = i + 1;
                            for(; j < n; j++)
                            {
                                if(input_stream[j].op_code == OP_BRANCH)
                                {
                                    mark ++;
                                }

                                if(input_stream[j].op_code == OP_BACK)
                                {
                                    mark --;
                                    if(mark == 0)
                                        break;
                                }
                            }
                            i = j;
                        }
                    }
                }else{
                    //skip loop body
                    int mark = 1;
                    int j = i + 1;
                    for(; j < n; j++)
                    {
                        if(input_stream[j].op_code == OP_BRANCH)
                        {
                            mark ++;
                        }

                        if(input_stream[j].op_code == OP_BACK)
                        {
                            mark --;
                            if(mark == 0)
                                break;
                        }
                    }
                    i = j;
                }
            }
            break;
        case OP_BACK:
            {
                const auto & cell = current_state.cell_states.find(current_state.pointer);
                if(cell != current_state.cell_states.cend())
                {
                    if(cell->second.tainted)
                    {
                        // If we don't know what is p[0], just terminate partial evaluation
                        *end = i;
                        return false;
                    }else{
                        if(cell->second.val == 0)
                        {
                            *end = i;
                            return true;
                        }else{
                            i = start - 1;
                        }
                    }
                }else{
                    *end = i;
                    return true;
                }
            }
            break;
        case OP_MEM_SCAN:
            {
                auto pointer = current_state.pointer;
                while (true)
                {
                    const auto & cell = current_state.cell_states.find(pointer);
                    if(cell != current_state.cell_states.cend())
                    {
                        if(cell->second.tainted)
                        {
                            return false;
                        }else{
                            if(cell->second.val == 0)
                            {
                                current_state.pointer = pointer;
                                break;
                            }
                        }
                    }else{
                        // find 0
                        current_state.pointer = pointer;
                        break;
                    }
                    pointer += inst.operand1;
                }
            }
            break;
        default:
            break;
        }
    }
    *end = n;
    return true;
}

inst_stream partial_eval(const inst_stream& input_stream)
{
    inst_stream opt_stream;
    partial_eval_state eval_state;
    //partial_eval_state delta_state;
    eval_state.pointer = 0;
    int end = 0;
    partial_eval_loop(input_stream, 0, input_stream.size(), &end,eval_state);

    opt_stream = eval_state.instructions;

    if(end < input_stream.size() - 1)
    {

        for(const auto & cell : eval_state.cell_states)
        {
            if(!cell.second.tainted && cell.second.val!=0)
            {
                // flush all the cells
                opt_stream.emplace_back(OP_ST_ADDR,cell.first,cell.second.val);
            }
        }

        opt_stream.emplace_back(OP_ST_P,eval_state.pointer);

        for(int i = end; i < input_stream.size(); i ++)
        {
            opt_stream.push_back(input_stream[i]);
        }
    }

    return opt_stream;
}

#endif