/* scr_parser.cpp
**
*/

#include "scr_parser.h"
#include "HardwareSerial.h"

Parser::Parser(){}

Parser::Parser(Lexer *_lexer_class) {
	lex = _lexer_class;
	code = new Code();
	current_token = lex->Next();
	Terminated = false;
}

// Сообщение об ошибке
void Parser::Exception(std::string s) {
	char buf[17];
	s += " at line " + std::string(itoa(lex->line, buf, 10)) + ", column " + std::string(itoa(lex->column, buf, 10));
	throw s;
}

void Parser::ErrorExpectedToken(Token::Type expected, Token::Type got) {
	std::string exc = "Error: Expected '";
	exc += Token_to_string(expected);
	exc += "', got '";
	exc += Token_to_string(got);
	exc += "'";
	Exception(exc);
}

/* Получить лексему
** [Token::Type t] - ожидаемый тип лексемы
*/

void Parser::Next(Token::Type t) {
	if (current_token.type == t) {
		current_token = lex->Next();
	} else {
		ErrorExpectedToken(t, current_token.type);
	}
}

void Parser::NewLine() {
	while (current_token.type == Token::Type::NewLine) {
		current_token = lex->Next();
	}
}

// если токен это переменная, число, минус, плюс или '!', то это начало выражения

bool IsExprTokenType(Token::Type t)	{
	switch (t) {
		case Token::Type::LeftParen: // (
		case Token::Type::Identifier: // переменная
		case Token::Type::Int: // число
		case Token::Type::Float: // число
		case Token::Type::Minus: // ун. минус
		case Token::Type::Plus: // ун. плюс
		case Token::Type::Not: // отрицание
			return true;
		default:
			return false;
	}
}

// Является ли токен типом переменной

bool IsVarTypeToken(Token t) {
	if (t.type == Token::Type::Identifier) {
		for (int i = 0; i < sizeof(var_types_list) / sizeof(var_types_list[0]); i++) {
			if (Compare_strings(t.value.c, var_types_list[i].name))
				return true;
		}
	}
	return false;
}

// Строка -> тип переменной

VarType FindVarType(char *t) {
	for (int i = 0; i < sizeof(var_types_list) / sizeof(var_types_list[0]); i++) {
		if (Compare_strings(t, var_types_list[i].name))
			return var_types_list[i].type;
	}
}

// [Program] - Программа включена в функцию с названием '0' (нуль)

void Parser::Program() {
	Serial.println(F("Program"));
	int ms = code->MainScope();
	StatementList();
	code->MainScopeEnd(ms);
}

// [StatementList] - Несколько операторов.

void Parser::StatementList() {
	Serial.println(F("StatementList"));
	while (current_token.type != Token::Type::RightCurly && current_token.type != Token::Type::End)
	{
		if (current_token.type == Token::Type::Semicolon)
			Next(Token::Type::Semicolon);
		else if (current_token.type == Token::Type::NewLine)
			NewLine();
		else
			Statement();
	}
}

// [Statement] - Один оператор.

void Parser::Statement() {
	Serial.println(F("Statement"));
	char *n;
	Token::Type type = current_token.type;
	if (IsVarTypeToken(current_token)) { // объявление переменной
		Declaration();
	} else {
		switch(type) {
			case Token::Type::Identifier:
				n = current_token.value.c;
				Next(Token::Type::Identifier);
				if (current_token.type == Token::Type::LeftParen) {
					FunctionCall(n); // вызов функции
				}
				else if (current_token.type == Token::Type::LeftSquare) {
					ArrayDeclaration(n); // объявление массива
				}
				else {
					Assignment(n); // присваивание
				}
				break;
			case Token::Type::Function:
				FunctionDeclaration(); // объявление функции
				break;
			case Token::Type::If:
				IfStatement();
				break;
			case Token::Type::While:
				WhileStatement();
				break;
			case Token::Type::Return:
				ReturnStatement();
				break;
			case Token::Type::End:
				Serial.println(F("The End!"));
				break;
			default:
				Serial.print("Error: Unknown statement: ");
				Serial.println(Token_to_string(type));
				throw "";
				break;
		}
	}
}

