
// Компилятор

#include "scr_code.h"

#include "HardwareSerial.h"

// ###

static inline void c_memcpy(void *dst, void *src, uint16_t len) {
	uint8_t *dst8 = (uint8_t*)dst;
	uint8_t *src8 = (uint8_t*)src;
	while(len--)
		*dst8++ = *src8++;
}

// ###

Uint8buffer::Uint8buffer(){}

Uint8buffer::~Uint8buffer() {
	if (data != nullptr)
		delete[] data;
}

Uint8buffer::Uint8buffer(int len){
	data = new uint8_t[len];
	pointer = data - 1;
	length = len;
}

void Uint8buffer::Add08(uint8_t n) {
	*(++pointer) = n;
}

void Uint8buffer::Add16(uint16_t n) {
	*(++pointer) = n & 0xFF;
	*(++pointer) = (n >> 8) & 0xFF;
}

void Uint8buffer::Set08(uint8_t* pt, uint8_t n) {
	*(pt) = n;
}

uint8_t Uint8buffer::Get08(uint8_t* pt) {
	return *(pt);
}

/* /////////////////////////////////////////////////////////////////////////////////// */

bool IsConstType(var_type t) {
	switch(t) {
		case var_type::Function:
		case var_type::ConstInt:
		case var_type::ConstFloat:
			return true;
			break;
	}
	return false;
}

var_name::~var_name() {
	if (name != nullptr)
		delete[] name;
}

func_descr::func_descr(){}

func_descr::func_descr(uint8_t* start, uint8_t* end) {
	start_pt = start;
	end_pt = end;
}

// ######

void Code::Init() {
	rcode = new Uint8buffer(256); // код функции (=256 + 12 б)
	bytecode = new Uint8buffer(1024); // байткод (=1024 + 12 б)
	consts = new num_t[256]; // константы (=4 * 256 = 1024 б)
	locals = new var_name[256]; // переменные (=7 * 256 = 1792 б)
	consts_count = 0;
	total_locals_count = 0;
	last_scope_id = -1;
	NewScope();
}

void Code::Add(uint8_t i) {
	rcode->Add08(i);
}

void Code::Add_08(uint8_t n) {
	rcode->Add08(n);
}

int Code::Add_const_int(int n) {
	consts[consts_count].i = n;
	return consts_count++;
}

int Code::Add_const_float(float n) {
	consts[consts_count].f = n;
	return consts_count++;
}

// Новое окружение

void Code::NewScope() {
	last_scope_id++;
	scope_info *new_scope = new(scope_info); // Создает звено
	new_scope->locals_count = 0;
	new_scope->locals_start = total_locals_count;
	new_scope->id = (uint8_t)last_scope_id;
	new_scope->max_stack = 0;
	new_scope->parent = current_scope;
	current_scope = new_scope;
}

// Возвращает в предыдущее окружение

void Code::CloseScope() {
	int len;
	scope_info *sc;
	if (current_scope != nullptr) {
		total_locals_count = current_scope->locals_start; // удаляет имена переменных, т.к. они не существуют вне функции
		if (current_scope->parent != nullptr) {
			len = rcode->pointer - rcode->data;
			c_memcpy(bytecode->pointer, rcode->data, len);
			bytecode->pointer += len;
			sc = current_scope;
			current_scope = current_scope->parent;
			last_scope_id = current_scope->id;
			delete sc; // удаляет звено
		}
	}
}

// Добавить локальную переменную в список переменных

int Code::AddLocal(char *name, var_type type) {
	Serial.print("AddLocal: "); // ###
	uint8_t id = total_locals_count;
	locals[id].name = name; // запомнить имя, чтобы получить ид
	locals[id].type = (uint8_t)type; // тип переменной
	locals[id].id = current_scope->locals_count; // количество переменных
	Serial.println(current_scope->locals_count); // ###
	current_scope->locals_count++;
	return total_locals_count++;
}

// Указать количество аргументов

void Code::SetArgs(char *name, uint8_t args) {
	int l = FindLocal(name);
	locals[l].args = args;
}

// Узнать количество аргументов

uint8_t Code::GetArgs(char *name) {
	int l = FindLocal(name);
	return locals[l].args;
}


// найти переменную из списка

int Code::FindLocal(char *name) {
	for (int i = total_locals_count - 1; i >= 0; i--){
		if (Compare_strings(name, locals[i].name))
			return i;
	}
	return -1;
}

