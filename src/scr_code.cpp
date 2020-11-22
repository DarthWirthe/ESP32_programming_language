
// Компилятор

#include "scr_code.h"

#include "HardwareSerial.h"

/* /////////////////////////////////////////////////////////////////////////////////// */

bool IsConstType(VarType t) {
	switch(t) {
		case VarType::Function:
		case VarType::ConstInt:
		case VarType::ConstFloat:
			return true;
			break;
	}
	return false;
}

VarState::~VarState() {
	//if (name != nullptr)
	//	delete[] name;
}

func_descr::func_descr(){}

func_descr::func_descr(uint8_t* start, uint8_t* end) {
	start_pt = start;
	end_pt = end;
}

// ######

Code::Code() {
	NewScope((char*)"/x0");
}

Code::~Code() {
	
}

void Code::Add_08(uint8_t n) {
	current_scope->fcode.push_back(n);
}

void Code::Add_16(uint16_t n) {
	current_scope->fcode.push_back((uint8_t)(n & 0xFF));
	current_scope->fcode.push_back((uint8_t)((n >> 8) & 0xFF));
}

// Новое окружение

void Code::NewScope(char *name) {
	Serial.println(F("Code::NewScope"));
	FuncState *new_scope = new(FuncState); // Создает звено
	new_scope->name = name;
	new_scope->locals_count = 0;
	new_scope->locals_start = locals.size();
	new_scope->max_stack = 0;
	new_scope->prev = current_scope;
	current_scope = new_scope;
}

// Возвращает в предыдущее окружение

void Code::CloseScope() {
	Serial.println(F("Code::CloseScope"));
	FuncState *sc;
	if (current_scope != nullptr) {
		//total_locals_count = current_scope->locals_start; // удаляет имена переменных, т.к. они не существуют вне функции
		locals.erase(locals.begin() + current_scope->locals_start, locals.end() - 1);
		if (current_scope->prev != nullptr) {
			Serial.print(F("bytecode.size() = "));
			Serial.println(bytecode.size());
			Serial.print(F("fcode.size() = "));
			Serial.println(current_scope->fcode.size());
			Serial.print(F("locals.size() = "));
			Serial.println(locals.size());
			Serial.print(F("num_consts.size() = "));
			Serial.println(num_consts.size());
			Serial.print(F("max_stack = "));
			Serial.println(current_scope->max_stack);
			// Переносит fcode в bytecode
			for (std::vector<uint8_t>::iterator i = current_scope->fcode.begin();
				i != current_scope->fcode.end(); i++) 
			{
				bytecode.push_back(*i);
			}
			sc = current_scope;
			current_scope = current_scope->prev;
			delete sc; // удаляет звено
		}
	}
}


int Code::AddConstInt(int n) {
	int pos = FindConstInt(n);
	num_t temp;
	if (pos == -1) {
		temp.i = n;
		num_consts.push_back(temp);
		return num_consts.size() - 1;
	}
	return pos;
}

int Code::AddConstFloat(float n) {
	int pos = FindConstFloat(n);
	num_t temp;
	if (pos == -1) {
		temp.f = n;
		num_consts.push_back(temp);
		return num_consts.size() - 1;
	}
	return pos;
}

// Указать количество аргументов

void Code::SetArgs(char *name, uint8_t args) {
	int l = FindLocal(name);
	declared_functions[locals[l].data].args = args;
}

// Узнать количество аргументов

uint8_t Code::GetArgs(char *name) {
	int l = FindLocal(name);
	return declared_functions[locals[l].data].args;
}

int Code::FindLocal(char *name) {
	int n = locals.size();
	for (std::vector<VarState>::reverse_iterator i = locals.rbegin(); i != locals.rend(); i++) {
		n--;
		if (Compare_strings(name, i->name))
			return n;
	}
	return -1;
}


int Code::FindConstInt(int n) {
	for (size_t i = 0; i < num_consts.size(); i++){
		if (num_consts[i].i == n){
			return i;
		}
	}
	return -1;
}