/* [Assignment] - Оператор присваивания
** прим.: a = 2 * (b - 1)
*/

void Parser::Assignment(char *name) {
	Serial.println(F("Assignment"));
	Next(Token::Type::Assign); // '='
	code->Expression(Expr(0)); // выражение справа
	code->LocalStore(name); // переменная слева
}

/* [Declaration] - Оператор объявления переменной
** прим.: int v = 1
*/

void Parser::Declaration() {
	Serial.println(F("Declaration"));
	bool assignment = false;
	VarType t = FindVarType(current_token.value.c);
	Next(current_token.type);
	char *name = current_token.value.c;
	Next(Token::Type::Identifier);
	// если есть '=', то должно быть выражение
	if (current_token.type == Token::Type::Assign) {
		Next(Token::Type::Assign); // '='
		code->Expression(Expr(0)); // выражение справа
		assignment = true;
	}
	code->LocalDeclare(name, t);
	if (assignment) {
		code->LocalStore(name); // переменная слева
	}
}

/* [FunctionArgument] - Аргумент при объявлении функции
** прим.: function func(int arg1, int arg2)
*/

uint8_t Parser::FunctionArgs() {
	Serial.println(F("FunctionArgs"));
	char *name;
	uint8_t args = 0;
	VarType t;
	while (current_token.type != Token::Type::RightParen) {
		if (IsVarTypeToken(current_token)) {
			t = FindVarType(current_token.value.c);
			Next(current_token.type);
			name = current_token.value.c;
			Next(Token::Type::Identifier);
			code->LocalDeclare(name, t);
		}
		if (current_token.type == Token::Type::Comma) // все запятые
			Next(Token::Type::Comma);
		else if (current_token.type == Token::Type::RightParen) // ')'
			{}
		else
			{Exception("Error: FunctionCall");}
	}
	return args;
}

/* [FunctionDeclaration] - Объявление функции
** прим.: function func(int a1, int a2){ return (arg1 + arg2) / 2 }
*/

void Parser::FunctionDeclaration() {
	Serial.println(F("FunctionDeclaration"));
	char *name;
	FuncPoint p;
	uint8_t args = 0;
	Next(Token::Type::Function); // 'function'
	name = current_token.value.c;
	Next(Token::Type::Identifier);
	p = code->FunctionDeclare(name); // 
	Next(Token::Type::LeftParen); // левая круглая скобка
	code->SetArgs(name, FunctionArgs()); // аргументы
	Next(Token::Type::RightParen); // правая круглая скобка
	NewLine();
	Next(Token::Type::LeftCurly); // левая фигурная скобка
	NewLine();
	StatementList(); // всякие операторы между скобок
	NewLine();
	Next(Token::Type::RightCurly); // правая фигурная скобка
	code->FunctionClose(p);
}

/* [FunctionCall] - Вызов функции
** прим:. f(arg1, arg2)
*/

void Parser::FunctionCall(char *name) {
	Serial.println(F("FunctionCall"));
	uint8_t args = 0;
	Next(Token::Type::LeftParen);
	while (current_token.type != Token::Type::RightParen) {
		if (IsExprTokenType(current_token.type)) {
			Serial.println("CallParameter");
			code->Expression(Expr(1));
			args++;
		}
		if (current_token.type == Token::Type::Comma) // ','
			Next(Token::Type::Comma);
		else if (current_token.type == Token::Type::RightParen)
			{}
		else
			Exception("Error: FunctionCall");
	}
	Next(Token::Type::RightParen);
	code->CallFunction(name, args);
}

/* [ReturnStatement] - Возврат значения
** прим.: return 10
*/

