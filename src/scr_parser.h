
#pragma once
#ifndef SCR_PARSER_H
#define SCR_PARSER_H

#include "scr_lex.h"
#include "scr_code.h"
#include <string.h>
//#include "utils/scr_dbg.h"

struct node_stack {
	node node_stack[16];
	int nst_pt = -1;
	void Push(node n);
	node Pop(void);
	node Top(void);
};

class Parser
{
	public:
		Parser(void);
		Parser(Lexer _lexer_obj);
		void Exception(void);
		void Next(Token::Type);
		void StatementList(void);
		void Statement(void);
		void FunctionDeclaration(void);
		void FunctionCall(char *name);
		void Assignment(char *name);
		void ArrayDeclaration(char *name);
		void IfStatement(uint8_t *ifs_pt);
		void ElseStatement(uint8_t *ifs_pt);
		void WhileStatement(void);
		void Expr(uint8_t lp);
		void Program(void);
		void Parse(void);	
		uint8_t* GetCode(void);
		num_t* GetConsts(void);
		int GetConstsCount(void);
		
		Code coder_obj;
		Lexer lexer_obj;
		Token current_token;
};

#endif