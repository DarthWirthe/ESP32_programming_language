/* scr_parser.cpp
**
*/

#include "scr_parser.h"
#include "utils/scr_debug.h"
#include "HardwareSerial.h"

Parser::Parser(){}

Parser::Parser(Lexer *_lexer_class) {
	lex = _lexer_class;
	code = new Code();
	current_token = lex->Next();
	exit_code = 1;
}

// Сообщение об ошибке
void Parser::InternalException(int code) {
	throw code;
}

void Parser::ErrorExpectedToken(Token::Type expected, Token::Type got) {
	PRINTF("Error: Expected '%.32s', got '%.32s'", Token_to_string(expected), Token_to_string(got));
	InternalException(1);
}

void Parser::ErrorExpectedExpr() {
	PRINTF("Error: Expected expression");
	InternalException(1);
}

void Parser::ErrorExpectedSymbol(const char *c) {
	PRINTF("Error: Expected symbol '%.12s'", c);
	InternalException(1);
}

void Parser::ErrorExtraSymbol(const char *c) {
	PRINTF("Error: Extra symbol '%.12s'", c);
	InternalException(1);
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
	while (current_token.type == Token::Type::NewLine)
		Next(Token::Type::NewLine);
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
	if (t.type == Token::Type::Const)
		return true;
	if (t.type == Token::Type::Identifier) {
		for (int i = 0; i < sizeof(var_types_list) / sizeof(var_types_list[0]); i++) {
			if (Compare_strings(t.value.c, var_types_list[i].name)) {
				return true;
			}
		}
	}
	return false;
}

// Строка -> тип переменной

VarType FindVarType(char *t, bool IsConst) {
	char *temp;
	if (IsConst) {
		temp = S_concat((char*)"const ", t);
		delete[] t; // строка больше не нужна
	}
	else
		temp = t;
	for (int i = 0; i < sizeof(var_types_list) / sizeof(var_types_list[0]); i++) {
		if (Compare_strings(t, var_types_list[i].name)) {
			delete[] temp; // строка больше не нужна
			return var_types_list[i].type;
		}
	}
}

// [Program] - Первичное окружение

void Parser::Program() {
	Serial.println(F("Program"));
	int ms = code->MainScope();
	StatementList();
	code->MainScopeEnd(ms);
}

// [StatementList] - Несколько операторов.

void Parser::StatementList() {
	Serial.println(F("StatementList"));
	while (current_token.type != Token::Type::RightCurly &&
		current_token.type != Token::Type::EndOfFile)
	{
		if (current_token.type == Token::Type::Semicolon)
			Next(Token::Type::Semicolon);
		else
			Statement();
	}
}

// [Statement] - Один оператор.

void Parser::Statement() {
	Serial.println(F("Statement"));
	char *n;
	VarType t;
	Token::Type type = current_token.type;
	if (IsVarTypeToken(current_token)) { // объявление переменной
		t = VariableType();
		n = current_token.value.c;
		Next(Token::Type::Identifier);
		Serial.println(Token_to_string(current_token.type));
		if (current_token.type == Token::Type::LeftSquare)
			ArrayDeclaration(n, t);
		else
			Declaration(n, t);
	} else {
		switch (type) {
			case Token::Type::Identifier:
				n = current_token.value.c;
				Next(Token::Type::Identifier);
				if (current_token.type == Token::Type::LeftParen)
					FunctionCall(n); // вызов функции
				else
					Assignment(n); // присваивание
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
			case Token::Type::RightCurly:
				ErrorExpectedSymbol("{");
			break;
			case Token::Type::NewLine:
				NewLine();
			break;
			case Token::Type::EndOfFile:
				PRINTF("The End!");
			break;
			default:
				PRINTF("Error: Unknown statement '%s'", Token_to_string(type));
				InternalException(1);
			break;
		}
	}
}

/* [Assignment] - Оператор присваивания
** прим.: a = 2 * (b - 1);
** a[0] = a + 1;
*/

