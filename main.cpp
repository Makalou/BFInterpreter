#include <cstdio>
#include <vector>
#include <string>

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
        tape.resize(2 * cell + 1);
    }
}

int main(int argc, char* argv[])
{
    unsigned long cell_pointer = 0;

    std::string instructions = "++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++.";

    unsigned long program_counter = 0;

    while(program_counter < instructions.size())
    {
        char current_inst = instructions[program_counter];
        switch (current_inst)
        {
        case '>':
            cell_pointer++;
            break;
        case '<':
            cell_pointer--;
            break;
        case '+':
            need_to_access_cell(cell_pointer);
            tape[cell_pointer]++;
            break;
        case '-':
            need_to_access_cell(cell_pointer);
            tape[cell_pointer]--;
            break;
        case '.':
            need_to_access_cell(cell_pointer);
            printf("%c", tape[cell_pointer]); // output the value of the current cell
            break;
        case ',':
            need_to_access_cell(cell_pointer);
            need_to_access_cell(cell_pointer+1);
            tape[cell_pointer] = tape[cell_pointer + 1];
            break;
        case '[':
            if (tape[cell_pointer] == 0)
            {
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
            if (tape[cell_pointer] != 0)
            {
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
}