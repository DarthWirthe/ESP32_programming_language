
#include "scr_parser.h"
#include "HardwareSerial.h"

ExprBuffer* expr = new(ExprBuffer); // буфер
node_stack nstack;

Parser::Parser(){}

Parser::Parser( Lexer _lexer_obj )
{
	Parser::lexer_obj = _lexer_obj;
	Parser::current_token = Parser::lexer_obj.Next();
	coder_obj.Init();
}

void Parser::Exception() { // прерывает сборку
	throw "";
}

void Parser::Next( Token::Type t )
{
	if ( current_token.type == t ) {
		current_token = lexer_obj.Next();
	}
	else {
		Serial.print("Error: Expected '");
		Serial.print(Token_to_string(t));
		Serial.print("', got '");
		Serial.print(Token_to_string(current_token.type));
		Serial.println("'");
		throw "";
	}
}

// если токен это переменная, число, минус, плюс или '!', то это начало выражения

bool IsExprTokenType(Token::Type t)	{
	switch(t) {
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

// [Program]

void Parser::Program()
{
	Serial.println("Program");
	uint8_t *fpt = coder_obj.CodeMainScope();
	StatementList();
	coder_obj.CodeMainScopeEnd(fpt);
}

// [StatementList] - Несколько операторов.

void Parser::StatementList()
{
	Serial.println("StatementList");
	while (current_token.type != Token::Type::RightCurly && current_token.type != Token::Type::End)
	{
		if (current_token.type == Token::Type::Semicolon)
			Next(Token::Type::Semicolon);
		else if (current_token.type == Token::Type::NewLine)
			Next(Token::Type::NewLine);
		else
			Statement();
	}
}

// [Statement] - Один оператор.

void Parser::Statement()
{
	Serial.println("Statement");
	char *n;
	Token::Type type = current_token.type;
	switch(type){
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
			IfStatement(0);
			break;
		case Token::Type::While:
			WhileStatement();
			break;
		case Token::Type::Return:
			Next(Token::Type::Return);
			if (IsExprTokenType(current_token.type)) {
				Expr(0);
				coder_obj.CodeExpression(expr);
				coder_obj.CodeReturnOperator(var_type::Int);
			} else {
				coder_obj.CodeReturnOperator(var_type::None);
			}
			break;
		case Token::Type::End:
			Serial.println("The End!");
			break;
		default:
			Serial.print("Error: Unknown statement: ");
			Serial.println(Token_to_string(type));
			throw "";
			break;
	}
}

// [FunctionDeclaration] - Объявление функции.
// прим.: function func(arg1, arg2){ return (arg1 + arg2) / 2 }

void Parser::FunctionDeclaration() {
	Serial.println("FunctionDeclaration");
	Next(Token::Type::Function);
	uint8_t args = 0;
	char *name = current_token.value.c;
	Next(Token::Type::Identifier);
	uint8_t *func_pt = coder_obj.CodeStartFunction(name);
	Next(Token::Type::LeftParen); // левая круглая скобка
	while (current_token.type != Token::Type::RightParen) {
		if (current_token.type == Token::Type::Identifier) {
			Serial.println("FunctionArgument");
			coder_obj.CodeLocalDeclare(current_token.value.c, var_type::Int);
			Next(Token::Type::Identifier);
			args++;
		}
		if (current_token.type == Token::Type::Comma)
			Next(Token::Type::Comma);
		else if (current_token.type == Token::Type::RightParen)
			{}
		else
			{Serial.println("Error: FunctionDeclaration");Exception();}
	}
	Next(Token::Type::RightParen); // правая круглая скобка
	coder_obj.SetArgs(name, args);
	Next(Token::Type::LeftCurly); // левая фигурная скобка
	StatementList();
	Next(Token::Type::RightCurly); // правая фигурная скобка
	coder_obj.CodeCloseFunction(func_pt);
}

// [FunctionCall] - Вызов функции.

void Parser::FunctionCall(char *name) {
	Serial.println("FunctionCall");
	Next(Token::Type::LeftParen);
	while (current_token.type != Token::Type::RightParen) {
		if (IsExprTokenType(current_token.type)) {
			Serial.println("CallParameter");
			Expr(1);
			coder_obj.CodeExpression(expr);
		}
		if (current_token.type == Token::Type::Comma)
			Next(Token::Type::Comma);
		else if (current_token.type == Token::Type::RightParen)
			{}
		else
			{Serial.println("Error: FunctionCall");Exception();}
	}
	Next(Token::Type::RightParen);
	coder_obj.CodeCallFunction(name, -1); // -1 - аргументы
}

// [Assignment] - Оператор присваивания.

void Parser::Assignment(char *name) {
	Serial.println("Assignment");
	Parser::Next(Token::Type::Assign);
	Expr(0);
	coder_obj.CodeExpression(expr);
	coder_obj.CodeLocalStore(name);
}

// [ArrayDeclaration] - Объявление массива
// прим.: array[5]{2,6,3,1,4}; arr[2];

void Parser::ArrayDeclaration(char *name) {
	Serial.println("ArrayDeclaration");
	bool UnknownLength = true;
	int num = 0;
	Parser::Next(Token::Type::LeftSquare);
	if (current_token.type != Token::Type::RightSquare) {
		Expr(0);
		coder_obj.CodeExpression(expr);
		UnknownLength = false;
	}
	Next(Token::Type::RightSquare);
	coder_obj.CodeArrayDeclaration(name);
	if (current_token.type == Token::Type::LeftCurly) {
		Next(Token::Type::LeftCurly);
		while (current_token.type != Token::Type::RightCurly) {
			if (IsExprTokenType(current_token.type)) {
				Serial.println("ArrayParameter");
				Expr(0);
				coder_obj.CodeExpression(expr);
			}
			else if (current_token.type == Token::Type::Comma)
				Next(Token::Type::Comma);
			else if (current_token.type == Token::Type::RightCurly)
				{}
			else
				{Serial.println("Error: ArrayDeclaration");}
		}
		Next(Token::Type::RightCurly);
	}
}

// [IfStatement] - Оператор условия.
// прим.: if(a){...} elseif(b){...} else{...}

void Parser::IfStatement(uint8_t *ifs_pt) {
	Serial.println("IfStatement");
	Next(Token::Type::If);
	Next(Token::Type::LeftParen);
	Expr(0);
	coder_obj.CodeExpression(expr);
	Next(Token::Type::RightParen); // правая круглая скобка
	ifs_pt = coder_obj.CodeStartIfStatement();
	Next(Token::Type::LeftCurly); // левая фигурная скобка
	StatementList();
	Next(Token::Type::RightCurly); // правая фигурная скобка
	if (current_token.type == Token::Type::ElseIf) {
		coder_obj.CodeCloseIfStatement(ifs_pt);
		IfStatement(ifs_pt);
	}
	else if (current_token.type == Token::Type::Else)
		ElseStatement(ifs_pt);
	else
		coder_obj.CodeCloseIfStatement(ifs_pt);
}

void Parser::ElseStatement(uint8_t *ifs_pt) {
	Serial.println("ElseStatement");
	Next(Token::Type::Else);
	Next(Token::Type::LeftCurly); // левая фигурная скобка
	StatementList();
	Next(Token::Type::RightCurly); // правая фигурная скобка
	coder_obj.CodeCloseIfStatement(ifs_pt);
}

// [WhileStatement] - Цикл с предусловием.
// прим.: while(a){...}

void Parser::WhileStatement() {
	Serial.println("WhileStatement");
	Next(Token::Type::While);
	Next(Token::Type::LeftParen);
	uint8_t *exstart = coder_obj.rcode->pointer;
	Expr(1);
	coder_obj.CodeExpression(expr);
	Next(Token::Type::RightParen); // правая круглая скобка
	uint8_t *ws_pt = coder_obj.CodeStartWhileStatement();
	Next(Token::Type::LeftCurly); // левая фигурная скобка
	StatementList();
	Next(Token::Type::RightCurly); // правая фигурная скобка
	coder_obj.CodeCloseWhileStatement(ws_pt, exstart);
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

/* --###--> [Expr] - математические и логические выражения <--###-- */

void TPush(node n) {
	expr->buffer[expr->count] = n;
	expr->count++;
}

void node_stack::Push(node n) {
	node_stack[++nst_pt] = n;
}

node node_stack::Pop() {
	return node_stack[nst_pt--];
}

node node_stack::Top() {
	return node_stack[nst_pt];
}

/* Вход: 1 + 2 * 3
** Выход: (type, value)
** [Const, 1][Const, 2][Const, 3][Operator, BinMul][Operator, BinAdd]
*/

// void Parser::ExprCheck() {
	
// }

void Parser::Expr(uint8_t lp){
	// в.2.1
	Serial.println("Expr");
	expr->count = 0;
	node_stack nstack;
	Token t;
	Token last_t = Token::Type::UndefToken;
	node n1;
	char *name;
	int8_t round_br = 0; // проверка количества круглых скобок
	int8_t square_br = 0; // и квадратных
	bool nnd = 1;
	while (nnd) {
		t = current_token;
		// Целое число
		if (t.type == Token::Type::Int) {
			n1.type = (uint8_t)NodeType::ConstInt;
			n1.value.i = t.value.i;
			TPush(n1);
			Next(Token::Type::Int);
		}
		// Вещественное число
		else if (t.type == Token::Type::Float) {
			n1.type = (uint8_t)NodeType::ConstFloat;
			n1.value.f = t.value.f;
			TPush(n1);
			Next(Token::Type::Float);
		}
		// Переменная или функция
		else if (t.type == Token::Type::Identifier) {
			name = t.value.c;
			Next(Token::Type::Identifier);
			if (current_token.type == Token::Type::LeftParen) { 
				// если есть круглая скобка, то это функция
				n1.type = (uint8_t)NodeType::Function;
				n1.value.c = name;
				nstack.Push(n1); // добавляет в стек
			}
			else if (current_token.type == Token::Type::LeftSquare) { 
				// если есть квадратная скобка, то это массив
				n1.type = (uint8_t)NodeType::Array;
				n1.value.c = name;
				nstack.Push(n1);
			} else {
				n1.type = (uint8_t)NodeType::Var;
				n1.value.c = name;
				TPush(n1);
			}
		}
		// Оператор
		else if (IsOperatorTokenType(t.type)) {
			n1.type = (uint8_t)NodeType::Operator;
			if (IsOperatorTokenType(last_t.type) || // если предыдущий токен был оператором
				last_t.type == Token::Type::UndefToken || // или если перед '-', '+', '!' ничего нет или это была скобка или запятая
				last_t.type == Token::Type::LeftParen || // то
				last_t.type == Token::Type::LeftSquare ||
				last_t.type == Token::Type::Comma) { // это считается унарным оператором (1+-2; -3+5; --1; ...)
				if (t.type == Token::Type::Minus) {
					n1.value.op = OpType::UnMinus;
				} else if (t.type == Token::Type::Plus) {
					n1.value.op = OpType::UnPlus;
				} else if (t.type == Token::Type::Not) {
					n1.value.op = OpType::UnNot;
				} else {
					Serial.print("Error: unary operator");
					Serial.print("'");
					Serial.print(Token_to_string(t.type));
					Serial.println("'");
					Exception();
				}
			} else { // иначе это бинарный оператор
				if (last_t.type == Token::Type::Int || // число, переменная, правая скобка
				last_t.type == Token::Type::Float || //
				last_t.type == Token::Type::Identifier ||
				last_t.type == Token::Type::RightParen ||
				last_t.type == Token::Type::RightSquare) {
					n1.value.op = TokenToOpType(t);
				} else { // если оказалось что-то вроде *2+2, то вылезет эта ошибка
					Serial.print("Error: binary operator");
					Serial.print("'");
					Serial.print(Token_to_string(t.type));
					Serial.println("'");
					Exception();
				}
			}
			// цикл перемещает из стека в строку всё до запятой, функции, массива, или оператора с большим приоритетом 
			while (
				nstack.nst_pt >= 0 &&  // стек не пустой
				(NodeType)nstack.Top().type == NodeType::Operator && // последний в стеке - оператор
				(GetPriorityLevel(n1.value.op) <= GetPriorityLevel(nstack.Top().value.op) || // по приоритету
				(NodeType)nstack.Top().type == NodeType::Function || // или последний в стеке - функция
				(NodeType)nstack.Top().type == NodeType::Array) // или последний в стеке - массив
			) {
				TPush(nstack.Pop());
			}
			nstack.Push(n1);
			Next(t.type);
		}
		else if (t.type == Token::Type::LeftParen) {
			round_br++;
			n1.type = (uint8_t)NodeType::LeftParen;
			nstack.Push(n1); // левая скобка добавляетсяв стек
			Next(Token::Type::LeftParen);
		}
		else if (t.type == Token::Type::RightParen || t.type == Token::Type::Comma) {
			// символ ')' или ','

			// всё до левой скобки переносится в буфер
			while (nstack.nst_pt >= 0 && 
			((NodeType)nstack.Top().type != NodeType::LeftParen ||
			(NodeType)nstack.Top().type != NodeType::Delimeter)
			) {
				TPush(nstack.Pop());
			}
			nstack.Pop(); // левая скобка удаляется из стека
			if(t.type == Token::Type::RightParen && round_br == 0 && lp) {
				nnd = 0; // завершение цикла
			} else if (t.type == Token::Type::Comma) {
				n1.type = (uint8_t)NodeType::Delimeter;
				nstack.Push(n1);
				Next(Token::Type::Comma);
			}
			else {
				if (t.type == Token::Type::RightParen && round_br <= 0) {; // если не было открывающей скобки
					Serial.println("Error: symbol '(' expected");
					Exception();
				}
				round_br--;
				Next(Token::Type::RightParen);
			}
		}
		else if (t.type == Token::Type::LeftSquare) {
			square_br++;
			n1.type = (uint8_t)NodeType::LeftSquare;
			nstack.Push(n1); // левая скобка добавляется в стек
			Next(Token::Type::LeftSquare);
		}
		else if (t.type == Token::Type::RightSquare) {
			// символ ']'
			if (square_br <= 0) {; // если не было открывающей скобки
				Serial.println("Error: symbol '[' expected");
				Exception();
			}
			// всё до левой скобки переносится в буфер
			while (nstack.nst_pt >= 0 && (NodeType)nstack.Top().type != NodeType::LeftSquare) {
				TPush(nstack.Pop());
			}
			nstack.Pop(); // левая скобка удаляется из стека
			square_br--;
			Next(Token::Type::RightSquare);
		}
		else if (t.type == Token::Type::Comma) {
			// Запятая в вызове функции
			round_br--;
			// всё до левой скобки или запятой переносится в буфер
			while (nstack.nst_pt >= 0 && 
			(NodeType)nstack.Top().type != NodeType::LeftParen &&
			(NodeType)nstack.Top().type != NodeType::Delimeter) {
				TPush(nstack.Pop());
			}
			nstack.Pop(); // левая скобка удаляется из стека
			Next(Token::Type::RightParen);
		}
		else { // выражение закончилось
			nnd = 0; // завершение цикла
		}
		last_t = t;
	}
	if (round_br != 0) { // проверка круглых скобочек
		if (round_br > 0) {
			Serial.println("Error: symbol ')' expected");
			Exception();
		}
		else if (round_br < 0) {
			Serial.println("Error: Extra symbol '('");
			Exception();
		}
	}
	if (square_br != 0) { // проверка квадратных скобочек
		if (square_br > 0) {
			Serial.println("Error: symbol ']' expected");
			Exception();
		}
		else if (square_br < 0) {
			Serial.println("Error: Extra symbol '['");
			Exception();
		}
	}
	while (nstack.nst_pt >= 0)
		TPush(nstack.Pop());
}

void Parser::Parse() {
	try {
		Parser::Program();
		if (Parser::current_token.type != Token::Type::End){
			throw "EOF expected";
		}
		//coder_obj.DeleteTemp();
	} catch (const char *msg) {
		Serial.println(msg);
	}
}

uint8_t* Parser::GetCode() {
	return coder_obj.GetCode();
}

num_t* Parser::GetConsts() {
	return coder_obj.GetConsts();
}

int Parser::GetConstsCount() {
	return coder_obj.GetConstsCount();
}


