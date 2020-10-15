
/*
	Виртуальная машина
*/

#include "scr_vm.h"
#include "HardwareSerial.h"

/*****************************/

#define PUSH(val) *++sp = (val)
#define POP() (*sp--)
#define TOP() (*sp)
#define SET_TOP(val) *sp = (val)

int_t uint_to_int32(uint32_t val) {
	if(val & (IMMEDIATE_MASK_32 >> 1))
		val |= IMMEDIATE_MASK_32;
	return val;
}

short_t uint_to_short16(uint16_t val) {
	if(val & (IMMEDIATE_MASK_16 >> 1))
		val |= IMMEDIATE_MASK_16;
	return val;
}

stack_t float_to_stack(float val) {
	union_t v;
	v.f[0] = val;
	uint8_t msb = (v.b[3] & 0x80) ? 0x40 : 0x00;
	v.b[3] &= 0x7f;
	if (v.b[3] == 0x7f && (v.b[2] & 0x80) == 0x80)
		msb |= 0x3f;
	else if (v.b[3] != 0x00 || (v.b[2] & 0x80) != 0x00)
		msb |= v.b[3] - 0x20;
	v.b[3] = msb;
	return v.i[0];
}

float stack_to_float(stack_t val) {
	union_t v;
	v.i[0] = val;
	uint8_t msb = (v.b[3] & 0x40) ? 0x80 : 0x00;
	v.b[3] &= 0x3f;
	if (v.b[3] == 0x3f && (v.b[2] & 0x80) == 0x80)
		msb |= 0x7f;
	else if (v.b[3] != 0x00 || (v.b[2] & 0x80) != 0x00)
		msb |= v.b[3] + 0x20;
	v.b[3] = msb;
	return v.f[0];
}

// инициализация стека, возвращает указатель на стек

stack_t* stack_init() {
	return (stack_t*)heap_get_base();
	//heap_steal(DEFAULT_STACK_SIZE * sizeof(stack_t));
}

VM::VM( Parser obj ) {
	_parser_obj = obj;
}

template <typename T>
void DBG(T c) {
	Serial.println(c);
}

template <typename T>
void vm_error(T c) {
	Serial.println(c);
	throw "";
}

void stackdump(stack_t *sb, stack_t *sp) {
//###
	uint32_t stack_length = (uint32_t)(sp - sb) + 1;
	Serial.print("stack length: ");
	Serial.print(stack_length);
	Serial.print(" (");
	Serial.print(stack_length * 4);
	Serial.println(" bytes)");
	Serial.print("stack dump: ");
	uint32_t pt = (uint32_t)sp;
	for (uint32_t i = (uint32_t)sb; i <= pt; i+=sizeof(stack_t)) {
		Serial.print(sb[i]);
		Serial.print(" ");
	}
	Serial.println();
//###
}

// Цикл