int Code::FindConstFloat(float n) {
	for (size_t i = 0; i < num_consts.size(); i++){
		if (num_consts[i].f == n){
			return i;
		}
	}
	return -1;
}

// Кодирует константу

// Целое число

int Code::CodeConstInt(int n) {
	if (n == 0) {
		Add_08(I_OP_LOAD0);
	} else {
		int pos = AddConstInt(n);
		Add_08(I_OP_LOADICONST);
		Add_08((uint8_t)pos);
		return pos;
	}
}

// Число с плавающей точкой

int Code::CodeConstFloat(float n) {
	int pos = AddConstFloat(n);
	Add_08(I_OP_LOADFCONST);
	Add_08((uint8_t)pos);
	return pos;
}

// Именованная константа, целое число
// имя, значение, тип, (code = true - добавить в код)

int Code::DeclareConstVarInt(char *name, int n, VarType type, bool code) {
	int l = FindLocal(name);
	int pos;
	if (l == -1) { // *если не объявлена
		l = AddLocal(name, type); // добавить переменную
		pos = AddConstInt(n); // значение переменной = номер константы
		locals[l].data = pos;
		if (code) {
			Add_08(I_OP_LOADICONST); // загрузить эту константу в стек
			Add_08((uint8_t)pos);
		}
		return l;
	} else {
		// ошибка: уже есть что-то с таким именем
	}
}


// Добавить локальную переменную в список переменных

int Code::AddLocal(char *name, VarType type) {
	Serial.print(F("AddLocal: "));
	Serial.println(name); // ###
	VarState vs;
	locals.push_back(vs);
	locals.back().name = name;
	locals.back().type = (uint8_t)type;
	locals.back().data = 0;
	locals.back().id = current_scope->locals_count;
	current_scope->locals_count++;
	return locals.size() - 1;
}


// Кодирует переменные в выражениях 

void Code::LocalLoad(char *name) {
	int l = FindLocal(name);
	VarType type;
	if (l == -1) { // *если не объявлена
		Serial.printf("Error: %s was not declared\n", name);
		throw "";
	} else {
		uint8_t id = locals[l].id;
		type = (VarType)locals[l].type;
		if (IsConstType(type)) { // Добавляет ранее объявленную константу
			if (type == VarType::Function)
				Add_08(I_OP_LOADICONST);
				Add_08(declared_functions[locals[l].data].cnst);
			if (type == VarType::ConstInt) {
				Add_08(I_OP_LOADICONST);
				Add_08(locals[l].data);
			}
		} else { // Просто переменная
			Add_08(I_OP_ILOAD);
			Add_08(id);
		}
	}
	if (current_scope->max_stack < 1) { // добавляет место для 1 переменной, если его не хватает
		current_scope->max_stack = 1;
	}
}

// Кодирует запись в переменную

void Code::LocalStore(char *name) {
	Serial.println(F("LocalStore"));
	Serial.print(F("LocalsSize: ")); // *
	Serial.println(locals.size()); // *
	uint8_t id;
	int l = FindLocal(name);
	if (l == -1) { // *если не объявлена
		l = AddLocal(name, VarType::Int);
		Add_08(I_OP_ISTORE);
		Add_08(locals[l].id); 
	} else {
		id = locals[l].id;
		Add_08(I_OP_ISTORE);
		Add_08((uint8_t)id); // 8-битный идентификатор переменной
	}
}

int Code::LocalDeclare(char *name, VarType type) {
	int l = FindLocal(name);
	if (l == -1) { // если не объявлена
		return AddLocal(name, type);
	} else {
		// ошибка
	}
}

void Code::CodeJump(int shift, bool condition, bool expected) {
	if (condition) {
		if (expected)
			Add_08(I_OP_JMPNZ);
		else
			Add_08(I_OP_JMPZ);
	}
	else
		Add_08(I_OP_JMP);
	Add_16(shift);
}


