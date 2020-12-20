
/* scr_parser.h
** Заголовочный файл.
** Синтаксический анализатор (парсер)
*/

#pragma once
#ifndef SCR_PARSER_H
#define SCR_PARSER_H

#include <vector>
#include "scr_lex.h"
#include "scr_code.h"

class Parser
{
	public:
		Parser(void);
		Parser(Lexer *_lexer_class);
		void InternalException(int code);
		void ErrorExpectedToken(Token::Type expected, Token::Type got);
		void ErrorExpectedExpr(void);
		void ErrorExpectedSymbol(const char *c);
		void ErrorExtraSymbol(const char *c);
		void Next(Token::Type);
		void NewLine(void);
		void StatementList(void);
		void Statement(void);
		void Assignment(char *name);
		VarType VariableType(void);
		void Declaration(char *name, VarType t);
		std::vector<func_arg_struct> FunctionArgs(void);
		void FunctionDeclaration(void);
		void FunctionCall(char *name);
		void ReturnStatement(void);
		void ArrayDeclaration(char *name, VarType t);
		void IfStatement(void);
		void WhileStatement(void);
		char* ParseIdentSequence(char *first_val);
		std::vector<Node> Expr(uint8_t lp);
		void Program(void);
		int Parse(void);
		Code* Compiler(void);
		uint8_t* GetCode(void);
		num_t* GetConsts(void);
		size_t GetCodeLength(void);
		size_t GetConstsCount(void);
		
		bool exit_code;
	private:
		Code *code;
		Lexer *lex;
		Token current_token;
};

#endif