void VM::Execute() {
	stack_t *stack_base = stack_init(); // основание стека
	register uint8_t *pc; // указатель программы
	register stack_t *sp = stack_base - 1; // указатель стека
	register stack_t *fp = nullptr; // указатель кадра стека
	register uint8_t instr; // регистр инструкции
	register uint32_t tmp1, tmp2; // беззнаковые числа
	register int_t i0, i1; // знаковые числа
	float f0, f1; // вещественные числа
	stack_t *ptr0, *ptr1, *locals_pt; // locals_pt - указатель на первую переменную в кадре стека
	uint8_t *return_pt;
	uint8_t *consts_pt;
	
	pc = code_base;
	
	DBG("Start");
	
	do {
		instr = pc[0];
		pc++;
		if (instr == I_OP_NOP) {
		}
		else if (instr == I_PUSH) {
			PUSH((uint32_t)pc[0]);
			pc += 4;
		}
		else if (instr == I_POP) {
			POP();
		}
		else if (instr == I_CALL) { //[call][args] -> [func_pt][locals][max_stack]
			DBG("CALL");
			tmp1 = POP(); // адрес функции берётся из стека
			tmp2 = pc[0]; // аргументы
/**/		Serial.println(tmp1);
			return_pt = pc + 1; // запоминить текущий указатель (+1 байт - кол-во аргументов)
/**/		Serial.print("return_pt = ");
/**/		Serial.println((uint32_t)return_pt);
			pc = (uint8_t*)(code_base + tmp1); // в месте перехода должен быть FUNC_PT
			if (pc[0] != (uint8_t)I_FUNC_PT) // если его нет, то сообщение об ошибке
				{vm_error("Error: Expected function");}
			tmp1 = pc[1]; // tmp1 - кол-во переменных
			sp -= tmp2; // добавляет аргументы в локальные переменные
			locals_pt = sp + 1; // запомнить расположение переменных
		// %%%%%%% Новый кадр [[аргументы][локальные переменные]][пред.кадр][адр.возвр.][нач.указатель][...]
/**/		DBG("NewStackFrame");
			// * увеличение стека: 3 байта данные для возврата, локальные переменные, аргументы, расширение
			heap_steal(sizeof(stack_t) * (FRAME_REQUIREMENTS + tmp1 + tmp2 + pc[2]));
/**/		Serial.print("heap_steal ");
/**/		Serial.print(FRAME_REQUIREMENTS + tmp1 + tmp2 + pc[2]);
/**/		Serial.print(" (");
/**/		Serial.print(heap_get_free_size());
/**/		Serial.println(" bytes free)");
			sp += tmp1; // создаёт место под локальные переменные
			PUSH((stack_t)fp); // адрес предыдущего кадра (=указатель)
			fp = sp;
			PUSH((stack_t)return_pt); // адрес возврата указателя (=указатель + 4 байта)
			PUSH((stack_t)locals_pt); // запомнить резерв, чтобы правильно удалить кадр (=указатель + 8 байт)
		// %%%%%%
/**/		stackdump(stack_base, sp);
			
			pc += 3;
		}
		else if (instr == I_CALL_B) {
			DBG("CALL_B");
		}
		else if (instr == I_OP_RETURN || 
		instr == I_OP_IRETURN ||
		instr == I_OP_FRETURN) {
			DBG("RETURN");
/**/		Serial.print("fp = ");
/**/		Serial.println((uint32_t)fp);
			if (instr == I_OP_IRETURN || instr == I_OP_FRETURN)
				tmp2 = POP(); // запомнить возвращаемое значение
/**/		Serial.print("Before: ");
/**/		stackdump(stack_base, sp);
			ptr0 = (stack_t*)(fp[2]) - 1; // указатель на начало кадра
			ptr1 = (stack_t*)(heap_get_relative_base()) - 2; // указатель на конец кадра
			Serial.print("stack block length: ");
			Serial.println(ptr1 - stack_base);
			pc = (uint8_t*)fp[1]; // перенос указателя программы обратно
			fp = (stack_t*)fp[0]; // перенос указателя кадра обратно
			if (fp) {
				//ptr1 = (stack_t*)(fp[2]); // указатель на конец старого кадра
				locals_pt = (stack_t*)fp[2]; // используем старые переменные
			}
			else { // возврат из первого окружения = конец программы
/**/			Serial.println("End of program!");
				break;
			}
		// %%%%%% возврат в предыдущую функцию
			sp = ptr0; // удаление кадра
			heap_unsteal(sizeof(stack_t) * (ptr1 - ptr0)); // возвращает память в кучу
/**/		Serial.print("heap_unsteal ");
/**/		Serial.print((uint32_t)(ptr1 - ptr0));
/**/		Serial.print(" (");
/**/		Serial.print(heap_get_free_size());
/**/		Serial.println(" bytes free)");
/**/		Serial.print("After: ");
/**/		stackdump(stack_base, sp);
/**/		ptr1 = (stack_t*)(heap_get_relative_base()) - 1; // убрать
/**/		Serial.print("stack block length: ");
/**/		Serial.println(ptr1 - stack_base);
		// %%%%%%
			if (instr == I_OP_IRETURN || instr == I_OP_FRETURN)
				PUSH(tmp2);
		}
		else if (instr >= I_OP_IADD && instr <= I_OP_GREQ) {
			i0 = uint_to_int32(POP()); // Только 32-битные числа со знаком
			i1 = uint_to_int32(POP());
			switch(instr) {
				case I_OP_IADD: DBG("OP_IADD"); // сложение	
					i1 += i0; break;
				case I_OP_ISUB: DBG("OP_ISUB"); // вычитание
					i1 -= i0; break;
				case I_OP_IMUL: DBG("OP_IMUL"); // умножение
					i1 *= i0; break;
				case I_OP_IDIV: DBG("OP_IDIV"); // деление
					i1 /= i0; break;
				case I_OP_IREM: DBG("OP_IREM"); // остаток
					i1 %= i0; break;
				case I_OP_ISHL: DBG("OP_ISHL"); // сдвиг влево
					i1 <<= i0; break;
				case I_OP_ISHR: DBG("OP_ISHR"); // сдвиг вправо
					i1 >>= i0; break;
				case I_OP_IAND: DBG("OP_IAND"); // и - побитовое
					i1 &= i0; break;
				case I_OP_IOR: DBG("OP_IOR"); // или - побитовое
					i1 |= i0; break;
				case I_OP_IXOR: DBG("OP_IXOR");// искл. или - побитовое
					i1 ^= i0; break;
				case I_OP_LESS: DBG("OP_LESS"); // меньше
					i1 = i1 < i0; break;
				case I_OP_LESSEQ: DBG("OP_LESSEQ"); // меньше или равно
					i1 = i1 <= i0; break;
				case I_OP_EQ: DBG("OP_EQ"); // равно
					i1 = i1 == i0; break;
				case I_OP_NOTEQ: DBG("OP_NOTEQ"); // не равно
					i1 = i1 != i0; break;
				case I_OP_GR: DBG("OP_GR"); // больше
					i1 = i1 > i0; break;
				case I_OP_GREQ: DBG("OP_GREQ"); // больше или равно
					i1 = i1 >= i0; break;
			}
			PUSH(i1);
		}
		else if (instr >= I_OP_FADD && instr <= I_OP_FREM) {
			f0 = stack_to_float(POP());
			f1 = stack_to_float(POP());
			switch(instr) {
				case I_OP_FADD: DBG("OP_FADD"); // сложение
					f1 += f0; break;
				case I_OP_FSUB: DBG("OP_FSUB"); // вычитание
					f1 -= f0; break;
				case I_OP_FMUL: DBG("OP_FMUL"); // умножение
					f1 *= f0; break;
				case I_OP_FDIV: DBG("OP_FDIV"); // деление
					f1 /= f0; break;
				case I_OP_FREM: DBG("OP_FREM"); // остаток
					//f1 %= f0; 
					break;	
			}
			PUSH(float_to_stack(f1));
		}
		else if (instr == I_OP_INOT) { // отрицание
			DBG("OP_INOT");
			PUSH(!uint_to_int32(POP()));
		}
		else if (instr == I_OP_IUNEG) { // ун. минус
			DBG("OP_IUNEG");
			PUSH(-uint_to_int32(POP()));
		}
		else if (instr == I_OP_IUPLUS) {
			DBG("OP_IUPLUS");
			i0 = uint_to_int32(POP());
			(i0 < 0) 
			? PUSH(-i0) 
			: PUSH(i0);
			// действия с унарным плюсом
		}
		else if (instr == I_OP_FUNEG) { // ун. минус для чисел с плавающей точкой
			DBG("OP_FUNEG");
			f0 = 0;//uint_to_int32(POP());
			PUSH(-f0);
		}
		else if (instr == I_OP_FUPLUS) { // ун. плюс для чисел с плавающей точкой
			DBG("OP_FUPLUS");
			f0 = 0;//uint_to_int32(POP());
			(f0 < 0) 
			? PUSH(-f0) 
			: PUSH(f0);
		}
		else if (instr == I_OP_LOAD) { // получение значения переменной
			tmp1 = pc[0]; // номер контекста
			tmp2 = pc[1]; // номер переменной
			pc += 2;
		}
		else if (instr == I_OP_LOADLOCAL) {
			DBG("OP_LOADLOCAL");
			tmp1 = pc[0]; // номер
			Serial.print("var #");
			Serial.println(tmp1);
			PUSH(locals_pt[tmp1]); // значение
			Serial.println(uint_to_int32(locals_pt[tmp1]));
			pc += 1;
		}
		else if (instr == I_OP_SET) {
			
		}
		else if (instr == I_OP_STORE) {
			tmp1 = pc[0]; // номер контекста
			tmp2 = pc[1]; // номер переменной
			pc += 2;
		}
		else if (instr == I_OP_STORELOCAL) {	
			DBG("OP_STORELOCAL");
			tmp1 = pc[0]; // номер переменной
			Serial.print("var #");
			Serial.println(tmp1);
			locals_pt[tmp1] = POP(); // значение
			Serial.println(uint_to_int32(locals_pt[tmp1]));
			pc += 1;
		}
		else if (instr == I_OP_LOADICONST) { // значение константы копируется на вершину стека
			DBG("OP_LOADICONST");
			tmp1 = pc[0]; //
			PUSH(consts[tmp1].i);
			Serial.println(consts[tmp1].i);
			pc += 1;
		}
		else if (instr == I_OP_LOADFCONST) { // значение константы копируется на вершину стека
			DBG("OP_LOADFCONST");
			tmp1 = pc[0]; //
			PUSH(consts[tmp1].f);
			Serial.println(consts[tmp1].f);
			pc += 1;
		}
		else if (instr == I_OP_JMP) { // 16-битный сдвиг
			DBG("OP_JMP");
			tmp1 = pc[0] << 8 | pc[1];
			i0 = uint_to_short16(tmp1);
			pc = (uint8_t*)((int)pc + i0); // получить относительный адрес для переноса указателя
			Serial.println(i0);
		}
		else if (instr == I_OP_JMPZ) { // если на вершине стека 0, 16-битный сдвиг
			DBG("OP_JMPZ");
			tmp1 = POP();
			tmp2 = pc[0] << 8 | pc[1];
			i0 = uint_to_short16(tmp2);
			if (tmp1 == 0) {
				pc = (uint8_t*)((int)pc + uint_to_short16(i0)); // получить относительный адрес для переноса указателя
			}
			else
				pc += 2;
			Serial.println(i0);
		}
		else if (instr == I_OP_JMPNZ) { // если на вершине стека не 0, 16-битный сдвиг
			DBG("OP_JMPNZ");
			tmp1 = POP();
			tmp2 = pc[0] << 8 | pc[1];
			i0 = uint_to_short16(tmp2);
			if (tmp1 != 0) {
				pc = (uint8_t*)((int)pc + uint_to_short16(i0)); // получить относительный адрес для переноса указателя
			}
			else
				pc += 2;
			Serial.println(i0);
		}
		else if (instr == I_FUNC_PT) {
		}
		else if (instr == I_OP_NEWARRAY) { // массив неопределённого типа
			tmp1 = POP(); // длина массива из стека(байт)
			PUSH((uint32_t)heap_alloc(tmp1)); // ид массива в стек
		}
		else if (instr == I_OP_IALOAD) { // только int
			tmp1 = POP(); // ид из стека
			tmp2 = POP(); // индекс из стека
			PUSH(((int_t*)heap_get_addr(tmp1))[tmp2]);
		}
		else if (instr == I_OP_IASTORE) { // только int // mas[4-2] = 5 + 5;
			i0 = uint_to_int32(POP()); // результат выражения из стека
			tmp1 = POP(); // ид из стека
			tmp2 = POP(); // индекс из стека
			((int_t*)heap_get_addr(tmp1))[tmp2] = i0;
		}
		else if (instr == I_OP_BALOAD) { // только 8-битные числа
			tmp1 = POP(); // ид из стека
			tmp2 = POP(); // индекс из стека
			PUSH(((uint8_t*)heap_get_addr(tmp1))[tmp2]);
		}
		else if (instr == I_OP_BASTORE) { // только 8-битные числа // mas[0] = "m";
			i0 = uint_to_int32(POP()); // результат выражения из стека
			tmp1 = POP(); // ид из стека
			tmp2 = POP(); // индекс из стека
			((uint8_t*)heap_get_addr(tmp1))[tmp2] = i0;
		}
		else {
			DBG("Error: Unknown instruction");
			DBG(instr);
		}
	} while(
		instr <= I_OP_JMPNZ
	);
	DBG("End");
}

// ######################

void VM::Assembly()
{
	try
	{
		_parser_obj.Parse();
		code_base = _parser_obj.GetCode();
		consts = _parser_obj.GetConsts();
	}
	catch(const char *exc)
	{
		
	}
}

void VM::Interpret()
{
	Assembly();
	try
	{
		heap_init();
		//stack.Init();
		Execute();
	}
	catch(const char *exc)
	{
		
	}
	
}


