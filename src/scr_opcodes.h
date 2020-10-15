#include <stdint.h>

#pragma once
#ifndef SCR_OPCODES_H
#define SCR_OPCODES_H

const uint8_t I_OP_NOP = 	    0x00;
const uint8_t I_PUSH =	        0x01;
const uint8_t I_POP =           0x02;
const uint8_t I_CALL =          0x03;
const uint8_t I_CALL_B =        0x04;
const uint8_t I_OP_RETURN =     0x05;
const uint8_t I_OP_IRETURN =    0x06;
const uint8_t I_OP_FRETURN =    0x07;
const uint8_t I_OP_IADD =       0x08;
const uint8_t I_OP_ISUB =       0x09;
const uint8_t I_OP_IMUL =       0x0a;
const uint8_t I_OP_IDIV =       0x0b;
const uint8_t I_OP_IREM =       0x0c;
const uint8_t I_OP_ISHL =       0x0d;
const uint8_t I_OP_ISHR =       0x0e;
const uint8_t I_OP_IAND =       0x0f;
const uint8_t I_OP_IOR = 	    0x10;
const uint8_t I_OP_IXOR =       0x11;
const uint8_t I_OP_FADD =       0x12;
const uint8_t I_OP_FSUB =       0x13;
const uint8_t I_OP_FMUL =       0x14;
const uint8_t I_OP_FDIV =       0x15;
const uint8_t I_OP_FREM =       0x16;
const uint8_t I_OP_LESS =       0x17;
const uint8_t I_OP_LESSEQ =     0x18;
const uint8_t I_OP_EQ =         0x19;
const uint8_t I_OP_NOTEQ =      0x1a;
const uint8_t I_OP_GR =         0x1b;
const uint8_t I_OP_GREQ =       0x1c;
const uint8_t I_OP_INOT =       0x1d;
const uint8_t I_OP_IUNEG =      0x1e;
const uint8_t I_OP_IUPLUS =     0x1f;
const uint8_t I_OP_FUNEG =      0x20;
const uint8_t I_OP_FUPLUS =     0x21;
const uint8_t I_OP_SET =        0x22;
const uint8_t I_OP_LOAD =       0x23;
const uint8_t I_OP_LOADLOCAL =  0x24;
const uint8_t I_OP_STORE =      0x25;
const uint8_t I_OP_STORELOCAL = 0x26;
const uint8_t I_OP_LOADICONST = 0x27;
const uint8_t I_OP_LOADFCONST = 0x28;
const uint8_t I_OP_JMP =        0x29;
const uint8_t I_OP_JMPZ =       0x2a;
const uint8_t I_OP_JMPNZ =      0x2b;
const uint8_t I_FUNC_PT =       0x2c;
const uint8_t I_OP_NEWARRAY =   0x2d;
const uint8_t I_OP_IALOAD =     0x2e;
const uint8_t I_OP_IASTORE =    0x2f;
const uint8_t I_OP_BALOAD =     0x30;
const uint8_t I_OP_BASTORE =    0x31;


#endif