void Code::CodeOperator(OpType t, bool IsFloat) {
	switch(t){
		case OpType::UnPlus:
			if (IsFloat) 
				Add(I_OP_FUPLUS); 
			else
				Add(I_OP_IUPLUS);
			break;
		case OpType::UnMinus:
			if (IsFloat) 
				Add(I_OP_FUNEG); 
			else
				Add(I_OP_IUNEG);
			break;
		case OpType::UnNot: Add(I_OP_INOT); break;
		case OpType::BinAdd: 
			if (IsFloat) 
				Add(I_OP_FADD); 
			else
				Add(I_OP_IADD);
			break;
		case OpType::BinSub:
			if (IsFloat) 
				Add(I_OP_FSUB); 
			else
				Add(I_OP_ISUB);
			break;
		case OpType::BinMul:
			if (IsFloat) 
				Add(I_OP_FMUL); 
			else
				Add(I_OP_IMUL);
			break;
		case OpType::BinDiv:
			if (IsFloat) 
				Add(I_OP_FDIV); 
			else
				Add(I_OP_IDIV);
			break;
		case OpType::BinMod:
			if (IsFloat) 
				Add(I_OP_FREM); 
			else
				Add(I_OP_IREM);
			break;
		case OpType::BinLess: Add(I_OP_LESS); break;
		case OpType::BinLessOrEqual: Add(I_OP_LESSEQ); break;
		case OpType::BinEqual: Add(I_OP_EQ); break;
		case OpType::BinNotEqual: Add(I_OP_NOTEQ); break;
		case OpType::BinGreater: Add(I_OP_GR); break;
		case OpType::BinGreaterOrEqual: Add(I_OP_GREQ); break;
		case OpType::BinAnd: Add(I_OP_IAND); break;
		case OpType::BinOr: Add(I_OP_IOR); break;
	}
}

int Code::FindConst(int n) {
	for (int i = 0; i < consts_count - 1; i++){
		if (consts[i].i == n){
			return i;
		}
	}
	return -1;
}

// Кодирует константу

// Целое число

void Code::CodeConstInt(int n) {
	// если такая константа уже объявлена, то используем её
	int pos = FindConst(n);
	if (pos == -1)
		pos = Add_const_int(n);
	Add(I_OP_LOADICONST);
	Add_08((uint8_t)pos);
}

// Число с плавающей точкой

void Code::CodeConstFloat(float n) {
	int pos = FindConst(n);
	if (pos == -1)
		pos = Add_const_float(n);
	Add(I_OP_LOADFCONST);
	Add_08((uint8_t)pos);
}

// Константа - целое число в виде переменной
// имя, значение, тип

void Code::CodeDeclareConstVarInt(char *name, int n, var_type type) {
	int l = FindLocal(name);
	if (l == -1) {
		l = AddLocal(name, type);
		locals[l].args = Add_const_int(n);
	} else {
		// ошибка: уже есть что-то с таким именем
	}
}

// void Code::CodeConstVarInt(char *name, int n) {
	// int pos = FindConst(n);
	// if (pos == -1)
		// pos = Add_const_int(n);
// }

// Кодирует переменные в выражениях 

void Code::CodeLocalLoad(char *name) {
	int l = FindLocal(name);
	if (l == -1) {
		Serial.print("Error: ");
		Serial.print(name);
		Serial.println(" was not declared");
		throw "";
	}
	else {
		uint8_t id = locals[l].id;
		if (IsConstType((var_type)locals[l].type)) { // Добавляет ранее объявленную константу
			Add(I_OP_LOADICONST); 
			Add_08(locals[l].args);
		} else { // Просто переменная
			Add(I_OP_LOADLOCAL);
			Add_08(id);
		}
	}
	if (current_scope->max_stack < 1) { // добавляет место для 1 переменной, если его не хватает
		current_scope->max_stack = 1;
	}
}

// Кодирует запись в переменную

void Code::CodeLocalStore(char *name) {
	uint8_t id;
	int l = FindLocal(name);
	if (l == -1) { // если не объявлена
		l = AddLocal(name, var_type::Int);
		// если переменная принадлежит функции, она находится в её кадре:
		Add(I_OP_STORELOCAL);
		Add_08(locals[l].id); // её можно найти с помощью 8-битного идентификатора
	}
	else {
		id = locals[l].id;
		Add(I_OP_STORELOCAL);
		Add_08((uint8_t)id);
	}
}

