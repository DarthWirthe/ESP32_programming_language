/*
**	Заголовочный файл.
**	Команды виртуальной машины.
*/

#pragma once
#ifndef SCR_OPCODES_H
#define SCR_OPCODES_H

#define I_OP_NOP         0x00
#define I_PUSH           0x01
#define I_POP            0x02
#define I_CALL           0x03
#define I_CALL_B         0x04
#define I_OP_RETURN      0x05
#define I_OP_IRETURN     0x06
#define I_OP_FRETURN     0x07
#define I_OP_IADD        0x08
#define I_OP_ISUB        0x09
#define I_OP_IMUL        0x0a
#define I_OP_IDIV        0x0b
#define I_OP_IREM        0x0c
#define I_OP_ISHL        0x0d
#define I_OP_ISHR        0x0e
#define I_OP_IAND        0x0f
#define I_OP_IOR  	     0x10
#define I_OP_IBAND       0x11
#define I_OP_IBOR  	     0x12
#define I_OP_IXOR        0x13
#define I_OP_FADD        0x14
#define I_OP_FSUB        0x15
#define I_OP_FMUL        0x16
#define I_OP_FDIV        0x17
#define I_OP_FREM        0x18
#define I_OP_LESS        0x19
#define I_OP_LESSEQ      0x1a
#define I_OP_EQ          0x1b
#define I_OP_NOTEQ       0x1c
#define I_OP_GR          0x1d
#define I_OP_GREQ        0x1e
#define I_OP_IBNOT       0x1f
#define I_OP_INOT        0x20
#define I_OP_IUNEG       0x21
#define I_OP_IUPLUS      0x22
#define I_OP_FUNEG       0x23
#define I_OP_FUPLUS      0x24
#define I_OP_ILOAD       0x25
#define I_OP_ISTORE      0x26
#define I_OP_FLOAD       0x27
#define I_OP_FSTORE      0x28
#define I_OP_LOADICONST  0x29
#define I_OP_LOADFCONST  0x2a
#define I_OP_LOADSTRING  0x2b
#define I_OP_JMP         0x2c
#define I_OP_JMPZ        0x2d
#define I_OP_JMPNZ       0x2e
#define I_OP_ITOF        0x2f
#define I_OP_FTOI        0x30
#define I_OP_GLOAD       0x31
#define I_OP_GSTORE      0x32
#define I_FUNC_PT        0x33
#define I_OP_LOAD0       0x34
#define I_OP_ILOAD1      0x35
#define I_OP_ALLOC       0x36
#define I_OP_REALLOC     0x37
#define I_OP_FREE        0x38
#define I_OP_IALOAD      0x39
#define I_OP_IASTORE     0x3a
#define I_OP_BALOAD      0x3b
#define I_OP_BASTORE     0x3c

#endif
