
/**
**	Компилятор, заголовочный файл.
*/

#pragma once
#ifndef SCR_CODE_H
#define SCR_CODE_H

#include <stdio.h>
#include <vector>
#include <string>
#include <string.h>
#include "scr_lib.h"
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
	UnBitNot,
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
	BinLeftShift,
	BinRightShift,
	BinAnd,
	BinOr,
	BinBitAnd,
	BinBitOr,
	BinBitXor
};

enum class NodeType {
	None,
	ConstInt,
	ConstFloat,
	Var,
	Prefix,
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
	Node(NodeType _type, NodeValueUnion _value) {
		type = _type;
		value = _value;
	}
	NodeType type;
	NodeValueUnion value;
};

// ######

enum class VarType {
	None,
	Int,
	Float,
	Function,
	Array,
	String,
	ConstInt,
	ConstFloat
};

struct v_reg_struct {
	VarType type;
	unsigned 	size : 1, // битовые поля
				is_const : 1,
				is_int : 1,
				is_pointer : 1,
				unused : 28;
};

/* Флаги:
** size: 0 - 8 бит, 1 - 32 бит
** constant: 0 - переменная, 1 - константа
** is_int: 0 - вещественный тип, 1 - целочисленный
*/

const v_reg_struct var_types_info[] = {
	{VarType::Int, 1, 0, 1, 0},
	{VarType::Float, 1, 0, 0, 0},
	{VarType::ConstInt, 1, 1, 1, 0},
	{VarType::ConstFloat, 1, 1, 0, 0},
	{VarType::Array, 1, 0, 1, 1},
	{VarType::String, 1, 0, 1, 1},
	{VarType::Function, 1, 1, 1, 0}
};

struct var_reg_struct {
	char *name;
	VarType type;
};

const var_reg_struct var_types_list[] = {
	{(char*)"int", VarType::Int},
	{(char*)"float", VarType::Float},
	{(char*)"const int", VarType::ConstInt},
	{(char*)"const float", VarType::ConstFloat},
	{(char*)"string", VarType::String}
};

struct func_arg_struct {
	uint8_t type;
};

struct FuncData {
	char *name; // имя
	std::vector<func_arg_struct> args; // типы аргументов
	uint32_t pointer; // указатель на функцию в коде
};

struct VarState {
	char *name; // имя переменной
	VarState *parent; // родительская переменная
	unsigned  	type : 8, // тип переменной
				data : 8, // аргументы / константа
				id : 8; // номер ид
	VarState():
	parent(nullptr)
	{}
};

struct FuncState {
	char *name;	
	std::vector<uint8_t> fcode; // буфер кода
	FuncState *prev; // указатель на пред. окружение
	unsigned  	decl_funcs : 8, // количество функций, объявленных в функции
				locals_count : 8, // количество переменных, объявленных в функции
				max_stack : 8; // расширение стека
	FuncState():
	prev(nullptr)
	{}
};

class Code {
	public:
		Code(void);
		void InternalException(int code);
		void ErrorDeclared(char *name);
		void ErrorNotDeclared(char *name);
		void ErrorDeclaredFunction(char *name);
		void ErrorNotDeclaredFunction(char *name);
		void Add(uint8_t i);
		void Add_08(uint8_t n);
		void Add_16(uint16_t n);
		void NewScope(char *name);
		void CloseScope(void);
		void RegisterLib(const char* name, const lib_reg *lib, size_t size);
		int AddConstInt(int n);
		int AddConstFloat(float n);
		int AddConstString(char *s);
		int FindConstString(char *s);
		void CodeLoadVariable(VarType type, bool is_local, int index);
		void CodeStoreVariable(VarType type, bool is_local, int index);
		int PushVar(char *name, VarType type, bool is_local);
		size_t AddFunction(char *name);
		int FindFunction(char *name);
		int FindBuiltInFunction(char *lib_label, char *name);
		int FindBuiltInLib(char *lib_label);
		void SetArgs(char *name, std::vector<func_arg_struct> args);
		size_t GetArgs(char *name);
		func_arg_struct GetArg(char *name, int n);
		int FindLocal(char *name);
		int FindGlobal(char *name);
		int FindVariable(char *name, bool &is_local);
		void CodeOperator(OpType t, bool IsFloat);
		int FindConstInt(int n);
		int FindConstFloat(float n);
		int CodeConstInt(int n);
		int CodeConstFloat(float n);
		int CodeConstString(char *s);
		void CodeJump(int shift, bool condition, bool expected);
		void ExprGenericOperation(Node n);
		void Expression(std::vector<Node> expr, uint8_t _max_st = 0);
		int MainScope(void);
		void MainScopeEnd(int pos_const);
		void LoadLocal(char *name);
		void StoreLocal(char *name);
		int DeclareLocal(char *name, VarType type);
		void LoadVariable(char *name, bool &is_local);
		void StoreVariable(char *name, bool &is_local);
		void LoadGlobal(char *name);
		void StoreGlobal(char *name);
		int DeclareGlobal(char *name, VarType type);
		size_t FunctionDeclare(char *name);
		void FunctionClose(char *name, size_t p);
		void CallFunction(char *name, int args);
		void ReturnOperator(VarType type);
		size_t StartIfStatement(void);
		size_t ElseIfStatement(void);
		void CloseIfBranch(size_t ifs_pt);
		void CloseIfStatement(std::vector<size_t> labels);
		size_t StartWhileStatement(void);
		void CloseWhileStatement(size_t ws_pt, size_t condition);
		void ArrayDeclaration(char *name, VarType type);
		size_t ArrayUnknownLengthStart(void);
		void ArrayUnknownLengthEnd(size_t pos, int len);
		void LoadArray(char *name);
		void StoreArray(char *name);
		size_t GetFuncCodeSize(void); 
		num_t* GetConsts(void);
		uint8_t* GetCode(void);
		size_t GetCodeLength(void);
		size_t GetConstsCount(void);
		// ###
	private:
		FuncState *current_scope = nullptr; // текущее окружение
		std::vector<FuncData> declared_functions; // локальные функции
		std::vector<uint8_t> bytecode; // байткод
		std::vector<num_t> num_consts; // число константа
		std::vector<char*> str_consts; // строка константа
		std::vector<VarState> locals; // локальная переменная
		std::vector<VarState> globals; // глобальная переменная
		std::vector<lib_struct> cpp_lib; // встроенные библиотеки
		size_t total_str_length;
		size_t main_scope_p;
		size_t f_pt;
};

#endif