void Code::CodeOperator(OpType t, bool IsFloat) {
	switch(t){
		case OpType::UnPlus:
			if (IsFloat) 
				Add_08(I_OP_FUPLUS); 
			else
				Add_08(I_OP_IUPLUS);
			break;
		case OpType::UnMinus:
			if (IsFloat) 
				Add_08(I_OP_FUNEG); 
			else
				Add_08(I_OP_IUNEG);
			break;
		case OpType::UnNot: Add_08(I_OP_INOT); break;
		case OpType::BinAdd: 
			if (IsFloat) 
				Add_08(I_OP_FADD); 
			else
				Add_08(I_OP_IADD);
			break;
		case OpType::BinSub:
			if (IsFloat) 
				Add_08(I_OP_FSUB); 
			else
				Add_08(I_OP_ISUB);
			break;
		case OpType::BinMul:
			if (IsFloat) 
				Add_08(I_OP_FMUL); 
			else
				Add_08(I_OP_IMUL);
			break;
		case OpType::BinDiv:
			if (IsFloat) 
				Add_08(I_OP_FDIV); 
			else
				Add_08(I_OP_IDIV);
			break;
		case OpType::BinMod:
			if (IsFloat) 
				Add_08(I_OP_FREM); 
			else
				Add_08(I_OP_IREM);
			break;
		case OpType::BinLess: Add_08(I_OP_LESS); break;
		case OpType::BinLessOrEqual: Add_08(I_OP_LESSEQ); break;
		case OpType::BinEqual: Add_08(I_OP_EQ); break;
		case OpType::BinNotEqual: Add_08(I_OP_NOTEQ); break;
		case OpType::BinGreater: Add_08(I_OP_GR); break;
		case OpType::BinGreaterOrEqual: Add_08(I_OP_GREQ); break;
		case OpType::BinAnd: Add_08(I_OP_IAND); break;
		case OpType::BinOr: Add_08(I_OP_IOR); break;
	}
}

bool IsUnaryOperatorType(OpType type) {
	switch(type) {
		case OpType::UnPlus:
		case OpType::UnMinus:
		case OpType::UnNot:
			return true;
	}
	return false;
}

/* Выражение
** a = function(a, b) -> func, a, b, rparen
** CodeExpression -> i = 0 -> CodeExprNode -> type=function -> CodeExprNode ->
** -> a, b, rparen -> rec_nodes += 3
** После вычислений на стеке остнутся результаты, их нужно 
** теоретически посчитать и сложить с предыдущими
*/ 

void Code::Expression(std::vector<Node> expr) {
	Serial.println("CodeExpression:");
	uint8_t max_st = 0;
	bool has_func = false;
	bool is_float_op;
	NodeType last_type2 = NodeType::None;
	NodeType last_type = NodeType::None;
	NodeType type;
	for (int i = 0; i < expr.size(); i++) {
		type = (NodeType)expr[i].type;
		if (type == NodeType::ConstInt) {
			Serial.print(expr[i].value.i);
			CodeConstInt(expr[i].value.i);
			max_st++; // резерв стека увеличивается, чтобы уместить несколько чисел
		}
		else if (type == NodeType::ConstFloat) {
			Serial.print(expr[i].value.f);
			CodeConstFloat(expr[i].value.f);
			max_st++;
		}
		else if (type == NodeType::Var) {
			Serial.print(expr[i].value.c);
			LocalLoad(expr[i].value.c);
			max_st++;
		}
		else if (type == NodeType::Operator) {
			Serial.print("Op");
			is_float_op = false;
			if (last_type == NodeType::ConstFloat || last_type2 == NodeType::ConstFloat) // (это немного не так работает)
				is_float_op = true;
			CodeOperator(expr[i].value.op, is_float_op);
			if (!IsUnaryOperatorType(expr[i].value.op))
				max_st--;
		}
		else if (type == NodeType::Function) {
			Serial.print("[func]");
			Serial.print(expr[i].value.c);
			CallFunction(expr[i].value.c, -1);
			if(!has_func) {
				max_st++; // вызов функции временно занимает 4 байта для адреса функции
				has_func = true;
			}
		}
		else if (type == NodeType::Array) {
			Serial.print("[array]");
		}
		if (current_scope->max_stack < max_st)
			current_scope->max_stack = max_st;
		last_type2 = last_type;
		last_type = type;
	}
	Serial.println();
}

