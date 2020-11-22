
/*
	Компилятор, заголовочный файл.
*/

#pragma once
#ifndef SCR_CODE_H
#define SCR_CODE_H

#include <stdio.h>
#include <vector>
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
} NodeValueUnion;

struct Node {
	Node(){}
	Node(uint8_t _type, NodeValueUnion _value) {
		type = _type;
		value = _value;
	}
	uint8_t type;
	NodeValueUnion value;
};

// ######

enum class VarType {
	None,
	Int,
	Float,
	Function,
	ConstInt,
	ConstFloat
};

// информация об объявленной ранее функции
struct FuncData {
	uint8_t args;
	uint8_t cnst;
};

struct VarState {
	~VarState(void);
	VarState(){}
	char *name; // имя переменной
	uint8_t type; // тип переменной
	uint8_t data; // аргументы / константа
	uint8_t id; // номер ид
};

struct func_descr { // можно удалить
	func_descr(void);
	func_descr(uint8_t *start, uint8_t *end);
	uint8_t *start_pt;
	uint8_t *end_pt;
};

struct FuncPoint {
	size_t c;
	size_t pt;
};

struct FuncState {
	char *name;
	uint8_t locals_start;
	uint8_t locals_count;
	uint8_t max_stack;
	uint8_t func_start;
	std::vector<uint8_t> fcode;
	FuncState *prev;
	FuncState():
	prev(nullptr)
	{}
};

struct scope_info {
	uint8_t *code_base; // указатель на начало
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

class Code {
	public:
		Code(void);
		~Code();
		void Add(uint8_t i);
		void Add_08(uint8_t n);
		void Add_16(uint16_t n);
		void NewScope(char *name);
		void CloseScope(void);
		int AddConstInt(int n);
		int AddConstFloat(float n);
		int AddLocal(char *name, VarType type);
		void SetArgs(char *name, uint8_t args);
		uint8_t GetArgs(char *name);
		int FindLocal(char *name);
		void CodeOperator(OpType t, bool IsFloat);
		int FindConstInt(int n);
		int FindConstFloat(float n);
		int CodeConstInt(int n);
		int CodeConstFloat(float n);
		int DeclareConstVarInt(char *name, int n, VarType type, bool code);
		//void DeclareConstVarFloat(char *name, float n, VarType type, bool code);
		void CodeJump(int shift, bool condition, bool expected);
		void Expression(std::vector<Node> expr);
		int MainScope(void);
		void MainScopeEnd(int pos_const);
		void LocalLoad(char *name);
		void LocalStore(char *name);
		int LocalDeclare(char *name, VarType type);
		FuncPoint FunctionDeclare(char *name);
		void FunctionClose(FuncPoint p);
		void CallFunction(char *name, int args);
		void ReturnOperator(VarType type);
		size_t StartIfStatement(void);
		size_t ElseIfStatement(void);
		void CloseIfBranch(size_t ifs_pt);
		void CloseIfStatement(std::vector<size_t> labels);
		size_t StartWhileStatement(void);
		void CloseWhileStatement(size_t ws_pt, size_t condition);
		void ArrayDeclaration(char *name);
		void ArrayLoad(char *name);
		void ArrayStore(char *name);
		size_t GetFuncCodeSize(void); 
		num_t* GetConsts(void);
		uint8_t* GetCode(void);
		size_t GetCodeLength(void);
		size_t GetConstsCount(void);
		// ###
	private:
		FuncState *current_scope = nullptr;
		std::vector<FuncData> declared_functions;
		std::vector<uint8_t> bytecode;
		std::vector<num_t> num_consts;
		std::vector<VarState> locals;
		size_t main_scope_p;
		size_t f_pt;
};

#endif
