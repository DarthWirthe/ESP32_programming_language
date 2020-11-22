
/* scr_parser.h
** Заголовочный файл.
** Синтаксический анализатор (парсер)
*/

#pragma once
#ifndef SCR_PARSER_H
#define SCR_PARSER_H

#include <string>
#include <vector>
#include "scr_lex.h"
#include "scr_code.h"

struct var_reg {
	char *name;
	VarType type; // из code.h
};

static const var_reg var_types_list[] = {
	{(char*)"int", VarType::Int},
	{(char*)"float", VarType::Float}
};

class Parser
{
	public:
		Parser(void);
		Parser(Lexer *_lexer_class);
		void Exception(std::string s);
		void ErrorExpectedToken(Token::Type expected, Token::Type got);
		void Next(Token::Type);
		void NewLine(void);
		void StatementList(void);
		void Statement(void);
		void Assignment(char *name);
		void Declaration(void);
		uint8_t FunctionArgs(void);
		void FunctionDeclaration(void);
		void FunctionCall(char *name);
		void ReturnStatement(void);
		void ArrayDeclaration(char *name);
		void IfStatement(void);
		void WhileStatement(void);
		std::vector<Node> Expr(uint8_t lp);
		std::vector<Node> ConstExpr(uint8_t lp);
		void Program(void);
		void Parse(void);
		uint8_t* GetCode(void);
		num_t* GetConsts(void);
		size_t GetCodeLength(void);
		size_t GetConstsCount(void);
		
		bool Terminated;
	private:
		Code *code;
		Lexer *lex;
		Token current_token;
};

#endif