/*
	function f(args){...}
	f(args)
	|
	V
	[call main scope]
	[function f]{...}
	[main scope]{...call f...}
	
*/

int Code::MainScope() {
	Serial.println(F("CodeMainScope"));
	char *name = (char*)"-main";
	bytecode.push_back(I_OP_LOADICONST);
	bytecode.push_back(0);
	bytecode.push_back(I_CALL);
	bytecode.push_back(0);
	int pos_const = AddConstInt(0); // константа - указатель
	NewScope(name); // новое окружение
	Add_08(I_FUNC_PT);
	main_scope_p = current_scope->fcode.size(); // указатель на 'func_pt'
	// 1 байт под количество переменных (locals) = func_pt
	// 1 байт под резерв стека (max_stack) = func_pt + 1
	Add_16(0);
	return pos_const;
}

void Code::MainScopeEnd(int pos_const) {
	Serial.println(F("CodeMainScopeEnd"));
	char *name = (char*)"-main";
	int loc = current_scope->locals_count;
	uint8_t max_stack = current_scope->max_stack;
	num_consts[pos_const].i = bytecode.size();
	// количество локальных переменных
	current_scope->fcode[main_scope_p] = (uint8_t)loc;
	// резерв стека
	current_scope->fcode[main_scope_p + 1] =  (uint8_t)max_stack;
	ReturnOperator(VarType::None);
	CloseScope(); // возврат в предыдущее окружение
}


/*
** Объявление функции
** 1) func_pt (в новом окружении) [func_pt] (8)
** 2) кол-во перменных в окружении [locals] (8)
** 3) расширение [max_stack] (8)
*/

FuncPoint Code::FunctionDeclare(char *name) { // возвращает указатель на начало функции в коде
	FuncPoint fp;
	FuncData fd;
	num_t temp;
	int l = FindLocal(name);
	if (l == -1) { // *если не объявлена
		l = AddLocal(name, VarType::Function); // добавить переменную
		locals[l].data = declared_functions.size(); // ссылка на 'FuncData'
		declared_functions.push_back(fd); // добавить 'FuncData'
		declared_functions.back().args = 0;
		temp.i = 0;
		num_consts.push_back(temp); // запушить пустую константу
		// теперь можно будет найти константу по имени
		declared_functions.back().cnst = num_consts.size() - 1;
		fp.c = l;
	} else {
		// ошибка: уже есть что-то с таким именем
	}
	NewScope(name); // новое окружение
	Add_08(I_FUNC_PT); // = func_pt - 1
	fp.pt = current_scope->fcode.size(); // указатель на функцию
	// 1 байт под количество переменных
	// 1 байт под резерв стека
	Add_16(0); // = func_pt
	return fp;
}

void Code::FunctionClose(FuncPoint p) {
	int locals_count = current_scope->locals_count;
	uint8_t max_stack = current_scope->max_stack;
	num_consts[declared_functions[locals[p.c].data].cnst].i = bytecode.size(); // расположение функции в константы
	// количество локальных переменных
	current_scope->fcode[p.pt] = (uint8_t)locals_count;
	// резерв стека
	current_scope->fcode[p.pt + 1] = (uint8_t)max_stack;
	ReturnOperator(VarType::None);
	CloseScope(); // возврат в старое окружение
}

// Вызов функции с аргументами / без аргументов

