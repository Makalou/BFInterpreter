#include <cstddef>
#include <cstdio>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

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

unsigned int instruction_count_table[8];

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
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

    unsigned long cell_pointer = 0;
    unsigned long program_counter = 0;

    while(program_counter < instructions.size())
    {
        char current_inst = instructions[program_counter];
        switch (current_inst)
        {
        case '>':
            instruction_count_table[0]++;
            cell_pointer++;
            break;
        case '<':
            instruction_count_table[1]++;
            cell_pointer--;
            break;
        case '+':
            need_to_access_cell(cell_pointer);
            instruction_count_table[2]++;
            tape[cell_pointer]++;
            break;
        case '-':
            need_to_access_cell(cell_pointer);
            instruction_count_table[3]++;
            tape[cell_pointer]--;
            break;
        case '.':
            need_to_access_cell(cell_pointer);
            instruction_count_table[4]++;
            printf("%c", tape[cell_pointer]); // output the value of the current cell
            break;
        case ',':
            need_to_access_cell(cell_pointer);
            need_to_access_cell(cell_pointer+1);
            instruction_count_table[5]++;
            tape[cell_pointer] = tape[cell_pointer + 1];
            break;
        case '[':
            need_to_access_cell(cell_pointer);
            if (tape[cell_pointer] == 0)
            {
                instruction_count_table[6]++;
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
                            program_counter = i - 1; // minus 1 because we inc it at the end of loop
                            find_matched = true;
                            break;
                        }
                    }
                }
                if (!find_matched) std::abort(); // ] cannot be matched
            }
            break;
        default:
            break;
        }
        program_counter++;
    }
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
}