int Code::CodeLocalDeclare(char *name, var_type type) {
	int l = FindLocal(name);
	if (l == -1) {
		int id = AddLocal(name, type);
		return id;
	} else {
		// ошибка
	}
}

void Code::CodeJump(int shift, bool condition, bool expected) {
	if (condition) {
		if (expected)
			Add(I_OP_JMPNZ);
		else
			Add(I_OP_JMPZ);
	}
	else
		Add(I_OP_JMP);
	rcode->Add16(shift);
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

// ### a = function(a, b) -> func, a, b, rparen
// ### CodeExpression -> i = 0 -> CodeExprNode -> type=function -> CodeExprNode ->
// ### -> a, b, rparen -> rec_nodes += 3

void Code::CodeExpression(ExprBuffer *expr) {
	Serial.println("CodeExpression:");
	uint8_t max_st = 0;
	bool has_func = false;
	bool is_float_op;
	NodeType last_type2 = NodeType::None;
	NodeType last_type = NodeType::None;
	NodeType type;
	for (int i = 0; i < expr->count; i++) {
		type = (NodeType)expr->buffer[i].type;
		if (type == NodeType::ConstInt) {
			Serial.print(expr->buffer[i].value.i);
			CodeConstInt(expr->buffer[i].value.i);
			max_st++; // резерв стека увеличивается, чтобы уместить несколько чисел
		}
		else if (type == NodeType::ConstFloat) {
			Serial.print(expr->buffer[i].value.f);
			CodeConstFloat(expr->buffer[i].value.f);
			max_st++;
		}
		else if (type == NodeType::Var) {
			Serial.print(expr->buffer[i].value.c);
			CodeLocalLoad(expr->buffer[i].value.c);
			max_st++;
		}
		else if (type == NodeType::Operator) {
			Serial.print("Op");
			is_float_op = false;
			if (last_type == NodeType::ConstFloat || last_type2 == NodeType::ConstFloat) // (это немного не так работает)
				is_float_op = true;
			CodeOperator(expr->buffer[i].value.op, is_float_op);
			if (!IsUnaryOperatorType(expr->buffer[i].value.op))
				max_st--;
		}
		else if (type == NodeType::Function) {
			Serial.print("[func]");
			Serial.print(expr->buffer[i].value.c);
			CodeCallFunction(expr->buffer[i].value.c, -1);
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

uint8_t* Code::CodeMainScope() {
	// 2 байта константа + 1 байт вызов + 1 байт аргументы + 1 байт возврат
	CodeConstInt((uint32_t)(rcode->pointer - rcode->data + 5));
	Add(I_CALL);
	Add_08(0);
	Add(I_OP_RETURN);
	Add(I_FUNC_PT); // = func_pt
	uint8_t *pt = rcode->pointer; // указатель на функцию
	// 1 байт под количество переменных = func_pt
	// 1 байт под резерв стека = func_pt + 1
	rcode->Add16(0); 
	NewScope(); // новое окружение
	return pt;
}

void Code::CodeMainScopeEnd(uint8_t *pt) {
	int locals = current_scope->locals_count;
	uint8_t max_stack = current_scope->max_stack;
	// количество локальных переменных
	rcode->Set08((uint8_t*)(pt), (uint8_t)locals);
	// резерв стека
	rcode->Set08((uint8_t*)(pt + 1), (uint8_t)max_stack);
	CodeReturnOperator(var_type::None);
	CloseScope(); // возврат в предыдущее окружение
	
}

// напоминалка
// ...[const_load][const_id][local_store][local_id][jmp][addr][func_pt][locals][max_stack]...
// ...[8][8][8][8][8][16][8][8][8]...

uint8_t* Code::CodeStartFunction(char *name) { // возвращает указатель на начало функции в коде
	uint8_t* func_pt = rcode->pointer; // указатель на начало
	CodeConstInt((int)(func_pt - rcode->data + 7)); // расположение функции добавляется в константы
	// 2 байта константа + 2 байта переменная + 3 байта смещение = 7
	CodeLocalDeclare(name, var_type::Function); // создаёт переменную-указатель на функцию
	CodeLocalStore(name);
	// ### т.к. функции расположены друг за другом, необходимо пропускать их код
	Add(I_OP_JMP);
	rcode->Add16(0); // 2 байта для адреса пропуска кода функции
	// ###
	Add(I_FUNC_PT); // = func_pt
	func_pt = rcode->pointer; // указатель на функцию
	// 1 байт под количество переменных
	Add_08(0); // = func_pt
	// 1 байт под резерв стека
	Add_08(0); // = func_pt + 1
	NewScope(); // новое окружение

	return func_pt;
}

// не доделано

void Code::CodeReturnOperator(var_type type) {
	if (type == var_type::None)
		Add(I_OP_RETURN);
	else if (type == var_type::Int)
		Add(I_OP_IRETURN);
	else if (type == var_type::Float)
		Add(I_OP_FRETURN);
}

//

void Code::CodeCloseFunction(uint8_t *func_pt) {
	int locals = current_scope->locals_count;
	uint8_t max_stack = current_scope->max_stack;
	CodeReturnOperator(var_type::None);
	// пропуск кода
	int16_t cpt = (int)rcode->pointer - (int)func_pt + 3;
	rcode->Set08((uint8_t*)(func_pt - 2), (uint8_t)(cpt & 0xFF)); // для команды
	rcode->Set08((uint8_t*)(func_pt - 3), (uint8_t)((cpt >> 8) & 0xFF)); // jump
	// количество локальных переменных
	rcode->Set08((uint8_t*)(func_pt), (uint8_t)locals);
	// резерв стека
	rcode->Set08((uint8_t*)(func_pt + 1), (uint8_t)max_stack);
	CloseScope(); // возврат в старое окружение
}

// Вызов функции с аргументами / без аргументов

void Code::CodeCallFunction(char *name, int args) {
	if (args < 0)
		args = GetArgs(name);
	CodeLocalLoad(name);
	Add(I_CALL);
	Add_08((uint8_t)args);
}

// ### Массивы ###

void Code::CodeArrayDeclaration(char *name) {
	Add(I_OP_NEWARRAY);
	CodeLocalStore(name);
}

void Code::CodeArrayLoad(char *name) {
	CodeLocalLoad(name);
	Add(I_OP_IALOAD);
}

void Code::CodeArrayStore(char *name) {
	CodeLocalLoad(name);
	Add(I_OP_IASTORE);
}

// ### //

// ### * Условие * ###

uint8_t* Code::CodeStartIfStatement() {
	Add(I_OP_JMPZ);
	rcode->Add16(0);
	uint8_t *ifs_pt = rcode->pointer;
	return ifs_pt;
}

void Code::CodeCloseIfStatement(uint8_t *ifs_pt) {
	short_t cpt = rcode->pointer - ifs_pt + 1;
	rcode->Set08((uint8_t*)(ifs_pt - 1), (uint8_t)(cpt & 0xFF)); // для команды
	rcode->Set08((uint8_t*)(ifs_pt - 2), (uint8_t)((cpt >> 8) & 0xFF)); // jumpz
}

// ### //

// ### Цикл с предусловием ###

uint8_t* Code::CodeStartWhileStatement() {
	Add(I_OP_JMPZ);
	rcode->Add16(0);
	uint8_t *ws_pt = rcode->pointer;
	return ws_pt;
}

void Code::CodeCloseWhileStatement(uint8_t *ws_pt, uint8_t *exs) {
	Add(I_OP_JMP); // возврат к проверке условия
	short_t jpt = -(rcode->pointer - exs + 1); // и вычислению выражения
	rcode->Add08((jpt >> 8) & 0xFF);
	rcode->Add08(jpt & 0xFF);
	short_t cpt = rcode->pointer - ws_pt + 2;
	rcode->Set08((uint8_t*)(ws_pt - 1), (uint8_t)(cpt & 0xFF)); // для команды
	rcode->Set08((uint8_t*)(ws_pt - 2), (uint8_t)((cpt >> 8) & 0xFF)); // jumpz
}

// ### //

uint8_t* Code::GetCode() {
	return rcode->data;
}

num_t* Code::GetConsts() {
	return consts;
}

int Code::GetConstsCount() {
	return consts_count;
}

void Code::DeleteTemp() {
	delete[] locals;
}

void Code::DeleteBuffer() {
	delete[] consts;
}

