
// Лексический анализатор

#pragma once
#ifndef SCR_LEX_H
#define SCR_LEX_H

#include <string>
#include <stdint.h>
#include "scr_tref.h"
#include "scr_file.h"

#define LEX_INPUT_STRING 0
#define LEX_INPUT_FILE 1

#define RESERVED_NUM 12

typedef union {
	int i;
	float f;
	char *c;
} token_value;

class Token
{
	public:
		enum class Type {
			If,
			ElseIf,
			Else,
			While,
			Do,
			For,
			Function,
			Return,
			And,
			Or,
			Const,
			Global,
//###
			LeftParen,
			RightParen,
			LeftSquare,
			RightSquare,
			LeftCurly,
			RightCurly,
			Assign,
			Less,
			LessOrEqual,
			Equal,
			NotEqual,
			Greater,
			GreaterOrEqual,
			DoubleLess,
			DoubleGreater,
			Plus,
			Minus,
			Asterisk,
			Slash,
			Modulo,
			Not,
			Tilde,
			Ampersand,
			Pipe,
			Dot,
			Comma,
			Colon,
			Semicolon,
			SingleQuote,
			DoubleQuote,
			Int,
			Float,
			String,
			Identifier,
			NewLine,
			EndOfFile,
			Unexpected,
			
			UndefToken
		};
		Token(void);
		Token(Type _type);
		Token(Type _type, int _value);
		Token(Type _type, char* _value);
		Type type;
		token_value value;
};

const char* Token_to_string(Token::Type t);

class Lexer
{
	public:
		Lexer(void);
		Lexer(int input_type, std::string s);
		int FindReserved(void);
		Token ReservedToken(void);
		char Peek(void);
		char Get(void);
		char PeekNext(void);
		Token Next(void);
		void Pointer(char *p);
		void ClearBuffer(void);
		bool EndOfFile(char c);
		void End(void);
		//###
		Token last_token;
		int line = 1;
		int column = 1;
	private:
		std::string T_BUF;
		int InputType;
		char *pointer;
		char *input_base;
		File file;
};

#endif