void Parser::ReturnStatement() {
	Next(Token::Type::Return);
	if (IsExprTokenType(current_token.type)) {
		code->Expression(Expr(0));
		code->ReturnOperator(VarType::Int);
	} else {
		code->ReturnOperator(VarType::None);
	}
}

// [IfStatement] - Оператор условия
// прим.: if(a){...} elseif(b){...} else{...}

void Parser::IfStatement() {
	Serial.println(F("IfStatement"));
	size_t ifs_pt;
	std::vector<size_t> labels;
	Next(Token::Type::If);
	code->Expression(Expr(0)); // условие
	ifs_pt = code->StartIfStatement();
	Next(Token::Type::LeftCurly); // левая фигурная скобка
	StatementList(); // разные операторы между скобок
	Next(Token::Type::RightCurly); // правая фигурная скобка
	while (current_token.type == Token::Type::ElseIf) {
		labels.push_back(code->ElseIfStatement());
		code->CloseIfBranch(ifs_pt);
		Next(Token::Type::ElseIf);
		code->Expression(Expr(0)); // условие
		ifs_pt = code->StartIfStatement();
		Next(Token::Type::LeftCurly); // левая фигурная скобка
		StatementList(); // разные операторы между скобок
		Next(Token::Type::RightCurly); // правая фигурная скобка
		code->CloseIfBranch(ifs_pt);
	}
	if (current_token.type == Token::Type::Else) {
		labels.push_back(code->ElseIfStatement());
		code->CloseIfBranch(ifs_pt);
		Next(Token::Type::Else);
		Next(Token::Type::LeftCurly); // левая фигурная скобка
		StatementList(); // разные операторы между скобок
		Next(Token::Type::RightCurly); // правая фигурная скобка
	} else {
		code->CloseIfBranch(ifs_pt);
	}
	code->CloseIfStatement(labels);
}

// [WhileStatement] - Цикл с предусловием.
// прим.: while(a){...}

void Parser::WhileStatement() {
	Serial.println("WhileStatement");
	size_t stat, cond;
	Next(Token::Type::While);
	cond = code->GetFuncCodeSize();
	code->Expression(Expr(0));
	stat = code->StartWhileStatement();
	Next(Token::Type::LeftCurly); // левая фигурная скобка
	StatementList();
	Next(Token::Type::RightCurly); // правая фигурная скобка
	code->CloseWhileStatement(stat, cond);
}

// [ArrayDeclaration] - Объявление массива
// прим.: array[5]{2,6,3,1,4}; arr[2];

void Parser::ArrayDeclaration(char *name) {
	Serial.println("ArrayDeclaration");
	// bool UnknownLength = true;
	// int num = 0;
	// Parser::Next(Token::Type::LeftSquare);
	// if (current_token.type != Token::Type::RightSquare) {
		// Expr(0);
		// coder_obj->CodeExpression(expr_buf);
		// UnknownLength = false;
	// }
	// Next(Token::Type::RightSquare);
	// coder_obj->CodeArrayDeclaration(name);
	// if (current_token.type == Token::Type::LeftCurly) {
		// Next(Token::Type::LeftCurly);
		// while (current_token.type != Token::Type::RightCurly) {
			// if (IsExprTokenType(current_token.type)) {
				// Serial.println("ArrayParameter");
				// Expr(0);
				// coder_obj->CodeExpression(expr_buf);
			// }
			// else if (current_token.type == Token::Type::Comma)
				// Next(Token::Type::Comma);
			// else if (current_token.type == Token::Type::RightCurly)
				// {}
			// else
				// {Serial.println("Error: ArrayDeclaration");}
		// }
		// Next(Token::Type::RightCurly);
	// }
}


