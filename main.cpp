#include <cstddef>
#include <cstdio>
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

std::vector<unsigned char> tape;

void need_to_access_cell(unsigned long cell)
{
    if (cell >= tape.size())
    {
        tape.resize(cell+1);
    }
}

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

    unsigned long cell_pointer = 0;
    unsigned long program_counter = 0;

    std::vector<loop_profile> simple_innermost_loops;
    std::vector<loop_profile> innermost_loops;
    loop_profile current_loop(-1,-1,0);
    int old_pointer = -1;
    int old_p_0 = -1;
    bool has_io = false;

    while(program_counter < instructions.size())
    {
        char current_inst = instructions[program_counter];
        switch (current_inst)
        {
        case '>':
            cell_pointer++;
            if(enable_profile)
            {
                instruction_count_table[0]++;
            }
            break;
        case '<':
            cell_pointer--;
            if(enable_profile)
            {
                instruction_count_table[1]++;
            }
            break;
        case '+':
            need_to_access_cell(cell_pointer);
            tape[cell_pointer]++;
            if(enable_profile)
            {
                instruction_count_table[2]++;
            }      
            break;
        case '-':
            need_to_access_cell(cell_pointer);
            tape[cell_pointer]--;
            if(enable_profile)
            {
                instruction_count_table[3]++;
            }
            break;
        case '.':
            need_to_access_cell(cell_pointer);
            printf("%c", tape[cell_pointer]); // output the value of the current cell
            if(enable_profile)
            {
                instruction_count_table[4]++;
                has_io = true;
            }  
            break;
        case ',':
            need_to_access_cell(cell_pointer);
            need_to_access_cell(cell_pointer+1);
            tape[cell_pointer] = tape[cell_pointer + 1];
            if(enable_profile)
            {
                instruction_count_table[5]++;
                has_io = true;
            }
            break;
        case '[':
            need_to_access_cell(cell_pointer);
            if (tape[cell_pointer] == 0)
            {
                if(enable_profile)
                {
                    instruction_count_table[6]++;
                }
                bool find_matched = false;
                unsigned int jump_markers = 1;
                for (unsigned long i = program_counter + 1; i < instructions.size(); i++)
                {
                    if (instructions[i] == '[')
                    {
                        jump_markers++;
                    }
                    else if (instructions[i] == ']')
                    {
                        jump_markers--;
                        if (jump_markers == 0)
                        {
                            program_counter = i - 1; // minus 1 because we inc it at the end of loop
                            find_matched = true;
                            break;
                        }
                    }
                }
                if(!find_matched) std::abort(); // [ cannot be matched
            }
            if(enable_profile)
            {
                current_loop.start = program_counter;
                old_pointer = cell_pointer;
                old_p_0 = tape[cell_pointer];
                has_io = false;
            }
            break;
        case ']':
            need_to_access_cell(cell_pointer);
            if (tape[cell_pointer] != 0)
            {
                instruction_count_table[7]++;
                bool find_matched = false;
                unsigned int jump_markers = 1;
                for (unsigned long i = program_counter - 1; i >= 0; i--)
                {
                    if (instructions[i] == ']')
                    {
                        jump_markers++;
                    }
                    else if (instructions[i] == '[')
                    {
                        jump_markers--;
                        if (jump_markers == 0)
                        {
                            if(enable_profile)
                            {
                                if(current_loop.start == i)
                                {
                                    // found innermost loop
                                    if(current_loop.end != program_counter)
                                    {
                                        current_loop.end = program_counter;
                                        current_loop.iter = 0;
                                    }
                                    current_loop.iter ++;

                                    bool is_simple = true;
                                    do {
                                        if(has_io) {
                                            is_simple = false;
                                            break;
                                        }
                                        if(cell_pointer != old_pointer)
                                        {
                                            is_simple = false;
                                            break;
                                        }
                                        auto new_p_0 = tape[cell_pointer];
                                        if((old_p_0 - 1 != new_p_0) && (old_pointer + 1 != new_p_0))
                                            is_simple = false;
                                    }while (0);
                                    current_loop.is_simple = is_simple;
                                }
                            }
                            program_counter = i - 1; // minus 1 because we inc it at the end of loop
                            find_matched = true;
                            break;
                        }
                    }
                }
                if (!find_matched) std::abort(); // ] cannot be matched
            }else{
                if(enable_profile)
                {
                    if(current_loop.end == program_counter)
                    {
                        // found innermost loop
                        if(current_loop.is_simple)
                        {
                            bool exist = false;
                            for(auto & loop : simple_innermost_loops)
                            {
                                if(loop.end == current_loop.end)
                                {
                                    loop.iter = loop.iter + current_loop.iter + 1;
                                    exist = true;
                                    break;
                                }
                            }
                            if(!exist)
                            {
                                simple_innermost_loops.emplace_back(current_loop.start,current_loop.end,current_loop.iter + 1);
                            }
                        }else{
                            bool exist = false;
                            for(auto & loop : innermost_loops)
                            {
                                if(loop.end == current_loop.end)
                                {
                                    loop.iter = loop.iter + current_loop.iter + 1;
                                    exist = true;
                                    break;
                                }
                            }
                            if(!exist)
                            {
                                innermost_loops.emplace_back(current_loop.start,current_loop.end,current_loop.iter + 1);
                            }
                        }
                    }
                }
            }
            break;
        default:
            break;
        }
        program_counter++;
    }
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