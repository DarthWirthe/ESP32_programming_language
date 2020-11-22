
// Лексический анализатор

#pragma once
#ifndef SCR_LEX_H
#define SCR_LEX_H

//#include "utils/scr_dbg.h"
#include <string>
#include <stdint.h>
#include "scr_tref.h"
#include "scr_file.h"

#define LEX_INPUT_STRING 0
#define LEX_INPUT_FILE 1

#define RESERVED_NUM 10

#define C_BUFFER_LEN 80

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
			Plus,
			Minus,
			Asterisk,
			Slash,
			Modulo,
			Not,
			Dot,
			Comma,
			Colon,
			Semicolon,
			SingleQuote,
			DoubleQuote,
			Int,
			Float,
			Identifier,
			NewLine,
			End,
			Unexpected,
			
			UndefToken
		};
		Token(void);
		Token(Type _type); // только тип
		Token(Type _type, int _value); // тип и значение
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

		char Peek(void); // возвращает последний символ
		char Get(void); // переход к следующему символу
		char PeekNext(void); // возвращает следующий символ
		Token Next(void);
		void Pointer(char *p);
		//###
		Token last_token;
		int line = 1;
		int column = 1;
	private:
		int InputType;
		char *pointer;
		char *input_base;
		File file;
};

#endif


