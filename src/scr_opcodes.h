/*
**	Заголовочный файл.
**	Команды виртуальной машины.
*/

#pragma once
#ifndef SCR_OPCODES_H
#define SCR_OPCODES_H

const unsigned char I_OP_NOP =        0x00;
const unsigned char I_PUSH =          0x01;
const unsigned char I_POP =           0x02;
const unsigned char I_CALL =          0x03;
const unsigned char I_CALL_B =        0x04;
const unsigned char I_OP_RETURN =     0x05;
const unsigned char I_OP_IRETURN =    0x06;
const unsigned char I_OP_FRETURN =    0x07;
const unsigned char I_OP_IADD =       0x08;
const unsigned char I_OP_ISUB =       0x09;
const unsigned char I_OP_IMUL =       0x0a;
const unsigned char I_OP_IDIV =       0x0b;
const unsigned char I_OP_IREM =       0x0c;
const unsigned char I_OP_ISHL =       0x0d;
const unsigned char I_OP_ISHR =       0x0e;
const unsigned char I_OP_IAND =       0x0f;
const unsigned char I_OP_IOR = 	      0x10;
const unsigned char I_OP_IXOR =       0x11;
const unsigned char I_OP_FADD =       0x12;
const unsigned char I_OP_FSUB =       0x13;
const unsigned char I_OP_FMUL =       0x14;
const unsigned char I_OP_FDIV =       0x15;
const unsigned char I_OP_FREM =       0x16;
const unsigned char I_OP_LESS =       0x17;
const unsigned char I_OP_LESSEQ =     0x18;
const unsigned char I_OP_EQ =         0x19;
const unsigned char I_OP_NOTEQ =      0x1a;
const unsigned char I_OP_GR =         0x1b;
const unsigned char I_OP_GREQ =       0x1c;
const unsigned char I_OP_INOT =       0x1d;
const unsigned char I_OP_IUNEG =      0x1e;
const unsigned char I_OP_IUPLUS =     0x1f;
const unsigned char I_OP_FUNEG =      0x20;
const unsigned char I_OP_FUPLUS =     0x21;
const unsigned char I_OP_ILOAD =      0x22;
const unsigned char I_OP_ISTORE =     0x23;
const unsigned char I_OP_LOADICONST = 0x24;
const unsigned char I_OP_LOADFCONST = 0x25;
const unsigned char I_OP_JMP =        0x26;
const unsigned char I_OP_JMPZ =       0x27;
const unsigned char I_OP_JMPNZ =      0x28;
const unsigned char I_OP_ITOF =       0x29;
const unsigned char I_OP_FTOI =       0x2a;
const unsigned char I_OP_GLOAD =      0x2b;
const unsigned char I_OP_GSTORE =     0x2c;
const unsigned char I_FUNC_PT =       0x2d;
const unsigned char I_OP_NEWARRAY =   0x2e;
const unsigned char I_OP_IALOAD =     0x2f;
const unsigned char I_OP_IASTORE =    0x30;
const unsigned char I_OP_BALOAD =     0x31;
const unsigned char I_OP_BASTORE =    0x32;
const unsigned char I_OP_LOAD0 =      0x33;

#endif
