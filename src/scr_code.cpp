/**
** Компилятор
**
**/

#include "scr_code.h"
#include "utils/scr_debug.h"
#include "HardwareSerial.h"

/* -~^~-~^~-~^~-~^~-~^~-~^~-~^~-~^~-~^~-~^~-~^~-~^~-~^~-~^~-~^~-~^~-~^~-~^~-~^~-~^~- */

// ######

Code::Code() {
	total_str_length = 0;
	NewScope((char*)"%");
}

void Code::InternalException(int code) {
	throw code;
}

void Code::ErrorDeclared(char *name) {
	PRINTF("'%.64s' already declared", name);
	InternalException(1);
}

void Code::ErrorNotDeclared(char *name) {
	PRINTF("'%.64s' was not declared", name);
	InternalException(1);
}

void Code::ErrorDeclaredFunction(char *name) {
	PRINTF("function '%.64s' already declared", name);
	InternalException(1);
}

void Code::ErrorNotDeclaredFunction(char *name) {
	PRINTF("function '%.64s' was not declared", name);
	InternalException(1);
}

void Code::Add_08(uint8_t n) {
	current_scope->fcode.push_back(n);
}

void Code::Add_16(uint16_t n) {
	current_scope->fcode.push_back(n & 0xFF);
	current_scope->fcode.push_back((n >> 8) & 0xFF);
}

// Новое окружение

void Code::NewScope(char *name) {
	Serial.println(F("Code::NewScope"));
	FuncState *new_scope = new(FuncState); // Создает звено
	new_scope->name = name;
	new_scope->locals_count = 0;
	new_scope->decl_funcs = 0;
	new_scope->max_stack = 0;
	new_scope->prev = current_scope;
	current_scope = new_scope;
}

// Возвращает в предыдущее окружение

void Code::CloseScope() {
	Serial.println(F("Code::CloseScope"));
	FuncState *temp;
	if (current_scope != nullptr) {
		// удаление имён из этого окружения
		locals.erase(locals.begin() + (locals.size() - current_scope->locals_count), 
				locals.end());
		declared_functions.erase(declared_functions.begin() + (declared_functions.size() - current_scope->decl_funcs), 
				declared_functions.end());
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
			for (auto i = current_scope->fcode.begin();
				i != current_scope->fcode.end(); i++)
			{
				bytecode.push_back(*i);
			}
			temp = current_scope;
			current_scope = current_scope->prev;
			delete temp; // удаляет звено
		}
	}
}

int DotsCount(char *c){
	int count = 0;
	char *temp = c;
	while (*temp != '\0') {
		if (*temp == '.')
			count++;
		temp++;
	}
	return count;
}


char** NameSeparation(char *name, int& levels){
	Serial.println("NameSeparation[1]");
	char** result;
	std::vector<std::string> buf;
	std::string temp;
	int i = 0, n = 0, len = 0;
	for (; i < strlen(name); i++){
		if (name[i] == '.'){
			buf.push_back(temp.c_str());
			i++;
			n = 0;
		}
		temp += name[i];
		n++;
	}
	result = new char*[len];
	for (int l = 0; l < len; l++){
		result[l] = new char[buf.at(l).size() + 1];
		strcpy(result[l], buf.at(l).c_str());
	}
	levels = len;
	Serial.println("NameSeparation[2]");
	return result;
}

// RegisterLib("mylib", mylib, sizeof(mylib)/sizeof(mylib[0]));

void Code::RegisterLib(const char* name, const lib_reg *lib, size_t size) {
	lib_struct ls;
	ls.name = name;
	ls.lib = lib;
	ls.size = size;
	cpp_lib.push_back(ls);
}

int FindVarType(VarType type) {
	for (int i = 0; i < sizeof(var_types_info) / sizeof(var_types_info[0]); i++) {
		if (var_types_info[i].type == type)
			return i;
	}
	return -1;
}

bool IsConstVarType(VarType type) {
	for (int i = 0; i < sizeof(var_types_info) / sizeof(var_types_info[0]); i++) {
		if (var_types_info[i].type == type) {
			if (var_types_info[i].is_const)
				return true;
			else
				return false;
		}
	}
	return false;
}

