
/*
	Компилятор, заголовочный файл.
*/

#pragma once
#ifndef SCR_CODE_H
#define SCR_CODE_H

#include "scr_opcodes.h"
#include "scr_tref.h"
#include "scr_types.h"
//#include "utils/scr_dbg.h"

// ######
enum class OpType {
	Nop,
	UnPlus,
	UnMinus,
	UnNot,
	BinAdd,
	BinSub,
	BinMul,
	BinDiv,
	BinMod,
	BinLess,
	BinLessOrEqual,
	BinEqual,
	BinNotEqual,
	BinGreater,
	BinGreaterOrEqual,
	BinAnd,
	BinOr
};

enum class NodeType {
	None,
	ConstInt,
	ConstFloat,
	Var,
	Function,
	Array,
	Operator,
	LeftParen,
	LeftSquare,
	Delimeter
};

typedef union {
	int i;
	float f;
	char *c;
	OpType op;
} node_value;

struct node {
	uint8_t type;
	node_value value;
};

struct ExprBuffer {
	node buffer[128];
	int count;
	ExprBuffer() {
		count = 0;
	}
};
// ######

enum class var_type {
	None,
	Int,
	Float,
	Function,
	ConstInt,
	ConstFloat
};

struct var_name {
	~var_name(void);
	char *name; // имя переменной
	uint8_t type; // тип переменной
	uint8_t args; // аргументы / константа
	uint8_t id; // номер ид
};

struct func_descr { // можно удалить
	func_descr(void);
	func_descr(uint8_t *start, uint8_t *end);
	uint8_t *start_pt;
	uint8_t *end_pt;
};

struct scope_info {
	//uint8_t *code_base; // указатель на начало
	uint8_t locals_count; // количество переменных в окружении
	uint8_t locals_start; // количество переменных, которое было до этого
	uint8_t id; // ид (уровень вложенности)
	uint8_t max_stack; // расширение кадра стека
	scope_info *parent; // указатель на предыдущее звено
	scope_info():
	locals_count(0),
	parent(nullptr)
	{}
};

class Uint8buffer {
	public:
		Uint8buffer(void);
		~Uint8buffer(void);
		Uint8buffer(int len);
		void Add08(uint8_t n);
		void Add16(uint16_t n);
		void Set08(uint8_t* pt, uint8_t n);
		uint8_t Get08(uint8_t* pt);
		
		uint8_t *data;
		uint8_t *pointer; // указатель на последний объект
		int length;
};

class Code {
	public:
		void Init(void);
		void Add(uint8_t i);
		void Add_08(uint8_t n);
		void NewScope(void);
		void CloseScope(void);
		int Add_const_int(int n);
		int Add_const_float(float n);
		int AddLocal(char *name, var_type type);
		void SetArgs(char *name, uint8_t args);
		uint8_t GetArgs(char *name);
		int FindLocal(char *name);
		void CodeOperator(OpType t, bool IsFloat);
		int FindConst(int n);
		void CodeConstInt(int n);
		void CodeConstFloat(float n);
		void CodeDeclareConstVarInt(char *name, int n, var_type type);
		void CodeJump(int shift, bool condition, bool expected);
		void CodeExpression(ExprBuffer *expr);
		uint8_t* CodeMainScope(void);
		void CodeMainScopeEnd(uint8_t *pt);
		void CodeLocalLoad(char *name);
		void CodeLocalStore(char *name);
		int CodeLocalDeclare(char *name, var_type type);
		uint8_t* CodeStartFunction(char *name);
		void CodeCloseFunction(uint8_t *func_pt);
		void CodeCallFunction(char *name, int args);
		void CodeReturnOperator(var_type type);
		void CodeArrayDeclaration(char *name);
		void CodeArrayLoad(char *name);
		void CodeArrayStore(char *name);
		uint8_t* CodeStartIfStatement(void);
		void CodeCloseIfStatement(uint8_t *ifs_pt);
		uint8_t* CodeStartWhileStatement(void);
		void CodeCloseWhileStatement(uint8_t *ws_pt, uint8_t *exs);
		num_t* GetConsts(void);
		int GetConstsCount(void);
		void DeleteTemp(void);
		void DeleteBuffer(void);
		uint8_t* GetCode(void);
		// ###
		Uint8buffer *rcode;
		Uint8buffer *bytecode;
		num_t *consts;
		int consts_count;
		var_name *locals;
		uint8_t total_locals_count;
		scope_info *current_scope = nullptr;
		int last_scope_id;
};

#endif