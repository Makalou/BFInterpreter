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

#include "optimize_pass.hpp"
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

void print_instructions(const inst_stream& input_stream)
{
    std::string prefix = "";
    for(const auto & inst : input_stream)
    {
        switch (inst.op_code) {
            case OP_MV :
            printf("%sMV %d\n",prefix.c_str(),inst.operand1);
            break;
            case OP_INC :
            printf("%sINC %d\n",prefix.c_str(),inst.operand1);
            break;
            case OP_WRITE :
            printf("%sWRITE\n",prefix.c_str());
            break;
            case OP_READ :
            printf("%sREAD\n",prefix.c_str());
            break;
            case OP_BRANCH :
            printf("%s[\n",prefix.c_str());
            prefix+="  ";
            break;
            case OP_BACK :
            prefix.pop_back();
            prefix.pop_back();
            printf("%s]\n",prefix.c_str());
            break;
            case OP_INC_OFF : //tape[op1] += op2
            printf("%sINC_OFF %d, %d\n",prefix.c_str(),inst.operand1, inst.operand2);
            break;
            case OP_MDA_OFF ://tape[op1] += tape[0] * op2
            printf("%sMDA_OFF %d, %d\n",prefix.c_str(),inst.operand1, inst.operand2);
            break;
            case OP_ST ://tape[0] = op1
            printf("%sST %d\n",prefix.c_str(),inst.operand1);
            break;
            case OP_MEM_SCAN :
            printf("%sMEM_SCAN %d\n",prefix.c_str(),inst.operand1);
            break;
            case OP_ST_OFF : // tape[op1] = op2
            printf("%sST_OFF %d, %d\n",prefix.c_str(),inst.operand1, inst.operand2);
            break;
        }
    }
}

inst_stream preprocess(const std::string& original_instruction_stream)
{
    auto stream = pass0(original_instruction_stream);
    stream = pass1(stream);
    stream = pass2(stream);
    stream = pass3(stream);
    bool fix_point = false;
    stream = pass4(stream,fix_point);
    stream = pass5(stream);
    stream = pass6(stream);
    bool enable_fix_point = false;
    while (enable_fix_point && !fix_point) {
        stream = pass4(stream,fix_point);
        stream = pass5(stream);
        stream = pass6(stream);
    }
    //print_instructions(stream);
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
            break;
        case OP_INC_OFF:
            // TODO: how large is op1 and op2?
            assert(inst.operand1 != 0);
            asm_builder << "\tldr     x19, [sp, #8]\n";
            if(inst.operand1 > 0)
            {
                asm_builder << "\tldrb     w8, [x19, #" << inst.operand1 << "]\t\t; INC_OFF " << inst.operand1 << ", " << inst.operand2 <<"\n";
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
            if(inst.operand3 == 1)
            {
                asm_builder << "\tmov     w8, #256\n";
                asm_builder << "\tsubs    w20, w8, w20\n";
            }
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
        case OP_ST_OFF:
            asm_builder << "\tldr     x19, [sp, #8]\t\t; ST_OFF "<< inst.operand1 <<"," <<inst.operand2 << "\n";
            if(inst.operand2 == 0)
            {
                asm_builder << "\tmov     w20, #0\t\t; ST 0\n";
            }else{
                char st = 0;
                st += inst.operand2;
                int c = (unsigned char)st;
                asm_builder << "\tmov     w20, #" << c << "\t\t; ST " << inst.operand2 <<"\n";
            }
            asm_builder << "\tstrb    w20, [x19, #" << inst.operand1 <<" ]\n";
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