void Parser::Assignment(char *name) {
	Serial.println(F("Assignment"));
	if (current_token.type == Token::Type::LeftSquare) { 
		// присвоение элементу массива
		Next(Token::Type::LeftSquare); // скобка '['
		std::vector<Node> pos;
		if (IsExprTokenType(current_token.type))
			pos = Expr(2);
		else
			ErrorExpectedExpr();
		Next(Token::Type::RightSquare); // скобка ']'
		Next(Token::Type::Assign); // '='
		if (IsExprTokenType(current_token.type))
			code->Expression(Expr(0)); // выражение между скобок []
		else
			ErrorExpectedExpr();
		code->Expression(pos, 2);
		code->StoreArray(name);
	} else {
		Next(Token::Type::Assign); // '='
		if (IsExprTokenType(current_token.type))
			code->Expression(Expr(0)); // выражение справа
		else
			ErrorExpectedExpr(); 
		code->StoreLocal(name); // переменная слева
	}
}

/* [VariableType] - Тип переменной
*/

VarType Parser::VariableType() {
	bool IsConst = false;
	if (current_token.type == Token::Type::Const) {
		IsConst = true;
		Next(Token::Type::Const);
	}
	VarType t = FindVarType(current_token.value.c, IsConst);
	Next(current_token.type);
	return t;
}

/* [Declaration] - Оператор объявления переменной
** прим.: int v = 1
** множественное объявление: int a = 1, b, c = 2, d = 5, e, f
*/

void Parser::Declaration(char *name, VarType t) {
	Serial.println(F("Declaration"));
	bool assignment, first = true;
	while (first || current_token.type == Token::Type::Comma) {
		if (!first) {
			Next(Token::Type::Comma); // ','
			NewLine(); // Возможен перенос строки
			name = current_token.value.c;
			Next(Token::Type::Identifier);
		}
		first = false;
		assignment = false;
		// если есть '=', то должно быть выражение
		if (current_token.type == Token::Type::Assign) {
			Next(Token::Type::Assign); // '='
			if (IsExprTokenType(current_token.type))
				code->Expression(Expr(0)); // выражение справа
			else
				ErrorExpectedExpr(); 
			assignment = true;
		}
		code->DeclareLocal(name, t);
		if (assignment) {
			code->StoreLocal(name); // переменная слева
		}
	}
}

/* [FunctionArgument] - Аргумент при объявлении функции
** прим.: function func(int arg1, int arg2)
*/

std::vector<func_arg_struct> Parser::FunctionArgs() {
	Serial.println(F("FunctionArgs"));
	func_arg_struct fa;
	std::vector<func_arg_struct> args_v;
	char *name;
	VarType t;
	while (current_token.type != Token::Type::RightParen) {
		NewLine(); // Возможен перенос строки
		if (IsVarTypeToken(current_token)) {
			t = VariableType();
			name = current_token.value.c;
			Next(Token::Type::Identifier);
			code->DeclareLocal(name, t);
			fa.type = (uint8_t)t;
			args_v.push_back(fa);
		}
		if (current_token.type == Token::Type::Comma) // все запятые
			Next(Token::Type::Comma);
		else if (current_token.type == Token::Type::RightParen) // ')'
			{}
		else
			ErrorExpectedToken(Token::Type::RightParen, current_token.type);
	}
	return args_v;
}

/* [FunctionDeclaration] - Объявление функции
** прим.: function func(int a1, int a2){ return (arg1 + arg2) / 2 }
*/

void Parser::FunctionDeclaration() {
	Serial.println(F("FunctionDeclaration"));
	char *name;
	size_t pt;
	Next(Token::Type::Function); // 'function'
	name = current_token.value.c;
	Next(Token::Type::Identifier);
	pt = code->FunctionDeclare(name); // 
	Next(Token::Type::LeftParen); // левая круглая скобка
	code->SetArgs(name, FunctionArgs()); // аргументы
	Next(Token::Type::RightParen); // правая круглая скобка
	NewLine(); // может быть перенос строки
	Next(Token::Type::LeftCurly); // левая фигурная скобка
	StatementList(); // всякие операторы между скобок
	Next(Token::Type::RightCurly); // правая фигурная скобка
	code->FunctionClose(name, pt);
}

