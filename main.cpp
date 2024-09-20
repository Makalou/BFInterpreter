#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
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

#define IS_INST(c) (c == '>' || c == '<' || c == '+' || c == '-' || c == '.' || c == ',' || c == '[' || c == ']')

#define OP_MV 0
#define OP_INC 1
#define OP_WRITE 2
#define OP_READ 3
#define OP_BRANCH 4
#define OP_BACK 5

std::vector<unsigned char> tape;
unsigned long cell_pointer = 0;

void need_to_access_cell(unsigned long cell)
{
    if (cell >= tape.size())
    {
        tape.resize(cell+1);
    }
}

struct instruction
{
    int op_code;
    int operand;
    instruction(int _code, int _operand) : op_code(_code),operand(_operand){};
};

bool enable_profile = false;

unsigned int instruction_count_table[8];

struct loop_profile
{
    unsigned long start;
    unsigned long end;
    unsigned long iter;

    bool is_simple;

    loop_profile(unsigned long _start,
    unsigned long _end,
    unsigned long _iter):start(_start),end(_end),iter(_iter){
        is_simple = true;
    };
};

std::vector<loop_profile> simple_innermost_loops;
std::vector<loop_profile> innermost_loops;

struct block_executer
{
    block_executer(unsigned  long start) : initial_start(start),execute_count(0)
    {
        
    }
    std::vector<instruction> opt_instructions;

    void push_instruction(int op_code,int operand)
    {
        if(!opt_instructions.empty())
        {
            if(op_code == OP_MV)
            {
                if(opt_instructions.back().op_code == OP_MV)
                {
                    opt_instructions.back().operand += operand;
                    return;
                }
            }

            if(op_code == OP_INC)
            {
                if(opt_instructions.back().op_code == OP_INC)
                {
                    opt_instructions.back().operand += operand;
                    return;
                }
            }
        }

        opt_instructions.emplace_back(op_code,operand);
    }

    std::vector<block_executer> children;

    unsigned long initial_start;

    unsigned int execute_count;

    int initial_execute(const std::string& original_instructions)
    {
        assert(execute_count == 0);
        execute_count = 1;
        auto pc = initial_start;
        while (pc < original_instructions.size()) {
            char current_inst = original_instructions[pc];
            switch (current_inst)
            {
            case '>':
                cell_pointer++;
                push_instruction(OP_MV, 1);
                break;
            case '<':
                cell_pointer--;
                push_instruction(OP_MV, -1);
                break;
            case '+':
                need_to_access_cell(cell_pointer);
                tape[cell_pointer]++;
                push_instruction(OP_INC, 1);    
                break;
            case '-':
                need_to_access_cell(cell_pointer);
                tape[cell_pointer]--;
                push_instruction(OP_INC,-1);
                break;
            case '.':
                need_to_access_cell(cell_pointer);
                printf("%c", tape[cell_pointer]); // output the value of the current cell 
                push_instruction(OP_WRITE, 0);
                break;
            case ',':
                need_to_access_cell(cell_pointer);
                need_to_access_cell(cell_pointer+1);
                tape[cell_pointer] = tape[cell_pointer + 1];
                push_instruction(OP_READ, 0);
                break;
            case '[':
                need_to_access_cell(cell_pointer);
                push_instruction(OP_BRANCH, children.size());
                children.emplace_back(pc + 1);
                if(tape[cell_pointer] == 0)
                {
                    // skip child block for now
                    bool find_matched = false;
                    unsigned int jump_markers = 1;
                    for (unsigned long i = pc + 1; i < original_instructions.size(); i++)
                    {
                        if (original_instructions[i] == '[')
                        {
                            jump_markers++;
                        }
                        else if (original_instructions[i] == ']')
                        {
                            jump_markers--;
                            if (jump_markers == 0)
                            {
                                pc = i;
                                find_matched = true;
                                break;
                            }
                        }
                    }
                    if(!find_matched) std::abort(); // [ cannot be matched
                }else{
                    pc = children.back().initial_execute(original_instructions);
                }
                break;
            case ']':
                need_to_access_cell(cell_pointer);
                push_instruction(OP_BACK, 0);
                if (tape[cell_pointer] != 0)
                {
                   // use optimized instructions to execute future loop
                   execute(original_instructions);
                }
                return pc;
            default:
                break;
            }
            pc++;
        }
        return pc;
    }

