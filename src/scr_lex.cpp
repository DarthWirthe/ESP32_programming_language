
// Лексический анализатор


#include "scr_lex.h"
#include "HardwareSerial.h"


const char* token_name[] {
	"If", "Elseif", "Else", "While", "Do", "For", "Function", "Return", "And",
	"Or", "Const", "Global", "LeftParen", "RightParen", "LeftSquare",
	"RightSquare", "LeftCurly", "RightCurly", "Assign", "Less",
	"LessOrEqual", "Equal", "NotEqual", "Greater", "GreaterOrEqual",
	"DoubleLess", "DoubleGreater", "Plus", "Minus", "Asterisk", "Slash",
	"Modulo", "Not", "Tilde", "Ampersand", "Pipe", "Dot", "Comma", "Colon",
	"Semicolon", "SingleQuote", "DoubleQuote", "Int", "Float", "String",
	"Identifier", "NewLine", "EOF", "Unexpected"
};

const char* Token_to_string(Token::Type t) {
	return token_name[(int)t];
}

const char* reserved_keywords[] = {
	"if",
	"elseif",
	"else",
	"while",
	"do",
	"for",
	"function",
	"return",
	"and",
	"or",
	"const",
	"global"
};

Token::Token() {}

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

static bool IsLetterChar(char c){
	if (( c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z' ) || c == '_')		
		return true;
	return false;
}

static bool IsDigit(char c){
	if ( c >= '0' && c <= '9' )
		return true;
	return false;
}

static bool IsIdentifiedChar(char c){
	if ( IsLetterChar(c) || IsDigit(c) )
		return true;
	return false;
}

// является ли символ пробелом
static bool IsSpace(char c) { 
	switch (c) {
		case ' ':
		case '\t':
		case '\r':
		return true;
	default:
		return false;
	}
}

int Lexer::FindReserved() {
	for(int i = 0; i < RESERVED_NUM; i++)
	{
		if(Compare_strings(T_BUF.c_str(), reserved_keywords[i]))
			return i;
	}
	return -1;
}

Token Lexer::ReservedToken() {
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
	return 0;
}

char Lexer::PeekNext(){
	return *(pointer + 1);
}

char Lexer::Get(){
	if (InputType == LEX_INPUT_FILE) {
	if (file.available()) {
        return file.read();
    }
	} else if (InputType ==  LEX_INPUT_STRING) {
		return *pointer++;
	}
	return 0;
}

Token Lexer::Next()
{
	Token token;
	token.value.c = nullptr; // !Пометить токен NULLPTR для последующего удаления
	char last_c;
	char temp_char;
	char *cpoint;

lexer_start: // 'goto label'
	last_c = Peek();
	while (IsSpace(last_c)) { // пропустить пробелы и табы
		Get();
		column++;
		last_c = Peek();
	}

	last_c = Get();
	if (IsLetterChar(last_c)) // если найдена буква
	{
		temp_char = last_c;
		while (true)
		{
			if (IsIdentifiedChar(temp_char))
			{
				T_BUF += temp_char;
				column++;
			}
			if (!IsIdentifiedChar(Peek()))
			{
				if (FindReserved() != -1) // если это ключевое слово
				{
					token = ReservedToken();
					ClearBuffer();
					last_token = token;
					return token;
				} else { // иначе это идентификатор
					cpoint = new char[T_BUF.size() + 1];
					Copy_str(T_BUF.c_str(), cpoint);
					token.type = Token::Type::Identifier;
					token.value.c = cpoint;
					ClearBuffer();
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
		temp_char = last_c;
		while (1)
		{
				if (IsDigit(temp_char))
				{
					T_BUF += temp_char;
					column++;
				}
				if (Peek() == '.') {
					if (!IsFloat) {
						IsFloat = true;
						T_BUF += '.';
						column++;
					} else {
						/*Ошибка!*/
					}
				} else if (!IsDigit(Peek())) {
					if (!IsFloat) {
						token.value.i = Const_char_to_int(T_BUF.c_str());
						token.type = Token::Type::Int;
					} else {
						token.value.f = Const_char_to_float(T_BUF.c_str());
						token.type = Token::Type::Float;
					}
					ClearBuffer();
					last_token = token;
					return token;
				}
			temp_char = Get();
		}
	}
	else if (last_c == '"') // двойная кавычка
	{
		temp_char = Get(); // текст между кавычек записывается в буфер
		while (!EndOfFile(temp_char) && temp_char != '"') {
			if (temp_char == '\n') {
				column = 1;
				line++;
			} else {
				T_BUF += temp_char;
				column++;
			}
			temp_char = Get();
		}
		cpoint = new char[T_BUF.size() + 1];
		Copy_str(T_BUF.c_str(), cpoint);
		token.value.c = cpoint;
		ClearBuffer();
		token.type = Token::Type::String;
		last_token = token;
		return token;
	}
	else if (last_c == '\n') {
		line++;
		column = 1;
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
		if (Peek() == '=') { // <=
			token.type = Token::Type::LessOrEqual;
			Get();
			column+=2;
		}
		else if (Peek() == '<') { // <<
			token.type = Token::Type::DoubleLess;
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
		if (Peek() == '=') { // >=
			token.type = Token::Type::GreaterOrEqual;
			Get();
			column+=2;
		}
		else if (Peek() == '>') { // >>
			token.type = Token::Type::DoubleGreater;
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
		// Комментарий
		if (Peek() == '/') {
			while (!EndOfFile(Peek())) {
				if (Get() == '\n') {
					column = 1;
					line++;
					goto lexer_start; // комментарий пропущен
				}
			}
		}
		else if (Peek() == '*') {
			while (!EndOfFile(Peek())) {
				last_c = Get();
				column++;
				if (last_c == '*' && Peek() == '/') {
					Get();
					goto lexer_start; // комментарий пропущен
				}
				if (last_c == '\n') {
					column = 1;
					line++;
				}
			}
		} else {
			column++;
			token.type = Token::Type::Slash;
			last_token = token;
			return token;
		}
	}
	else if (last_c == '%') // знак процента (остаток от деления)
	{
		column++;
		token.type = Token::Type::Modulo;
		last_token = token;
		return token;
	}
	else if (last_c == '!') // восклицательный знак (отрицание)
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
	else if (last_c == '~') // Тильда (побитовое отрицание)
	{
		column++;
		token.type = Token::Type::Tilde;
		last_token = token;
		return token;
	}
	else if (last_c == '&') // побитовое И
	{
		column++;
		token.type = Token::Type::Ampersand;
		last_token = token;
		return token;
	}
	else if (last_c == '|') // побитовое ИЛИ
	{
		column++;
		token.type = Token::Type::Pipe;
		last_token = token;
		return token;
	}
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
	else if (EndOfFile(last_c)) // Конец ввода
	{
		token.type = Token::Type::EndOfFile;
		last_token = token;
		return token;
	}
	token.type = Token::Type::Unexpected;
	token.value.i = (uint8_t)last_c;
	last_token = token;
	return token; // неожиданный символ
}

void Lexer::ClearBuffer() {
	T_BUF.clear();
	T_BUF.shrink_to_fit();
}

bool Lexer::EndOfFile(char c) {
	if (c == '\0')
		return true;
	if (InputType == LEX_INPUT_FILE && !file.available())
		return true;
	return false;
}

void Lexer::End() {
	if (InputType == LEX_INPUT_FILE)
		SPIFFS_CLOSE_FILE(file);
}


