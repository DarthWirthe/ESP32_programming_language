
#include "scr_lex.h"
#include "HardwareSerial.h"

char LEX_C_BUFFER[C_BUFFER_LEN];

const char* token_name[] {
	"If", "Elseif", "Else", "While", "Do", "For", "Function", "Return", "LeftParen", "RightParen","LeftSquare", "RightSquare", "LeftCurly", "RightCurly", 
	"Assign", "Less", "LessOrEqual", "Equal", "NotEqual", "Greater", "GreaterOrEqual", "Plus", "Minus", "Asterisk", "Slash", "Modulo", "And", "Or", "Not",
	"Dot", "Comma", "Colon", "Semicolon", "SingleQuote", "DoubleQuote", "Int", "Float",
	"Identifier", "NewLine", "End", "Unexpected"
};

const char* Token_to_string(Token::Type t) {
	return token_name[(int)t];
}

static const char* reserved_keywords[] = {
	"if",
	"elseif",
	"else",
	"while",
	"do",
	"for",
	"function",
	"return",
	"and",
	"or"
};

Token::Token() {
	
}

Token::Token(Type _type) {
	type = _type;
	value.c = nullptr;
}

Token::Token(Type _type, int _value) { // тип и значение
	type = _type;
	value.i = _value;
}

Token::Token(Type _type, char* _value){ // тип и значение
	type = _type;
	value.c = _value;
}

bool IsLetterChar(char c){
	if ( ( c >= 'A' && c <= 'z' ) || c == '_' )		
		return true;
	return false;
}

bool IsDigit(char c){
	if ( c >= '0' && c <= '9' )
		return true;
	return false;
}

bool IsIdentifiedChar(char c){
	if ( IsLetterChar(c) || IsDigit(c) )
		return true;
	return false;
}

bool IsSpace(char c){ // является ли символ пробелом
	switch (c) {
		case ' ':
		case '\t':
		case '\r':
		return true;
	default:
		return false;
	}
}

int FindReserved(){
	for(int i = 0; i < RESERVED_NUM; i++)
	{
		if(Compare_strings(LEX_C_BUFFER, reserved_keywords[i]))
			return i;
	}
	return -1;
}

bool IsReserved(){
	if(FindReserved() == -1)
		return false;
	return true;
}

Token ReservedToken(){
	return Token((Token::Type)FindReserved());
}

Lexer::Lexer(){}

Lexer::Lexer(int input_type, std::string s) {
	InputType = input_type;
	if (input_type == LEX_INPUT_STRING) {
		char *cstr = new char[s.length() + 1];
		strcpy(cstr, s.c_str());
		input_base = cstr;
		pointer = input_base;
	} else if (input_type == LEX_INPUT_FILE) {
		SPIFFS_BEGIN();
		file = FS_OPEN_FILE(SPIFFS, s.c_str());
	}
}

void Lexer::Pointer(char *inp){
	pointer = inp;
}

char Lexer::Peek(){
	if (InputType == LEX_INPUT_FILE) {
	if (file.available()) {
        return file.peek();
    }
	} else if (InputType ==  LEX_INPUT_STRING) {
		return *pointer;
	}
}

char Lexer::PeekNext(){
	return *(pointer+1);
}

char Lexer::Get(){
	if (InputType == LEX_INPUT_FILE) {
	if (file.available()) {
        return file.read();
    }
	} else if (InputType ==  LEX_INPUT_STRING) {
		return *pointer++;
	}
}