bool IsIntVarType(VarType type) {
	for (int i = 0; i < sizeof(var_types_info) / sizeof(var_types_info[0]); i++) {
		if (var_types_info[i].type == type) {
			if (var_types_info[i].is_int)
				return true;
			else
				return false;
		}
	}
	return false;
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

int Code::AddConstString(char *s) {
	int pos = FindConstString(s);
	if (pos == -1) {
		str_consts.push_back(s);
		return str_consts.size() - 1;
	}
	return pos;
}

/* Добавить функцию в список
 * возвращает номер константы
*/

size_t Code::AddFunction(char *name) {
	FuncData fd;
	num_t temp;
	int l = FindFunction(name);
	if (l == -1) { // *если не объявлена
		fd.name = name;
		declared_functions.push_back(fd);
		num_consts.push_back(temp); // запушить пустую константу
		declared_functions.back().pointer =  num_consts.size() - 1;
		current_scope->decl_funcs++; // кол-во функций в этом окружениеи +1
		return declared_functions.size() - 1;
	} else {
		// ошибка: уже есть что-то с таким именем
		ErrorDeclaredFunction(name);
	}
	return 0;
}

int Code::FindFunction(char *name) {
	int n = declared_functions.size();
	for (auto i = declared_functions.rbegin(); 
	i != declared_functions.rend(); i++)
	{
		n--;
		if (Compare_strings(name, i->name))
			return n;
	}
	return -1;
}

/* Поиск встроенной библиотеки
** lib_label - метка библиотеки
*/

int Code::FindBuiltInLib(char *lib_label){
	int n = cpp_lib.size();
	for (auto i = cpp_lib.begin(); i != cpp_lib.end(); i++)
	{
		n--;
		if (Compare_strings(lib_label, i->name))
			return n;
	}
	return -1;
}

/* Поиск встроенной функции
** lib_label - метка библиотеки
** name - имя функции
*/

int Code::FindBuiltInFunction(char *lib_label, char *name) {
	int n = cpp_lib.size();
	for (auto i = cpp_lib.begin(); i != cpp_lib.end(); i++)
	{
		n--;
		// если найдена библиотека с такой меткой
		if (lib_label == nullptr || Compare_strings(lib_label, i->name)) {
			for (int i = 0; i < cpp_lib.at(n).size; i++) {
				if (Compare_strings(cpp_lib.at(n).name, name)) {
					return i;
				}
			}
			// если нет метки, то можно искать в другой библиотеке
			if (lib_label == nullptr)
				continue;
			else
				return -1;
		}
	}
	return -1;
}

// Указать количество аргументов

void Code::SetArgs(char *name, std::vector<func_arg_struct> args) {
	int l = FindFunction(name);
	declared_functions[l].args = args;
}

// Узнать количество аргументов

size_t Code::GetArgs(char *name) {
	int l = FindFunction(name);
	return declared_functions[l].args.size();
}

func_arg_struct Code::GetArg(char *name, int n) {
	int l = FindFunction(name);
	return declared_functions[l].args[n];
}

// Поиск локальной переменной

int Code::FindLocal(char *name) {
	int n = locals.size();
	for (auto i = locals.rbegin(); i != locals.rend(); i++)
	{
		n--;
		if ((IsConstVarType((VarType)locals[n].type) // если константа
			|| n >= (locals.size() - current_scope->locals_count)) // или объявлена в этом окружении
			&& Compare_strings(name, i->name))
			return n;
	}
	return -1;
}

// Поиск локальной переменной

int Code::FindGlobal(char *name) {
	int n = locals.size();
	for (auto i = locals.rbegin(); i != locals.rend(); i++)
	{
		n--;
		if (IsConstVarType((VarType)locals[n].type))
			return n;
	}
	return -1;
}

int Code::FindVariable(char *name, bool &is_local) {
	int l = FindLocal(name);
	if (l != -1) {
		is_local = true;
		return l;
	}
	is_local = false;
	return FindGlobal(name);
}

// Поиск констант

int Code::FindConstInt(int n) {
	int pos = 0;
	for (auto i = num_consts.begin(); i != num_consts.end(); i++) {
		if ((*i).i == n) {
			return pos;
		pos++;
		}
	}
	return -1;
}

int Code::FindConstFloat(float n) {
	int pos = 0;
	for (auto i = num_consts.begin(); i != num_consts.end(); i++) {
		if ((*i).f == n) {
			return pos;
		pos++;
		}
	}
	return -1;
}

int Code::FindConstString(char *s) {
	int pos = 0;
	for (auto i = str_consts.begin(); i != str_consts.end(); i++) {
		if (Compare_strings(*i, s)) {
			return pos;
		pos++;
		}
	}
	return -1;
}

// Кодирует константу

// Целое число

int Code::CodeConstInt(int n) {
	if (n == 0) {
		Add_08(I_OP_LOAD0);
	} else if (n == 1){
		Add_08(I_OP_ILOAD1);
	} else {
		int pos = AddConstInt(n);
		Add_08(I_OP_LOADICONST);
		Add_16(pos);
		return pos;
	}
	return 0;
}

// Число с плавающей точкой

int Code::CodeConstFloat(float n) {
	if (n == 0) {
		Add_08(I_OP_LOAD0);
	} else {
		int pos = AddConstFloat(n);
		Add_08(I_OP_LOADFCONST);
		Add_16((uint16_t)pos);
		return pos;
	}
	return 0;
}

// Строка

int Code::CodeConstString(char *s) {
	int pos = AddConstString(s);
	size_t pt = 1; // указатель на строку для вм
	for (int i = 0; i < str_consts.size(); i++){
		pt += strlen(str_consts.at(i)) + 1;
	}
	CodeConstInt(pt);
	Add_08(I_OP_LOADSTRING);
	Add_16((uint16_t)pos);
	return pos;
}

// Загрузка переменных в код

void Code::CodeLoadVariable(VarType type, bool is_local, int index) {
	int pos = FindVarType(type);
	if (var_types_info[pos].is_const) { // константы
		if (var_types_info[pos].is_int) {
			Add_08(I_OP_LOADICONST);
		} else {
			Add_08(I_OP_LOADFCONST);
		}
		Add_16(locals[index].data);
	} else { 
		if (is_local) {
			if (var_types_info[pos].is_int) { // локальные
				Add_08(I_OP_ILOAD);
			} else {
				Add_08(I_OP_FLOAD);
			}
			Add_08(locals[index].id);
		} else { 
			if (var_types_info[pos].is_int) { // глобальные
				Add_08(I_OP_GLOAD);
			} else {
				Add_08(I_OP_GLOAD);
			}
			Add_08(globals[index].id);
		}
	}
	// добавляет место для 1 переменной, если его не хватает
	if (current_scope->max_stack < 1)
		current_scope->max_stack = 1;
}

void Code::CodeStoreVariable(VarType type, bool is_local, int index) {
	int pos = FindVarType(type);
	if (is_local){ // локальные
		if (var_types_info[pos].is_int) {
			Add_08(I_OP_ISTORE);
		} else {
			Add_08(I_OP_FSTORE);
		}
		Add_08(locals[index].id);
	} else { // глобальные
		if (var_types_info[pos].is_int) {
			Add_08(I_OP_GSTORE);
		} else {
			Add_08(I_OP_GSTORE);
		}
		Add_08(globals[index].id);
	}
}

// Добавить переменную в список

int Code::PushVar(char *name, VarType type, bool is_local) {
	VarState vs;
	if (is_local) {
		locals.push_back(vs);
		locals.back().name = name;
		locals.back().type = (uint8_t)type;
		locals.back().data = 0;
		locals.back().id = current_scope->locals_count;
		current_scope->locals_count++;
		return locals.size() - 1;
	} else {
		globals.push_back(vs);
		globals.back().name = name;
		globals.back().type = (uint8_t)type;
		globals.back().data = 0;
		globals.back().id = globals.size() - 1;
		return globals.size() - 1;
	}
}

// Кодирует переменные в выражениях 

void Code::LoadLocal(char *name) {
	int l = FindLocal(name);
	if (l == -1) { // *если не объявлена
		ErrorNotDeclared(name);
	} else {
		VarType type = (VarType)locals[l].type;
		CodeLoadVariable(type, true, l);
	}
}

// Локальные переменные

void Code::StoreLocal(char *name) {
	int l = FindLocal(name);
	if (l == -1) { // *если не объявлена
		ErrorNotDeclared(name);
	} else {
		VarType type = (VarType)locals[l].type;
		CodeStoreVariable(type, true, l);
	}
}

int Code::DeclareLocal(char *name, VarType type) {
	int l = FindLocal(name);
	if (l == -1) { // если не объявлена
		return PushVar(name, type, true);
	} else {
		ErrorDeclared(name);
	}
	return 0;
}

void Code::LoadVariable(char *name, bool &is_local) {
	int l = FindLocal(name);
	if (l == -1) { // возможно, глобальная
		l = FindGlobal(name);
		if (l == -1) { // переменная нигде не объявлена
			ErrorNotDeclared(name);
		} else {
			VarType type = (VarType)globals[l].type;
			CodeLoadVariable(type, false, l);
		}
	} else {
		VarType type = (VarType)locals[l].type;
		CodeLoadVariable(type, true, l);
	}
}

void Code::StoreVariable(char *name, bool &is_local) {
	int l = FindLocal(name);
	if (l == -1) { // возможно, глобальная
		l = FindGlobal(name);
		if (l == -1) { // переменная нигде не объявлена
			ErrorNotDeclared(name);
		} else {
			VarType type = (VarType)globals[l].type;
			CodeStoreVariable(type, false, l);
		}
	} else {
		VarType type = (VarType)locals[l].type;
		CodeStoreVariable(type, true, l);
	}
}

// Глобальные переменные

void Code::LoadGlobal(char *name) {
	int l = FindGlobal(name);
	if (l == -1) { // *если не объявлена
		ErrorNotDeclared(name);
	} else {
		VarType type = (VarType)globals[l].type;
		CodeLoadVariable(type, false, l);
	}
}

void Code::StoreGlobal(char *name) {
	int l = FindGlobal(name);
	if (l == -1) { // *если не объявлена
		ErrorNotDeclared(name);
	} else {
		VarType type = (VarType)globals[l].type;
		CodeStoreVariable(type, false, l);
	}
}

int Code::DeclareGlobal(char *name, VarType type) {
	int l = FindGlobal(name);
	if (l == -1) { // если не объявлена
		return PushVar(name, type, false);
	} else {
		ErrorDeclared(name);
	}
	return 0;
}

//?///////

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
	switch (t){
		case OpType::Nop:

		break;
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
		case OpType::UnBitNot: Add_08(I_OP_IBNOT); break;
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
		case OpType::BinLeftShift: Add_08(I_OP_ISHL); break;
		case OpType::BinRightShift: Add_08(I_OP_ISHR); break;
		case OpType::BinAnd: Add_08(I_OP_IAND); break;
		case OpType::BinOr: Add_08(I_OP_IOR); break;
		case OpType::BinBitAnd: Add_08(I_OP_IBAND); break;
		case OpType::BinBitOr: Add_08(I_OP_IBOR); break;
	}
}