/* [FunctionCall] - Вызов функции
** прим.: f(arg1, arg2)
*/

void Parser::FunctionCall(char *name) {
	Serial.println(F("FunctionCall"));
	uint8_t args = 0;
	Next(Token::Type::LeftParen);
	while (current_token.type != Token::Type::RightParen) {
		if (IsExprTokenType(current_token.type)) {
			NewLine(); // Возможен перенос строки
			Serial.println("CallParameter");
			code->Expression(Expr(1));
			args++;
			NewLine(); // Возможен перенос строки
		}
		if (current_token.type == Token::Type::Comma) // ','
			Next(Token::Type::Comma);
		else if (current_token.type == Token::Type::RightParen)
			{}
		else
			ErrorExpectedToken(Token::Type::RightParen, current_token.type);
	}
	Next(Token::Type::RightParen);
	code->CallFunction(name, args);
}

/* [ReturnStatement] - Возврат значения
** прим.: return a+b
*/

void Parser::ReturnStatement() {
	Next(Token::Type::Return);
	// если есть выражение, то возвращает значение
	if (IsExprTokenType(current_token.type)) {
		code->Expression(Expr(0));
		code->ReturnOperator(VarType::Int);
	} 
	// иначе не возвращает значение
	else 
	{
		code->ReturnOperator(VarType::None);
	}
}

/* [IfStatement] - Оператор условия
** прим.: if(a){...} elseif(b){...} else{...}
*/

void Parser::IfStatement() {
	Serial.println(F("IfStatement"));
	size_t ifs_pt;
	std::vector<size_t> labels;
	Next(Token::Type::If);
	code->Expression(Expr(0)); // условие
	ifs_pt = code->StartIfStatement();
	NewLine(); // Возможен перенос строки
	Next(Token::Type::LeftCurly); // левая фигурная скобка
	StatementList(); // разные операторы между скобок
	Next(Token::Type::RightCurly); // правая фигурная скобка
	while (current_token.type == Token::Type::ElseIf) {
		labels.push_back(code->ElseIfStatement());
		code->CloseIfBranch(ifs_pt);
		Next(Token::Type::ElseIf);
		code->Expression(Expr(0)); // условие
		ifs_pt = code->StartIfStatement();
		NewLine(); // Возможен перенос строки
		Next(Token::Type::LeftCurly); // левая фигурная скобка
		StatementList(); // разные операторы между скобок
		Next(Token::Type::RightCurly); // правая фигурная скобка
		code->CloseIfBranch(ifs_pt);
	}
	if (current_token.type == Token::Type::Else) {
		labels.push_back(code->ElseIfStatement());
		code->CloseIfBranch(ifs_pt);
		Next(Token::Type::Else);
		NewLine(); // Возможен перенос строки
		Next(Token::Type::LeftCurly); // левая фигурная скобка
		StatementList(); // разные операторы между скобок
		Next(Token::Type::RightCurly); // правая фигурная скобка
	} else {
		code->CloseIfBranch(ifs_pt);
	}
	code->CloseIfStatement(labels);
}

/* [WhileStatement] - Цикл с предусловием.
** прим.: while(a){...}
*/

void Parser::WhileStatement() {
	Serial.println("WhileStatement");
	size_t stat, cond;
	Next(Token::Type::While);
	cond = code->GetFuncCodeSize();
	code->Expression(Expr(0));
	stat = code->StartWhileStatement();
	NewLine(); // Возможен перенос строки
	Next(Token::Type::LeftCurly); // левая фигурная скобка
	StatementList();
	Next(Token::Type::RightCurly); // правая фигурная скобка
	code->CloseWhileStatement(stat, cond);
}

/* [ArrayDeclaration] - Объявление массива
** прим.: 
** с известным размером: arr[2];
** с размером из результата выражения: arr[5+a*2];
** с объявлением: array[5]{2,6,3,1,4};
** с неизвестным размером: array[]{5,4,3,2};
*/