    void execute(const std::string& original_instructions)
    {
        if(opt_instructions.empty())
        {
            initial_execute(original_instructions);
            return;
        }

        long opt_pc = 0;
        while(opt_pc < opt_instructions.size())
        {
            auto current_inst = opt_instructions[opt_pc];
            switch (current_inst.op_code)
            {
            case OP_MV:
                cell_pointer += current_inst.operand;
                break;
            case OP_INC:
                need_to_access_cell(cell_pointer);
                tape[cell_pointer] += current_inst.operand;
                break;
            case OP_WRITE:
                need_to_access_cell(cell_pointer);
                printf("%c", tape[cell_pointer]); // output the value of the current cell
                break;
            case OP_READ:
                need_to_access_cell(cell_pointer);
                need_to_access_cell(cell_pointer + 1);
                tape[cell_pointer] = tape[cell_pointer + 1];
                break;
            case OP_BRANCH:
                need_to_access_cell(cell_pointer);
                if (tape[cell_pointer] != 0)
                {
                    children[current_inst.operand].execute(original_instructions);
                }
                break;
            case OP_BACK:
                need_to_access_cell(cell_pointer);
                if (tape[cell_pointer] != 0)
                {
                    execute_count ++;
                    opt_pc = -1;
                }else{
                    return;
                }
                break;
            default:
                std::abort(); //unknown instruction
            }
            opt_pc ++;
        }
    }
};

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [-p] <filename>\n";
        return 1; 
    }

    std::string filename;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-p") == 0) {
            enable_profile = true;
        }else {
            // Assume that the last argument is always the filename
            filename = argv[i];
        }
    }

    std::ifstream inFile(filename);  

    if (!inFile) {
        std::cerr << "Unable to open file: " << filename << std::endl;
        return 1; 
    }

    std::stringstream buffer;
    buffer << inFile.rdbuf();

    std::string instructions = buffer.str();  // Convert the buffer into a string

    //execute_original_block( 0, instructions);
    block_executer main_block(0);
    main_block.execute(instructions);
    
    if(enable_profile)
    {
        printf("\n");
        printf("Instructions Count:\n");
        printf(" < :\t%d\n",instruction_count_table[0]);
        printf(" > :\t%d\n",instruction_count_table[1]);
        printf(" + :\t%d\n",instruction_count_table[2]);
        printf(" - :\t%d\n",instruction_count_table[3]);
        printf(" . :\t%d\n",instruction_count_table[4]);
        printf(" , :\t%d\n",instruction_count_table[5]);
        printf(" [ :\t%d\n",instruction_count_table[6]);
        printf(" ] :\t%d\n",instruction_count_table[7]);
        printf("\n");
        std::sort(simple_innermost_loops.begin(),simple_innermost_loops.end(),[](const auto & a, const auto & b){ return a.iter > b.iter;});
        std::sort(innermost_loops.begin(),innermost_loops.end(),[](const auto & a, const auto & b){ return a.iter > b.iter;});
        printf("Simple Innermost Loops:\n");
        for(const auto & loop : simple_innermost_loops)
        {
            for(auto i = loop.start; i <= loop.end; i ++)
            {   
                if(IS_INST(instructions[i]))
                    printf("%c",instructions[i]);
            }
            printf("\t%lu\n",loop.iter);
        }
        printf("\n");
        printf("Other Innermost Loops:\n");
        for(const auto & loop : innermost_loops)
        {
            for(auto i = loop.start; i <= loop.end; i ++)
            {
                if(IS_INST(instructions[i]))
                    printf("%c",instructions[i]);
            }
            printf("\t%lu\n",loop.iter);
        }
    }
}