bool IsUnaryOperatorType(OpType type) {
	switch(type) {
		case OpType::UnPlus:
		case OpType::UnMinus:
		case OpType::UnNot:
		case OpType::UnBitNot:
			return true;
	}
	return false;
}

/** Выражение
** a = function(a, b) -> func, a, b, rparen
** CodeExpression -> i = 0 -> CodeExprNode -> type=function -> CodeExprNode ->
** -> a, b, rparen -> rec_nodes += 3
** После вычислений на стеке остнутся результаты, их нужно 
** теоретически посчитать и сложить с предыдущими
**/ 

Node const_un_operation(OpType op, Node n1) {
	Node exb;
	switch (op) {
		case OpType::UnPlus:
			
		break;
		case OpType::UnMinus:

		break;
		case OpType::UnNot:
		
		break;
		case OpType::UnBitNot:
		
		break;
	}
	return exb;
}

Node const_bin_operation(OpType op, Node n1, Node n2) {
	Node exb;
	NodeType type = NodeType::ConstFloat;
	float _n1, _n2;
	// Преобразование в float, для удобства
	if (n1.type == NodeType::ConstFloat && n2.type == NodeType::ConstInt) {
		_n1 = n1.value.f;
		_n2 = (float)n2.value.i;
	}
	else if (n1.type == NodeType::ConstInt && n2.type == NodeType::ConstFloat) {
		_n1 = (float)n1.value.i;
		_n2 = n2.value.f;
	}
	else if (n1.type == NodeType::ConstFloat && n2.type == NodeType::ConstFloat) {
		_n1 = n1.value.f;
		_n2 = n2.value.f;
	}
	else {
		_n1 = (float)n1.value.i;
		_n2 = (float)n2.value.i;
		type = NodeType::ConstInt;
	}
	// вычисление
	switch (op) {
		case OpType::BinAdd:
			if (type == NodeType::ConstFloat)
				exb.value.f = _n1 + _n2;
			else
				exb.value.i = (int)(_n1 + _n2);
		break;
		case OpType::BinSub:
			if (type == NodeType::ConstFloat)
				exb.value.f = _n1 - _n2;
			else
				exb.value.i = (int)(_n1 - _n2);
		break;
		case OpType::BinMul:
			if (type == NodeType::ConstFloat)
				exb.value.f = _n1 * _n2;
			else
				exb.value.i = (int)(_n1 * _n2);
		break;
		case OpType::BinDiv:
			if (type == NodeType::ConstFloat)
				exb.value.f = _n1 / _n2;
			else
				exb.value.i = (int)(_n1 / _n2);
		break;
		case OpType::BinMod:
			if (type == NodeType::ConstFloat)
				exb.value.f = (int)_n1 % (int)_n2;
			else
				exb.value.i = (int)_n1 % (int)_n2;
		break;
		case OpType::BinLess:
			if (type == NodeType::ConstFloat)
				exb.value.f = _n1 < _n2;
			else
				exb.value.i = (int)_n1 < (int)_n2;
		break;
		case OpType::BinLessOrEqual:
			if (type == NodeType::ConstFloat)
				exb.value.f = _n1 <= _n2;
			else
				exb.value.i = (int)_n1 <= (int)_n2;
		break;
		case OpType::BinEqual:
					if (type == NodeType::ConstFloat)
				exb.value.f = _n1 == _n2;
			else
				exb.value.i = (int)_n1 == (int)_n2;
		break;
		case OpType::BinNotEqual:
			if (type == NodeType::ConstFloat)
				exb.value.f = _n1 != _n2;
			else
				exb.value.i = (int)_n1 != (int)_n2;
		break;
		case OpType::BinGreater:		
			if (type == NodeType::ConstFloat)
				exb.value.f = _n1 > _n2;
			else
				exb.value.i = (int)_n1 >= (int)_n2;
		break;
		case OpType::BinGreaterOrEqual:
			if (type == NodeType::ConstFloat)
				exb.value.f = _n1 > _n2;
			else
				exb.value.i = (int)_n1 >= (int)_n2;
		break;
		case OpType::BinLeftShift:
			if (type == NodeType::ConstFloat)
				exb.value.i = (int)_n1 << (int)_n2;
			else
				exb.value.i = (int)_n1 << (int)_n2;
		break;
		case OpType::BinRightShift:
			if (type == NodeType::ConstFloat)
				exb.value.i = (int)_n1 >> (int)_n2;
			else
				exb.value.i = (int)_n1 >> (int)_n2;
		break;
		case OpType::BinAnd:
			if (type == NodeType::ConstFloat)
				exb.value.f = _n1 && _n2;
			else
				exb.value.i = (int)_n1 && (int)_n2;
		break;
		case OpType::BinOr:
			if (type == NodeType::ConstFloat)
				exb.value.f = _n1 || _n2;
			else
				exb.value.i = (int)_n1 || (int)_n2;
		break;
	}
	exb.type = type;
	return exb;
}