int GetPriorityLevel(OpType type) {
	switch(type) {
		case OpType::UnPlus:
		case OpType::UnMinus:
		case OpType::UnNot:
			return 7;
			break;
		case OpType::BinMul:
		case OpType::BinDiv:
		case OpType::BinMod:
			return 6;
			break;
		case OpType::BinAdd:
		case OpType::BinSub:
			return 5;
			break;
		case OpType::BinLess:
		case OpType::BinLessOrEqual:
		case OpType::BinGreater:
		case OpType::BinGreaterOrEqual:
			return 4;
			break;
		case OpType::BinEqual:
		case OpType::BinNotEqual:
			return 3;
			break;
		case OpType::BinAnd:
			return 2;
			break;
		case OpType::BinOr:
			return 1;
			break;
	}

	return 0;
}

OpType TokenToOpType(Token t) {
	switch(t.type) {
		case Token::Type::Less: return OpType::BinLess;
		case Token::Type::LessOrEqual: return OpType::BinLessOrEqual;
		case Token::Type::Equal: return OpType::BinEqual;
		case Token::Type::NotEqual: return OpType::BinNotEqual;
		case Token::Type::Greater: return OpType::BinGreater;
		case Token::Type::GreaterOrEqual: return OpType::BinGreaterOrEqual;
		case Token::Type::Plus: return OpType::BinAdd;
		case Token::Type::Minus: return OpType::BinSub;
		case Token::Type::Asterisk: return OpType::BinMul;
		case Token::Type::Slash: return OpType::BinDiv;
		case Token::Type::Modulo: return OpType::BinMod;
		case Token::Type::Not: return OpType::UnNot;
		case Token::Type::And: return OpType::BinAnd;
		case Token::Type::Or: return OpType::BinOr;
	}
	return OpType::Nop;
}

bool IsOperatorTokenType(Token::Type t) {
	switch(t) {
		case Token::Type::Less: // <
		case Token::Type::LessOrEqual: // <=
		case Token::Type::Equal: // ==
		case Token::Type::NotEqual: // !=
		case Token::Type::Greater: // >
		case Token::Type::GreaterOrEqual: // >=
		case Token::Type::Minus: // -
		case Token::Type::Plus: // +
		case Token::Type::Asterisk: // *
		case Token::Type::Slash: // /
		case Token::Type::Modulo: // %
		case Token::Type::Not: // !
		case Token::Type::And: // &
		case Token::Type::Or: // |
			return true;
			break;
	}
	return false;
}


// [Expr] - Математические и логические выражения.

/* Вход: 1 + 2 * 3
** Выход: (type, value)
** [Const, 1][Const, 2][Const, 3][Operator, BinMul][Operator, BinAdd]
*/