void Parser::ArrayDeclaration(char *name, VarType t) {
	Serial.println(F("ArrayDeclaration"));
	bool UnknownLength = true;
	int len = 0;
	size_t pos;
	Parser::Next(Token::Type::LeftSquare);
	if (current_token.type != Token::Type::RightSquare) {
		code->Expression(Expr(2)); // размер массива
		UnknownLength = false;
	}
	Next(Token::Type::RightSquare);
	if (UnknownLength) {
		pos = code->ArrayUnknownLengthStart();
	}
	code->ArrayDeclaration(name, t);
	if (current_token.type == Token::Type::LeftCurly) {
		Next(Token::Type::LeftCurly);
		while (current_token.type != Token::Type::RightCurly) {
			if (IsExprTokenType(current_token.type)) {
				Serial.println(F("ArrayParameter"));
				code->Expression(Expr(0), 2); // позиция, 2 - расширение стека
				code->CodeConstInt(len);
				code->StoreArray(name);
				len++;
			}
			else if (current_token.type == Token::Type::Comma)
				Next(Token::Type::Comma);
			else if (current_token.type == Token::Type::RightCurly)
				{}
			else
				ErrorExpectedToken(Token::Type::RightCurly, current_token.type);
		}
		Next(Token::Type::RightCurly);
	}
	if (UnknownLength) {
		code->ArrayUnknownLengthEnd(pos, len);
	}
}

/* Последовательность идентификаторов через точку
** п.: ident.subident -> [ident][subident]
** first_val - первый ид-тор
*/

char* Parser::ParseIdentSequence(char *first_val) {
	std::vector<char*> str_buf = {first_val};
	char *temp, *result;
	int length = strlen(first_val) + 1;
	while (current_token.type == Token::Type::Dot) {
		Next(Token::Type::Dot);
		temp = current_token.value.c;
		Next(Token::Type::Identifier);
		str_buf.push_back(temp);
		length += strlen(temp) + 1;
	}
	int n = 0;
	for (auto i = str_buf.begin(); i != str_buf.end(); i++) {
		for (int l = 0; l < strlen(*i); i++) {
			result[n] = (*i)[l];
			n++;
		}
		result[n] = '.';
	}
	result[n] = '\0';
	// Список переносится в ноду
	return result;
}

/* Приоритет операторов
** Наименьший приоритет - 1
*/

int GetOperatorPriority(OpType type) {
	switch(type) {
		case OpType::UnPlus: // +
		case OpType::UnMinus: // -
		case OpType::UnNot: // !
		case OpType::UnBitNot: // ~
			return 11;
		case OpType::BinMul: // *
		case OpType::BinDiv: // /
		case OpType::BinMod: // %
			return 10;
		case OpType::BinAdd: // +
		case OpType::BinSub: // -
			return 9;
		case OpType::BinLeftShift: // <<
		case OpType::BinRightShift: // >>
			return 8;
		case OpType::BinLess: // <
		case OpType::BinLessOrEqual: // <=
		case OpType::BinGreater: // >
		case OpType::BinGreaterOrEqual: // >=
			return 7;
		case OpType::BinEqual: // ==
		case OpType::BinNotEqual: // !=
			return 6;
		case OpType::BinBitAnd: // &
			return 5;
		case OpType::BinBitXor: // ^
			return 4;
		case OpType::BinBitOr: // |
			return 3;
		case OpType::BinAnd: // and
			return 2;
		case OpType::BinOr: // or
			return 1;
	}

	return 0;
}