void Code::ExprGenericOperation(Node n) {
	NodeType type = n.type;
	switch(type) {
		case NodeType::ConstInt:
			PRINTF("%d ", n.value.i);
			CodeConstInt(n.value.i);
		break;
		case NodeType::ConstFloat:
			PRINTF("%f ", n.value.f);
			CodeConstFloat(n.value.f);
		break;
		case NodeType::Var: {
			PRINTF("'%s' ", n.value.c);
			bool is_local;
			LoadVariable(n.value.c, is_local);
		break;
		}
		case NodeType::Function:
			PRINTF("[%s]() ", n.value.c);
			CallFunction(n.value.c, -1);
		break;
		case NodeType::Array:
			PRINTF("[%s][] ", n.value.c);
			LoadArray(n.value.c);
		break;
	}
}

// max_st по умолчанию = 0

void Code::Expression(std::vector<Node> expr, uint8_t _max_st) {
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
			bool is_local;
			LoadVariable(expr[i].value.c, is_local);
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
			max_st+=2;
			LoadArray(expr[i].value.c);
		}
		if (current_scope->max_stack < max_st)
			current_scope->max_stack = max_st;
		last_type2 = last_type;
		last_type = type;
	}
	Serial.println();
}


/* 
** 
*/

int Code::MainScope() {
	Serial.println(F("CodeMainScope"));
	char *name = (char*)"-main";
	num_t temp;
	num_consts.push_back(temp);
	int pos_const = num_consts.size() - 1; // константа - указатель
	bytecode.push_back(I_OP_LOADICONST);
	bytecode.push_back((uint16_t)pos_const & 0xFF);
	bytecode.push_back(((uint16_t)pos_const >> 8) & 0xFF);
	bytecode.push_back(I_CALL);
	bytecode.push_back(0);
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

size_t Code::FunctionDeclare(char *name) { // возвращает указатель на начало функции в коде
	size_t pt;
	AddFunction(name);
	NewScope(name); // новое окружение
	Add_08(I_FUNC_PT); // = func_pt - 1
	pt = current_scope->fcode.size(); // указатель на функцию
	// 1 байт под количество переменных
	// 1 байт под резерв стека
	Add_16(0); // = func_pt
	return pt;
}

void Code::FunctionClose(char *name, size_t pt) {
	int locals_count = current_scope->locals_count;
	uint8_t max_stack = current_scope->max_stack;
	size_t fnum = FindFunction(name);
	num_consts[declared_functions[fnum].pointer].i = bytecode.size(); // расположение функции в константы
	// количество локальных переменных
	current_scope->fcode[pt] = (uint8_t)locals_count;
	// резерв стека
	current_scope->fcode[pt + 1] = (uint8_t)max_stack;
	ReturnOperator(VarType::None);
	CloseScope(); // возврат в старое окружение
}

// Вызов функции с аргументами / без аргументов

void Code::CallFunction(char *name, int args) {
	Serial.println(F("CallFunction"));
	int expected = GetArgs(name);
	if (args != expected);
		Serial.println(F("[CallFunction] args warning!"));
	if (args < 0)
		args = expected;
	int l = FindBuiltInFunction(nullptr, name); // поиск встроенной функции
	if (l != -1) { // *если нет такой функции
		Add_08(I_OP_LOADICONST);
		Add_16(0);
		Add_08(I_CALL_B);
		Add_08((uint8_t)args);
	} else { // поиск объявленной функции
		l = FindFunction(name);
		if (l == -1) { // *если не объявлена
			ErrorNotDeclaredFunction(name);
		} else {
			Add_08(I_OP_LOADICONST);
			Add_16(declared_functions[l].pointer);
			Add_08(I_CALL);
			Add_08((uint8_t)args);
		}
		if (current_scope->max_stack < 1) { // добавляет место для 1 переменной, если его не хватает
			current_scope->max_stack = 1;
		}
	}
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
	// сначала старший, затем младший
	current_scope->fcode[ifs_pt] = (uint8_t)(cpt & 0xFF); // jumpz
	current_scope->fcode[ifs_pt + 1] = (uint8_t)((cpt >> 8) & 0xFF); // для команды
}

void Code::CloseIfStatement(std::vector<size_t> labels) {
	int16_t cpt;
	size_t pos = current_scope->fcode.size();
	for (int i = 0; i < labels.size(); i++) {
		cpt = pos - labels[i];
		current_scope->fcode[labels[i]] = (uint8_t)(cpt & 0xFF);
		current_scope->fcode[labels[i] + 1] = (uint8_t)((cpt >> 8) & 0xFF);
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
	int16_t jumpto = condition - current_scope->fcode.size() - 1;
	Add_08(I_OP_JMP); // возврат к проверке условия
	Add_08(jumpto & 0xFF);
	Add_08((jumpto >> 8) & 0xFF);
	int16_t cpt = current_scope->fcode.size() - ws_pt;
	current_scope->fcode[ws_pt] = (uint8_t)(cpt & 0xFF);
	current_scope->fcode[ws_pt + 1] = (uint8_t)((cpt >> 8) & 0xFF); // jmpz
}

// ### Массивы ###

void Code::ArrayDeclaration(char *name, VarType type) {
	int t = FindVarType(type);
	if (var_types_info[t].size) { // 32x
		CodeConstInt(2); // побитовый сдвиг влево на 2 вместо умножения на 4
		Add_08(I_OP_ISHL);
		Add_08(I_OP_ALLOC);
	}
	else // 8x
		Add_08(I_OP_ALLOC);
	DeclareLocal(name, VarType::Array); // объявляет переменную и сохраняет в неё ид массива
	StoreLocal(name);
}

size_t Code::ArrayUnknownLengthStart() {
	Add_08(I_OP_LOADICONST);
	size_t pos = current_scope->fcode.size();
	Add_08(0);
	return pos;
}

void Code::ArrayUnknownLengthEnd(size_t pos, int len) {
	int _const = AddConstInt(len);
	current_scope->fcode[pos] = _const;
}

void Code::LoadArray(char *name) {
	LoadLocal(name);
	Add_08(I_OP_IALOAD);
}

void Code::StoreArray(char *name) {
	LoadLocal(name);
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


