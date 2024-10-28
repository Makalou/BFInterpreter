#ifndef BF_INTERPRETER_IR_HPP
#define BF_INTERPRETER_IR_HPP

#include <vector>

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
#define OP_ST_OFF 10 // tape[op1] = op2
#define OP_ST_P 11
#define OP_INC_ADDR 12
#define OP_WRITE_ADDR 13
#define OP_WRITE_IMM 14
#define OP_READ_ADDR 15
#define OP_ST_ADDR 16
#define OP_MDA_ADDR 17 //tape[op1] += tape[op2] * op3

struct inter_inst
{
    char op_code;
    int operand1;
    int operand2;
    int operand3;
    inter_inst(int _code) : op_code(_code),operand1(0),operand2(0){};
    inter_inst(int _code, int _operand1) : op_code(_code),operand1(_operand1),operand2(0){};
    inter_inst(int _code, int _operand1, int _operand2) : op_code(_code),operand1(_operand1),operand2(_operand2){};
    inter_inst(int _code, int _operand1, int _operand2, int _operand3) : op_code(_code),operand1(_operand1),operand2(_operand2),operand3(_operand3){};
};

using inst_stream = std::vector<inter_inst>;

#endif