void Code::CallFunction(char *name, int args) {
	int expected = GetArgs(name);
	if (args != expected);
		Serial.println(F("[CallFunction] args warning!"));
	if (args < 0)
		args = expected;
	LocalLoad(name);
	Add_08(I_CALL);
	Add_08((uint8_t)args);
}

// не доделано

void Code::ReturnOperator(VarType type) {
	if (type == VarType::None)
		Add_08(I_OP_RETURN);
	else if (type == VarType::Int)
		Add_08(I_OP_IRETURN);
	else if (type == VarType::Float)
		Add_08(I_OP_FRETURN);
}


// ### Условие ###

size_t Code::StartIfStatement() {
	size_t pos;
	Add_08(I_OP_JMPZ);
	pos = current_scope->fcode.size();
	Add_16(0);
	return pos;
}

size_t Code::ElseIfStatement() {
	size_t pos;
	// если тело выполнилось, то нужно пропустить всё остальное
	Add_08(I_OP_JMP);
	pos = current_scope->fcode.size();
	Add_16(0);
	return pos;
}

void Code::CloseIfBranch(size_t ifs_pt) {
	int16_t cpt = current_scope->fcode.size() - ifs_pt;
	// если условие не выпонилось, то нужно пропустить тело
	current_scope->fcode[ifs_pt] = (uint8_t)((cpt >> 8) & 0xFF); // для команды
	current_scope->fcode[ifs_pt + 1] = (uint8_t)(cpt & 0xFF); // jumpz
}

void Code::CloseIfStatement(std::vector<size_t> labels) {
	int16_t cpt;
	size_t pos = current_scope->fcode.size();
	for (int i = 0; i < labels.size(); i++) {
		cpt = pos - labels[i];
		current_scope->fcode[labels[i]] = (uint8_t)((cpt >> 8) & 0xFF);
		current_scope->fcode[labels[i] + 1] = (uint8_t)(cpt & 0xFF);
	}
}


// ### Цикл с предусловием ###

size_t Code::StartWhileStatement() {
	size_t pos;
	Add_08(I_OP_JMPZ);
	pos = current_scope->fcode.size();
	Add_16(0);
	return pos;
}

void Code::CloseWhileStatement(size_t ws_pt, size_t condition) {
	int16_t jpt = condition - current_scope->fcode.size() - 2;
	Add_08(I_OP_JMP); // возврат к проверке условия
	Add_08((jpt >> 8) & 0xFF);
	Add_08(jpt & 0xFF);
	int16_t cpt = current_scope->fcode.size() - ws_pt;
	current_scope->fcode[ws_pt] = (uint8_t)((cpt >> 8) & 0xFF); // jmpz
	current_scope->fcode[ws_pt + 1] = (uint8_t)(cpt & 0xFF);
}

// ### Массивы ###

void Code::ArrayDeclaration(char *name) {
	Add_08(I_OP_NEWARRAY);
	LocalStore(name);
}

void Code::ArrayLoad(char *name) {
	LocalLoad(name);
	Add_08(I_OP_IALOAD);
}

void Code::ArrayStore(char *name) {
	LocalLoad(name);
	Add_08(I_OP_IASTORE);
}

// ### //

size_t Code::GetFuncCodeSize() {
	return current_scope->fcode.size();
}

uint8_t* Code::GetCode() {
	std::vector<uint8_t>::iterator i;
	uint8_t *code = new uint8_t[bytecode.size()];
	uint8_t *c = code;
	i = bytecode.begin();
	while (i != bytecode.end()) {
		*c++ = *i++;
	}
	bytecode.clear();
	return code;
}

num_t* Code::GetConsts() {
	std::vector<num_t>::iterator i;
	num_t *cnst = new num_t[num_consts.size()];
	num_t *c = cnst;
	i = num_consts.begin();
	while (i != num_consts.end()) {
		*c++ = *i++;
	}
	num_consts.clear();
	return cnst;
}

size_t Code::GetCodeLength() {
	return bytecode.size(); 
}

size_t Code::GetConstsCount() {
	return num_consts.size();
}


