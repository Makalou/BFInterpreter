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

    loop_profile(unsigned long _start,
    unsigned long _end,
    unsigned long _iter):start(_start),end(_end),iter(_iter){};
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
    unsigned long is_simple_loop = false;
    unsigned long p_change = 0;

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
                is_simple_loop = false;
            }
            break;
        case '<':
            cell_pointer--;
            if(enable_profile)
            {
                instruction_count_table[1]++;
                is_simple_loop = false;
            }
            break;
        case '+':
            need_to_access_cell(cell_pointer);
            tape[cell_pointer]++;
            if(enable_profile)
            {
                instruction_count_table[2]++;
                p_change++;
            }      
            break;
        case '-':
            need_to_access_cell(cell_pointer);
            tape[cell_pointer]--;
            if(enable_profile)
            {
                instruction_count_table[3]++;
                p_change--;
            }
            break;
        case '.':
            need_to_access_cell(cell_pointer);
            printf("%c", tape[cell_pointer]); // output the value of the current cell
            if(enable_profile)
            {
                instruction_count_table[4]++;
                is_simple_loop = false;
            }  
            break;
        case ',':
            need_to_access_cell(cell_pointer);
            need_to_access_cell(cell_pointer+1);
            tape[cell_pointer] = tape[cell_pointer + 1];
            if(enable_profile)
            {
                instruction_count_table[5]++;
                is_simple_loop = false;
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
                                if(current_loop.end == program_counter)
                                {
                                    current_loop.iter ++;
                                }else{
                                    current_loop.start = i;
                                    current_loop.end = program_counter;
                                    current_loop.iter = 1;
                                }
                                //reset loop state
                                is_simple_loop = true;
                                p_change = 0;
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
                        if(is_simple_loop && (p_change == 1 || p_change == -1))
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

                    //reset loop state
                    is_simple_loop = true;
                    p_change = 0;
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
        std::sort(simple_innermost_loops.begin(),simple_innermost_loops.end(),[](const auto & a, const auto & b){ return a.iter < b.iter;});
        std::sort(innermost_loops.begin(),innermost_loops.end(),[](const auto & a, const auto & b){ return a.iter < b.iter;});
        printf("Simple Innermost Loops:\n");
        printf("From\tTo\tIterate\n");
        for(const auto & loop : simple_innermost_loops)
        {
            printf("%lu\t%lu\t%lu\n",loop.start,loop.end,loop.iter);
        }
        printf("\n");
        printf("Other Innermost Loops:\n");
        printf("From\tTo\tIterate\n");
        for(const auto & loop : innermost_loops)
        {
            printf("%lu\t%lu\t%lu\n",loop.start,loop.end,loop.iter);
        }
    }
}