Token Lexer::Next()
{
	Token token;
	char last_c;
	char temp_char;
	char *cpoint;
	int len;
	
	while (IsSpace(Peek())){Get();column++;} // пропустить пробелы

	last_c = Get();
	if (IsLetterChar(last_c)) // если найдена буква
	{
		len = 0;
		temp_char = last_c;
		while (true)
		{
			if (IsIdentifiedChar(temp_char))
			{
				if (len < (C_BUFFER_LEN - 1))
				{
					LEX_C_BUFFER[len] = temp_char;
					len++;
					column++;
				}
			}
			if (!IsIdentifiedChar(Peek()))
			{
				LEX_C_BUFFER[len] = '\0';
				if (IsReserved())
				{
					token = ReservedToken();
					last_token = token;
					return token;
				} else {
					cpoint = new char[len + 1];
					Copy_str(LEX_C_BUFFER, cpoint);
					token.type = Token::Type::Identifier;
					token.value.c = cpoint;
					last_token = token;
					return token;
				}
			}
			temp_char = Get();
		}
	}
	else if (IsDigit(last_c)) // если найдена цифра
	{
		bool IsFloat = false;
		len = 0;
		temp_char = last_c;
		while (1)
		{
				if (IsDigit(temp_char))
				{
					if (len < (C_BUFFER_LEN - 1))
					{
						LEX_C_BUFFER[len] = temp_char;
						len++;
						column++;
					}
				}
				if (Peek() == '.') {
					if (!IsFloat) {
						IsFloat = true;
						LEX_C_BUFFER[len] = '.';
						len++;
						column++;
					} else {
						/*Ошибка!*/
					}
				} else if (!IsDigit(Peek())) {
					LEX_C_BUFFER[len] = '\0';
					if (!IsFloat) {
						token.value.i = String_to_int(LEX_C_BUFFER);
						token.type = Token::Type::Int;
					} else {
						token.value.f = String_to_float(LEX_C_BUFFER);
						token.type = Token::Type::Float;
					}
					last_token = token;
					return token;
				}
			temp_char = Get();
		}
	}
	else if (last_c == '@') // Пропускает комментарий до конца строки
	{
		while (Peek() != '\0')
		{
			column++;
			if (Get() == '\n')
			{
				column = 1;
				line++;
				token.type = Token::Type::NewLine;
				last_token = token;
				return token;
			}
		}
	}
	else if (last_c == '\n') // Конец строки
	{
		column = 1;
		line++;
		token.type = Token::Type::NewLine;
		last_token = token;
		return token;
	}
	else if (last_c == '(') // левая скобка
	{
		column++;
		token.type = Token::Type::LeftParen;
		last_token = token;
		return token;
	}
	else if (last_c == ')') // правая скобка
	{
		column++;
		token.type = Token::Type::RightParen;
		last_token = token;
		return token;
	}
	else if (last_c == '[') // левая квадратная скобка
	{
		column++;
		token.type = Token::Type::LeftSquare;
		last_token = token;
		return token;
	}
	else if (last_c == ']') // правая квадратная скобка
	{
		column++;
		token.type = Token::Type::RightSquare;
		last_token = token;
		return token;
	}
	else if (last_c == '{') // левая фигурная скобка
	{
		column++;
		token.type = Token::Type::LeftCurly;
		last_token = token;
		return token;
	}
	else if (last_c == '}') // правая фигурная скобка
	{
		column++;
		token.type = Token::Type::RightCurly;
		last_token = token;
		return token;
	}
	else if (last_c == '<') // меньше
	{
		if (Peek() == '=') {
			token.type = Token::Type::LessOrEqual;
			Get();
			column+=2;
		}
		else {
			token.type = Token::Type::Less;
			column++;
		}
		
		last_token = token;
		return token;
	}
	else if (last_c == '>') // больше
	{
		if (Peek() == '=') {
			token.type = Token::Type::GreaterOrEqual;
			Get();
			column+=2;
		}
		else {
			token.type = Token::Type::Greater;
			column++;
		}
		last_token = token;
		return token;
	}
	else if (last_c == '=') // равно (присваивание)
	{
		if (Peek() == '=') {
			token.type = Token::Type::Equal;
			Get();
			column+=2;
		}
		else {
			token.type = Token::Type::Assign;
			column++;
		}
		last_token = token;
		return token;
	}
	else if (last_c == '+') // плюс (сложение)
	{
		column++;
		token.type = Token::Type::Plus;
		last_token = token;
		return token;
	}
	else if (last_c == '-') // минус (вычитание)
	{
		column++;
		token.type = Token::Type::Minus;
		last_token = token;
		return token;
	}
	else if (last_c == '*') // звёздочка (умножение)
	{
		column++;
		token.type = Token::Type::Asterisk;
		last_token = token;
		return token;
	}
	else if (last_c == '/') // косая черта (деление)
	{
		column++;
		token.type = Token::Type::Slash;
		last_token = token;
		return token;
	}
	else if (last_c == '%') // знак процента (остаток от деления)
	{
		column++;
		token.type = Token::Type::Modulo;
		last_token = token;
		return token;
	}
	else if (last_c == '!') // восклицательный знак (НЕ)
	{
		if (Peek() == '=') {
			token.type = Token::Type::NotEqual;
			Get();
			column+=2;
		}
		else {
			token.type = Token::Type::Not;
			column++;
		}
		last_token = token;
		return token;
	}
	// else if (last_c == '&') // И
	// {
		// column++;
		// token.type = Token::Type::And;
		// last_token = token;
		// return token;
	// }
	// else if (last_c == '|') // ИЛИ
	// {
		// column++;
		// token.type = Token::Type::Or;
		// last_token = token;
		// return token;
	// }
	else if (last_c == '.') // точка (разделитель числа)
	{
		column++;
		token.type = Token::Type::Dot;
		last_token = token;
		return token;
	}
	else if (last_c == ',') // запятая (перечисление)
	{
		column++;
		token.type = Token::Type::Comma;
		last_token = token;
		return token;
	}
	else if (last_c == ':') // двоеточие
	{
		column++;
		token.type = Token::Type::Colon;
		last_token = token;
		return token;
	}
	else if (last_c == ';') // точка с запятой (конец строки)
	{
		column++;
		token.type = Token::Type::Semicolon;
		last_token = token;
		return token;
	}
	else if (last_c == '\'') // кавычка
	{
		column++;
		token.type = Token::Type::SingleQuote;
		last_token = token;
		return token;
	}
	else if (last_c == '"') // двойная кавычка
	{
		column++;
		token.type = Token::Type::DoubleQuote;
		last_token = token;
		return token;
	}
	else if (last_c == '\0') // Конец ввода
	{
		token.type = Token::Type::End;
		last_token = token;
		return token;
	}
	else if(InputType == LEX_INPUT_FILE && !file.available()) {
		token.type = Token::Type::End;
		last_token = token;
		return token;
	}
	token.type = Token::Type::Unexpected;
	last_token = token;
	Serial.print("Unexpected symbol #");
	Serial.println((uint8_t)last_c);
	return token; // неожиданный символ
}