OpType TokenToOpType(Token t) {
	switch(t.type) {
		case Token::Type::Less: return OpType::BinLess; // <
		case Token::Type::LessOrEqual: return OpType::BinLessOrEqual; // <=
		case Token::Type::Equal: return OpType::BinEqual; // ==
		case Token::Type::NotEqual: return OpType::BinNotEqual; // !=
		case Token::Type::Greater: return OpType::BinGreater; // >
		case Token::Type::GreaterOrEqual: return OpType::BinGreaterOrEqual; // >=
		case Token::Type::DoubleLess: return OpType::BinLeftShift; // <<
		case Token::Type::DoubleGreater: return OpType::BinRightShift; // >>
		case Token::Type::Plus: return OpType::BinAdd; // +
		case Token::Type::Minus: return OpType::BinSub; // -
		case Token::Type::Asterisk: return OpType::BinMul; // *
		case Token::Type::Slash: return OpType::BinDiv; // /
		case Token::Type::Modulo: return OpType::BinMod; // %
		case Token::Type::Tilde: return OpType::UnBitNot; // ~
		case Token::Type::Ampersand: return OpType::BinBitAnd; // &
		case Token::Type::Pipe: return OpType::BinBitOr; // |
		case Token::Type::Not: return OpType::UnNot; // !
		case Token::Type::And: return OpType::BinAnd; // and
		case Token::Type::Or: return OpType::BinOr; // or
	}
	return OpType::Nop;
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
	NodeType node_type;
	Token t;
	Token prev_token = Token::Type::UndefToken;
	int round_br = 0, square_br = 0; // количество круглых и квадратных скобок
	bool handle = true;
	while (handle) {
		Serial.print("[expr]");
		Serial.println(Token_to_string(current_token.type));
		t = current_token;
		// Целое число
		if (t.type == Token::Type::Int) {
			val.i = t.value.i;
			expr_buf.emplace_back(NodeType::ConstInt, val);
			Next(Token::Type::Int);
		}
		// Вещественное число
		else if (t.type == Token::Type::Float) {
			val.f = t.value.f;
			expr_buf.emplace_back(NodeType::ConstFloat, val);
			Next(Token::Type::Float);
		}
		// Переменная, вызов функции или взятие элемента массива
		else if (t.type == Token::Type::Identifier) {
			val.c = t.value.c;
			Next(Token::Type::Identifier);
			if (current_token.type == Token::Type::Dot) { // если после имени точка - это префикс
				node_type = NodeType::Prefix;
				expr_buf.emplace_back(node_type, val);
			}
			else if (current_token.type == Token::Type::LeftParen) { // если есть круглая скобка, то это функция
				node_type = NodeType::Function;
				nstack.emplace_back(node_type, val);
			}
			else if (current_token.type == Token::Type::LeftSquare) { // если есть квадратная скобка, то это массив
				node_type = NodeType::Array;
				nstack.emplace_back(node_type, val);
			}
			else { // переменная
				node_type = NodeType::Var;
				expr_buf.emplace_back(node_type, val);
			}
		}
		// Оператор
		else if (TokenToOpType(t.type) != OpType::Nop) {
			if (TokenToOpType(prev_token.type) != OpType::Nop || // если предыдущий токен был оператором
				prev_token.type == Token::Type::UndefToken || // или если перед '-', '+', '!' ничего нет или это была скобка или запятая
				prev_token.type == Token::Type::LeftParen || // то
				prev_token.type == Token::Type::LeftSquare ||
				prev_token.type == Token::Type::Comma) { // это считается унарным оператором (1+-2; -3+5; --1; ...)
				if (t.type == Token::Type::Minus) {
					val.op = OpType::UnMinus;
				} else if (t.type == Token::Type::Plus) {
					val.op = OpType::UnPlus;
				} else if (t.type == Token::Type::Not) {
					val.op = OpType::UnNot;
				} else if (t.type == Token::Type::Tilde) {
					val.op = OpType::UnBitNot;
				} else {	
					PRINTF("Error: unary operator '%s'", Token_to_string(t.type));
					InternalException(1);
				}
			} else { // иначе это бинарный оператор
				if (prev_token.type == Token::Type::Int || // число, переменная, правая скобка
				prev_token.type == Token::Type::Float || //
				prev_token.type == Token::Type::Identifier ||
				prev_token.type == Token::Type::RightParen ||
				prev_token.type == Token::Type::RightSquare) {
					val.op = TokenToOpType(t);
				} else { // если оказалось что-то вроде *2+2, то вылезет эта ошибка
					PRINTF("Error: binary operator '%s'", Token_to_string(t.type));
					InternalException(1);
				}
			}
			// цикл перемещает из стека в строку всё до запятой, функции, массива, или оператора с большим приоритетом 
			while (
				!nstack.empty() &&  // стек не пустой
				((NodeType)nstack.back().type == NodeType::Operator && // последний в стеке - оператор
				(GetOperatorPriority(val.op) <= GetOperatorPriority(nstack.back().value.op)) || // его приоритет выше
				(NodeType)nstack.back().type == NodeType::Function || // или последний в стеке - функция
				(NodeType)nstack.back().type == NodeType::Array) // или последний в стеке - массив
			) {
				expr_buf.emplace_back(nstack.back().type, nstack.back().value);
				nstack.pop_back();
			}
			nstack.emplace_back(NodeType::Operator, val);
			Next(t.type);
		}
		else if (t.type == Token::Type::LeftParen) {
			// левая скобка '(' добавляется в стек
			nstack.emplace_back(NodeType::LeftParen, val);
			Next(Token::Type::LeftParen);
			round_br++;
		}
		else if (t.type == Token::Type::RightParen ) { // правая круглая скобка
			while (!nstack.empty() && 
			(NodeType)nstack.back().type != NodeType::LeftParen
			) {
				expr_buf.emplace_back(nstack.back().type, nstack.back().value);
				nstack.pop_back();
			}
			if (!nstack.empty())
				nstack.pop_back(); // левая скобка удаляется из стека
			if(round_br == 0 && lp == 1) {
				handle = false; // завершение цикла
			} else {
				if (round_br <= 0) { // если не было открывающей скобки
					ErrorExpectedSymbol("(");
				}
				Next(Token::Type::RightParen);
				round_br--;
			}
		}
		else if (t.type == Token::Type::Comma) { // запятая
			// если не было открывающей скобки '('
			if (round_br > 0) {
				// всё до левой скобки переносится в буфер
				while (!nstack.empty() && 
				(NodeType)nstack.back().type != NodeType::LeftParen
				) {
					expr_buf.emplace_back(nstack.back().type, nstack.back().value);
					nstack.pop_back();
				}
				nstack.emplace_back(NodeType::Delimeter, val);
				Next(Token::Type::Comma);
			} else {
				handle = false;
			}
		}
		else if (t.type == Token::Type::LeftSquare) {
			// левая скобка '[' добавляется в стек
			nstack.emplace_back(NodeType::LeftSquare, val);
			Next(Token::Type::LeftSquare);
			square_br++;
		}
		else if (t.type == Token::Type::RightSquare) {
			// правая скобка ']'
			// всё до левой скобки переносится в буфер
			while (!nstack.empty() && (NodeType)nstack.back().type != NodeType::LeftSquare) {
				expr_buf.emplace_back(nstack.back().type, nstack.back().value);
				nstack.pop_back();
			}
			if (!nstack.empty())
				nstack.pop_back(); // левая скобка удаляется из стека
			if(square_br == 0 && lp == 2)
				handle = false; // завершение цикла
			else {
				if (square_br <= 0) { // если не было открывающей скобки
					ErrorExpectedSymbol("[");
				}
				Next(Token::Type::RightSquare);
				square_br--;
			}
		}
		else { // выражение закончилось
			handle = false; // завершение цикла
		}
		prev_token = t;
	}
	if (round_br != 0) { // проверка круглых скобочек
		if (round_br > 0) {
			ErrorExpectedSymbol(")");
		}
		else if (round_br < 0) {
			ErrorExtraSymbol("(");
		}
	}
	if (square_br != 0) { // проверка квадратных скобочек
		if (square_br > 0) {
			ErrorExpectedSymbol("]");
		}
		else if (square_br < 0) {
			ErrorExtraSymbol("[");
		}
	}
	while (!nstack.empty()) {
		expr_buf.emplace_back(nstack.back().type, nstack.back().value);
		nstack.pop_back();
	}
	return expr_buf;
}

int Parser::Parse() {
	try {
		Parser::Program();
		if (Parser::current_token.type != Token::Type::EndOfFile){
			PRINTF("EOF expected");
			InternalException(1);
		}
		exit_code = 0;
	} catch (int _exit_code) {
		PRINTF(" at line %d, column %d\n", lex->line, lex->column);
		exit_code = _exit_code;
	}
	lex->End();
	return exit_code;
}

Code* Parser::Compiler() {
	return code;
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