std::vector<Node> Parser::Expr(uint8_t lp) { // версия 3
	Serial.println("Expr");
	std::vector<Node> expr_buf;
	std::vector<Node> nstack;
	NodeValueUnion val;
	Token t;
	Token last_t = Token::Type::UndefToken;
	Node n1;
	char *name;
	int8_t round_br = 0; // проверка количества круглых скобок
	int8_t square_br = 0; // и квадратных
	bool nnd = 1;
	while (nnd) {
		Serial.print("[expr]");
		Serial.println(Token_to_string(current_token.type));
		t = current_token;
		// Целое число
		if (t.type == Token::Type::Int) {
			val.i = t.value.i;
			expr_buf.push_back(n1);
			expr_buf.back().type = (uint8_t)NodeType::ConstInt;
			expr_buf.back().value = val;
			Next(Token::Type::Int);
		}
		// Вещественное число
		else if (t.type == Token::Type::Float) {
			val.f = t.value.f;
			expr_buf.push_back(n1);
			expr_buf.back().type = (uint8_t)NodeType::ConstFloat;
			expr_buf.back().value = val;
			Next(Token::Type::Float);
		}
		// Переменная или функция
		else if (t.type == Token::Type::Identifier) {
			val.c = t.value.c;
			Next(Token::Type::Identifier);
			if (current_token.type == Token::Type::LeftParen) { 
				// если есть круглая скобка, то это функция
				nstack.push_back(n1);
				nstack.back().type = (uint8_t)NodeType::Function;
				nstack.back().value = val;
			} else if (current_token.type == Token::Type::LeftSquare) { 
				// если есть квадратная скобка, то это массив
				nstack.push_back(n1);
				nstack.back().type = (uint8_t)NodeType::Array;
				nstack.back().value = val;
			} else {
				// переменная
				expr_buf.push_back(n1);
				expr_buf.back().type = (uint8_t)NodeType::Var;
				expr_buf.back().value = val;
			}
		}
		// Оператор
		else if (IsOperatorTokenType(t.type)) {
			if (IsOperatorTokenType(last_t.type) || // если предыдущий токен был оператором
				last_t.type == Token::Type::UndefToken || // или если перед '-', '+', '!' ничего нет или это была скобка или запятая
				last_t.type == Token::Type::LeftParen || // то
				last_t.type == Token::Type::LeftSquare ||
				last_t.type == Token::Type::Comma) { // это считается унарным оператором (1+-2; -3+5; --1; ...)
				if (t.type == Token::Type::Minus) {
					val.op = OpType::UnMinus;
				} else if (t.type == Token::Type::Plus) {
					val.op = OpType::UnPlus;
				} else if (t.type == Token::Type::Not) {
					val.op = OpType::UnNot;
				} else {
					Serial.print("Error: unary operator");
					Serial.print("'");
					Serial.print(Token_to_string(t.type));
					Serial.println("'");
					Exception("");
				}
			} else { // иначе это бинарный оператор
				if (last_t.type == Token::Type::Int || // число, переменная, правая скобка
				last_t.type == Token::Type::Float || //
				last_t.type == Token::Type::Identifier ||
				last_t.type == Token::Type::RightParen ||
				last_t.type == Token::Type::RightSquare) {
					val.op = TokenToOpType(t);
				} else { // если оказалось что-то вроде *2+2, то вылезет эта ошибка
					Serial.print("Error: binary operator");
					Serial.print("'");
					Serial.print(Token_to_string(t.type));
					Serial.println("'");
					Exception("");
				}
			}
			// цикл перемещает из стека в строку всё до запятой, функции, массива, или оператора с большим приоритетом 
			while (
				!nstack.empty() &&  // стек не пустой
				(NodeType)nstack.back().type == NodeType::Operator && // последний в стеке - оператор
				(GetPriorityLevel(val.op) <= GetPriorityLevel(nstack.back().value.op) || // по приоритету
				(NodeType)nstack.back().type == NodeType::Function || // или последний в стеке - функция
				(NodeType)nstack.back().type == NodeType::Array) // или последний в стеке - массив
			) {
				expr_buf.push_back(n1);
				expr_buf.back().type = nstack.back().type;
				expr_buf.back().value = nstack.back().value;
				nstack.pop_back();
			}
			nstack.push_back(n1);
			nstack.back().type = (uint8_t)NodeType::Operator;
			nstack.back().value = val;
			Next(t.type);
		}
		else if (t.type == Token::Type::LeftParen) {
			round_br++;
			// левая скобка добавляетсяв стек
			nstack.push_back(n1);
			nstack.back().type = (uint8_t)NodeType::LeftParen;
			nstack.back().value = val;
			Next(Token::Type::LeftParen);
		}
		else if (t.type == Token::Type::RightParen || t.type == Token::Type::Comma) {
			// символ ')' или ','

			// всё до левой скобки переносится в буфер
			while (!nstack.empty() && 
			((NodeType)nstack.back().type != NodeType::LeftParen ||
			(NodeType)nstack.back().type != NodeType::Delimeter)
			) {
				expr_buf.push_back(n1);
				expr_buf.back().type = nstack.back().type;
				expr_buf.back().value = nstack.back().value;
				nstack.pop_back();
				Serial.print(F("nstack.size = ")); // *****************
				Serial.println(nstack.size());
			}
			//nstack.pop_back(); // левая скобка удаляется из стека
			if(t.type == Token::Type::RightParen && round_br == 0 && lp) {
				nnd = 0; // завершение цикла
			} else if (t.type == Token::Type::Comma) {
				nstack.push_back(n1);
				nstack.back().type = (uint8_t)NodeType::Delimeter;
				nstack.back().value = val;
				Next(Token::Type::Comma);
			}
			else {
				if (t.type == Token::Type::RightParen && round_br <= 0) {; // если не было открывающей скобки
					Exception("Error: symbol '(' expected");
				}
				round_br--;
				Next(Token::Type::RightParen);
			}
		}
		else if (t.type == Token::Type::LeftSquare) {
			square_br++;
			// левая скобка добавляется в стек
			nstack.push_back(n1);
			nstack.back().type = (uint8_t)NodeType::LeftSquare;
			nstack.back().value = val;
			Next(Token::Type::LeftSquare);
		}
		else if (t.type == Token::Type::RightSquare) {
			// символ ']'
			if (square_br <= 0) {; // если не было открывающей скобки
				Exception("Error: symbol '[' expected");
			}
			// всё до левой скобки переносится в буфер
			while (!nstack.empty() && (NodeType)nstack.back().type != NodeType::LeftSquare) {
				expr_buf.push_back(n1);
				expr_buf.back().type = nstack.back().type;
				expr_buf.back().value = nstack.back().value;
				nstack.pop_back();
			}
			//nstack.pop_back(); // левая скобка удаляется из стека
			square_br--;
			Next(Token::Type::RightSquare);
		}
		else if (t.type == Token::Type::Comma) {
			// Запятая в вызове функции
			round_br--;
			// всё до левой скобки или запятой переносится в буфер
			while (!nstack.empty() && 
			(NodeType)nstack.back().type != NodeType::LeftParen &&
			(NodeType)nstack.back().type != NodeType::Delimeter) {
				expr_buf.push_back(n1);
				expr_buf.back().type = nstack.back().type;
				expr_buf.back().value = nstack.back().value;
				nstack.pop_back();
			}
			nstack.pop_back(); // левая скобка удаляется из стека
			Next(Token::Type::RightParen);
		}
		else { // выражение закончилось
			nnd = 0; // завершение цикла
		}
		last_t = t;
	}
	if (round_br != 0) { // проверка круглых скобочек
		if (round_br > 0) {
			Exception("Error: symbol ')' expected");
		}
		else if (round_br < 0) {
			Exception("Error: Extra symbol '('");
		}
	}
	if (square_br != 0) { // проверка квадратных скобочек
		if (square_br > 0) {
			Exception("Error: symbol ']' expected");
		}
		else if (square_br < 0) {
			Exception("Error: Extra symbol '['");
		}
	}
	while (!nstack.empty()) {
		expr_buf.push_back(n1);
		expr_buf.back().type = nstack.back().type;
		expr_buf.back().value = nstack.back().value;
		nstack.pop_back();
	}
	//nstack.clear(); // удалить nstack
	return expr_buf;
}

std::vector<Node> Parser::ConstExpr(uint8_t lp) {
	Serial.println("Expr");
	std::vector<Node> expr_buf;
	std::vector<Node> nstack;
	Token t;
	return expr_buf;
}

void Parser::Parse() {
	try {
		Parser::Program();
		if (Parser::current_token.type != Token::Type::End){
			Exception("EOF expected");
		}
	} catch (std::string s) {
		Serial.println(s.c_str());
		Terminated = true;
	}
}

uint8_t* Parser::GetCode() {
	return code->GetCode();
}

num_t* Parser::GetConsts() {
	return code->GetConsts();
}

size_t Parser::GetCodeLength() {
	return code->GetCodeLength();
}

size_t Parser::GetConstsCount() {
	return code->GetConstsCount();
}

