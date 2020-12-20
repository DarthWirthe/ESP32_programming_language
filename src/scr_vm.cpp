
/** scr_vm.cpp
*** Виртуальная машина
*/

#include "scr_vm.h"
#include "utils/scr_debug.h"
#include "HardwareSerial.h"

// Макросы для работы со стеком

#define PUSH(v) 	 *++sp = (v)
#define POP() 		 (*sp--)
#define TOP()		 (*sp)
#define SET_TOP(v)	 *sp = (v)

// Макрос для ошибок

#define VM_ERR(err) throw (err)

// Макрос для отладки

#define VM_DBG

#ifdef VM_DBG
	#define DEBUGF(__f, ...) __DEBUGF__(__f, ##__VA_ARGS__)
#else
	#define DEBUGF(__f, ...)
#endif // VM_DBG

// Полезные функции

static inline int_t uint_to_int32(uint32_t val) {
	if (val & (IMMEDIATE_MASK_32 >> 1))
		val |= IMMEDIATE_MASK_32;
	return val;
}

static inline short_t uint_to_short16(uint16_t val) {
	if (val & (IMMEDIATE_MASK_16 >> 1))
		val |= IMMEDIATE_MASK_16;
	return val;
}

static inline stack_t float_to_stack(float val) {
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

static inline float stack_to_float(stack_t val) {
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


VM::VM(){}

VM::VM(uint16_t h_size, uint8_t *ptcode, num_t *nums, char *strs, int g_count) {
	heap = new Heap(h_size);
	code_base = ptcode;
	consts_num_base = nums;
	consts_str_base = strs;
	globals_count = g_count;
}

stack_t VM::LoadString(char *c) {
	size_t length = strlen(c);
	uint32_t id = heap->alloc(length);
	return id;
}

void stack_dump(stack_t *sb, stack_t *sp) {
	uint32_t stack_length = (uint32_t)(sp - sb + 1);
	DEBUGF("stack length: %u (%u bytes)\n", stack_length, stack_length * 4);
	DEBUGF("stack dump: ");
	while (sb <= sp) {
		DEBUGF("%u ", *sb);
		sb++;
	}
	DEBUGF("\n");
}

/**
  * pc - указатель программы
  * sp - указатель стека
  * fp - указатель кадра стека
  * locals_pt - указатель на локальные переменные
  * globals_pt - указатель на глобальные переменные
**/

int VM::Run() {
	stack_t *stack_base = (stack_t*)(heap->heap); // основание стека
	uint8_t instr, *pc; // указатель программы
	stack_t *sp = stack_base - 1; // указатель стека
	stack_t *fp = 0; // указатель кадра стека
	uint32_t tmp1, tmp2; // без знака
	int_t i0, i1; // со знаком
	float f0, f1; // вещественные числа
	stack_t *ptr0s, *ptr1s, *ptr2s, *locals_pt, *globals_pt;
	num_t *consts_n = consts_num_base; // указатель на числовые константы
	char *consts_s = consts_str_base; // указатель на строковые константы
	pc = code_base;
	heap->mem_steal(sizeof(stack_t) * (globals_count + 1));

	DEBUGF("#Starting virtual machine [%.3f kB available]\n", (float)heap->get_free_size() / 1024);

	do {
		instr = *pc++;
		switch (instr) {
			case I_OP_NOP:
			break;
			case I_PUSH: {
				DEBUGF("I_PUSH\n");
				PUSH(*((stack_t*)pc));
				pc += 4;
			break;
			}
			case I_POP: {
				DEBUGF("I_POP\n");
				POP();
			break;
			}
			// Вызов функции
			case I_CALL: {
				DEBUGF("CALL\n");
				tmp1 = POP(); // адрес функции берётся из стека
				tmp2 = pc[0]; // кол-во аргументов берётся из кода
				uint8_t *return_pt = pc + 1; // запоминить текущий указатель
				pc = (uint8_t*)(code_base + tmp1); // в месте перехода должен быть FUNC_PT
				if (pc[0] != (uint8_t)I_FUNC_PT) // если его нет, то сообщение об ошибке
					VM_ERR(VM_ERR_EXPECTED_FUNCTION);
				uint8_t *func_ref = pc; // запомнить pc
				tmp1 = pc[1]; // tmp1 - кол-во переменных
				sp -= tmp2; // добавляет аргументы в локальные переменные
				locals_pt = sp + 1; // запомнить расположение переменных
				DEBUGF("return_pt = %u\n", (uint32_t)return_pt);
				DEBUGF("[CALL] Args = %u\n", tmp2);
				DEBUGF("[CALL] Locals = %u\n", tmp1);
				DEBUGF("[CALL] Max stack = %u\n", pc[2]);
				DEBUGF("NewStackFrame\nheap->steal %u bytes (%u bytes free)\n",
				sizeof(stack_t) * (FRAME_REQUIREMENTS + tmp1 + pc[2]), heap->get_free_size());
				// ...[[аргументы][локальные переменные]][пред.кадр][адр.возвр.][нач.кадра][функция]...
				// увеличение стека: 4 байта данные для возврата, локальные переменные, аргументы, расширение
				heap->mem_steal(sizeof(stack_t) * (FRAME_REQUIREMENTS + tmp1 + pc[2]));
				sp += tmp1; // создаётся место под локальные переменные
				PUSH((stack_t)fp); // адрес предыдущего кадра (=указатель)
				fp = sp;
				PUSH((stack_t)return_pt); // адрес возврата указателя (=указатель + 4 байта)
				PUSH((stack_t)locals_pt); // начало кадра (=указатель + 8 байт)
				PUSH((stack_t)func_ref); // конец кадра (=указатель + 12 байт)
				stack_dump(stack_base, sp);
				pc += 3;
			break;
			}
			case I_CALL_B: {
				DEBUGF("CALL_B\n");
				tmp1 = pc[0]; // кол-во аргументов берётся из кода
				this->_sp = sp; // указатель передаётся для функций
				/* Вызов функции */
				sp = this->_sp; // функции могли изменить указатель
			break;
			}
			case I_OP_RETURN:
			case I_OP_IRETURN:
			case I_OP_FRETURN: {
				DEBUGF("RETURN\n");	
				if (instr == I_OP_IRETURN || instr == I_OP_FRETURN)
					tmp2 = POP(); // запомнить возвращаемое значение
				DEBUGF("fp = %u\n", (uint32_t)fp);
				DEBUGF("Before: ");
				stack_dump(stack_base, sp);
				DEBUGF("[RETURN] current fp = %u\n", (uint32_t)fp);
				DEBUGF("[RETURN] fp = %u\n", fp[0]);
				DEBUGF("[RETURN] return_pt = %u\n", fp[1]);
				DEBUGF("[RETURN] locals_pt = %u\n", fp[2]);
				DEBUGF("[RETURN] stack_end = %u\n", (uint32_t(heap->get_relative_base() - 4)));
				DEBUGF("stack block length = %u\n", 
				(uint32_t)heap->get_relative_base() - (uint32_t)stack_base - 4);
				if (fp[0]) {
					ptr0s = (stack_t*)fp[2] - 1; // указатель на конец пред. кадра
					ptr1s = (stack_t*)(fp - *((uint8_t*)(fp[3] + 1))); // указатель на начало кадра
					ptr2s = (stack_t*)(fp + FRAME_REQUIREMENTS + *((uint8_t*)(fp[3] + 2))); // указатель на конец кадра
					pc = (uint8_t*)fp[1]; // перенос указателя программы обратно
					fp = (stack_t*)fp[0]; // перенос указателя кадра обратно
					locals_pt = (stack_t*)fp[2]; // используем старые переменные
				} else {
					// возврат из первого окружения = конец программы
					DEBUGF("End of program!\n");
					goto vm_end;
				}
				// %%%%%% возврат в предыдущую функцию
				sp = ptr0s; // удаление кадра
				heap->mem_unsteal(sizeof(stack_t) * (ptr2s - ptr1s)); // возвращает память в кучу
				DEBUGF("heap->unsteal %u bytes (%u bytes free)\n", 
				sizeof(stack_t) * (ptr2s - ptr1s), heap->get_free_size());
				DEBUGF("After: ");
				stack_dump(stack_base, sp);
				DEBUGF("stack block length: %u\n",
				(uint32_t)heap->get_relative_base() - (uint32_t)stack_base - 4);
				if (instr == I_OP_IRETURN || instr == I_OP_FRETURN)
					PUSH(tmp2);
			break;
			}
			// Арифметические и логические операции с целыми числами
			case I_OP_IADD: DEBUGF("OP_IADD\n");
				i0 = static_cast<int>(POP());
				*sp = static_cast<int>(*sp) + i0;
			break;
			case I_OP_ISUB: DEBUGF("OP_ISUB\n");
				i0 = static_cast<int>(POP());
				*sp = static_cast<int>(*sp) - i0;
			break;
			case I_OP_IMUL: DEBUGF("OP_IMUL\n");
				i0 = static_cast<int>(POP());
				*sp = static_cast<int>(*sp) * i0;
			break;
			case I_OP_IDIV: DEBUGF("OP_IDIV\n");
				i0 = static_cast<int>(POP());
				*sp = static_cast<int>(*sp) / i0;
			break;
			case I_OP_IREM: DEBUGF("OP_IREM\n");
				i0 = static_cast<int>(POP());
				*sp = static_cast<int>(*sp) % i0;
			break;
			case I_OP_ISHL:
			case I_OP_ISHR:
			case I_OP_IAND:
			case I_OP_IOR:
			case I_OP_IBAND:
			case I_OP_IBOR:
			case I_OP_IXOR:
			case I_OP_LESS:
			case I_OP_LESSEQ:
			case I_OP_EQ:
			case I_OP_NOTEQ:
			case I_OP_GR:
			case I_OP_GREQ:
				i0 = static_cast<int>(POP());
				i1 = static_cast<int>(*sp);
				switch (instr) {
					case I_OP_ISHL: DEBUGF("OP_ISHL\n"); // сдвиг влево
						*sp = i1 << i0; break;
					case I_OP_ISHR: DEBUGF("OP_ISHR\n"); // сдвиг вправо
						*sp = i1 >> i0; break;
					case I_OP_IAND: DEBUGF("OP_IAND\n"); // и - логическое
						*sp = i1 && i0; break;
					case I_OP_IOR: DEBUGF("OP_IOR\n"); // или - логическое
						*sp =  i1 || i0; break;
					case I_OP_IBAND: DEBUGF("OP_IBAND\n"); // и - побитовое
						*sp = i1 & i0; break;
					case I_OP_IBOR: DEBUGF("OP_IBOR\n"); // или - побитовое
						*sp = i1 | i0; break;
					case I_OP_IXOR: DEBUGF("OP_IXOR\n");// искл. или - побитовое
						*sp = i1 ^ i0; break;
					case I_OP_LESS: DEBUGF("OP_LESS\n"); // меньше
						*sp = i1 < i0; break;
					case I_OP_LESSEQ: DEBUGF("OP_LESSEQ\n"); // меньше или равно
						*sp = i1 <= i0; break;
					case I_OP_EQ: DEBUGF("OP_EQ\n"); // равно
						*sp = i1 == i0; break;
					case I_OP_NOTEQ: DEBUGF("OP_NOTEQ\n"); // не равно
						*sp = i1 != i0; break;
					case I_OP_GR: DEBUGF("OP_GR\n"); // больше
						*sp = i1 > i0; break;
					case I_OP_GREQ: DEBUGF("OP_GREQ\n"); // больше или равно
						*sp = i1 >= i0; break;
				}
			break;
			// Арифметические операции с вещественными числами
			case I_OP_FADD:
			case I_OP_FSUB:
			case I_OP_FMUL:
			case I_OP_FDIV:
			case I_OP_FREM:
				f0 = static_cast<float>(POP());
				f1 = static_cast<float>(*sp);
				switch (instr) {
					case I_OP_FADD: DEBUGF("OP_FADD\n"); // сложение
						f1 += f0; break;
					case I_OP_FSUB: DEBUGF("OP_FSUB\n"); // вычитание
						f1 -= f0; break;
					case I_OP_FMUL: DEBUGF("OP_FMUL\n"); // умножение
						f1 *= f0; break;
					case I_OP_FDIV: DEBUGF("OP_FDIV\n"); // деление
						f1 /= f0; break;
					case I_OP_FREM: DEBUGF("OP_FREM\n"); // остаток
						//f1 %= f0; 
						break;	
				}
				*sp = float_to_stack(f1);
			break;
			case I_OP_IBNOT: // отрицание
				DEBUGF("OP_IBNOT\n");
				i0 = ~(static_cast<int>(*sp));
				*sp = i0;
			break;
			case I_OP_INOT: // отрицание
				DEBUGF("OP_INOT\n");
				i0 = !(static_cast<int>(*sp));
				*sp = i0;
			break;
			case I_OP_IUNEG: // ун. минус
				DEBUGF("OP_IUNEG\n");
				i0 = -(static_cast<int>(*sp));
				*sp = i0;
			break;
			case I_OP_IUPLUS: // ун. плюс
				DEBUGF("OP_IUPLUS\n");
				i0 = static_cast<int>(POP());
				(i0 < 0)
				? PUSH(-i0)
				: PUSH(i0);
			break;
			case I_OP_FUNEG: // ун. минус для чисел с плавающей точкой
				DEBUGF("OP_FUNEG\n");
				f0 = static_cast<float>(*sp);
				*sp = -f0;
			break;
			case I_OP_FUPLUS: // ун. плюс для чисел с плавающей точкой
				DEBUGF("OP_FUPLUS\n");
				f0 = 0;
				(f0 < 0) 
				? PUSH(-f0) 
				: PUSH(f0);
			break;
			// Из переменных на вершину стека
			case I_OP_ILOAD:
				DEBUGF("OP_LOADLOCAL\n");
				tmp1 = *pc++; // номер переменной
				i0 = static_cast<int>(locals_pt[tmp1]);
				PUSH(i0); // значение
				DEBUGF("#%u = %d\n", tmp1, i0);
			break;
			// Из стека в переменные
			case I_OP_ISTORE:	
				DEBUGF("OP_STORELOCAL\n");
				tmp1 = *pc++; // номер переменной
				i0 = static_cast<int>(POP());
				locals_pt[tmp1] = i0; // значение
				DEBUGF("#%u = %d\n", tmp1, i0);
			break;
			// Из констант на вершину стека (целое число)
			case I_OP_LOADICONST:
				DEBUGF("OP_LOADICONST\n");
				tmp1 = *((uint16_t*)(pc)); // позиция
				DEBUGF("#%u = %d\n", tmp1, consts_n[tmp1].i);
				PUSH(consts_n[tmp1].i);
				pc += 2;
			break;
			// Из констант на вершину стека (вещественное число)
			case I_OP_LOADFCONST:
				DEBUGF("OP_LOADFCONST\n");
				tmp1 = *((uint16_t*)(pc)); // позиция
				DEBUGF("#%u = %f\n", tmp1, consts_n[tmp1].f);
				PUSH(consts_n[tmp1].f);
				pc += 2;
			break;
			case I_OP_LOADSTRING:
				DEBUGF("OP_LOADSTRING\n");
				*sp = LoadString(consts_s + *sp);
			break;
			// Безусловный переход
			case I_OP_JMP: // 16-битный сдвиг
				DEBUGF("OP_JMP\n");
				pc += uint_to_short16(*((uint16_t*)(pc))); // перенос указателя
				//DEBUGF("%d\n", i0);
			break;
			// Условный переход (если на вершине стека 0)
			case I_OP_JMPZ: // 16-битный сдвиг
				DEBUGF("OP_JMPZ\n");
				tmp1 = POP();
				if (tmp1 == 0)
					pc += uint_to_short16(*((uint16_t*)(pc))); // перенос указателя
				else
					pc += 2;
				//DEBUGF("%d\n", i0);
			break;
			// Условный переход (если на вершине стека не 0)
			case I_OP_JMPNZ: // 16-битный сдвиг
				DEBUGF("OP_JMPNZ\n");
				tmp1 = POP();
				if (tmp1 != 0)
					pc += uint_to_short16(*((uint16_t*)(pc))); // перенос указателя
				else
					pc += 2;
				//DEBUGF("%d\n", i0);
			break;
			case I_FUNC_PT:
			break;
			case I_OP_ITOF:
				DEBUGF("OP_ITOF\n");
				f0 = (float)uint_to_int32(POP());
				PUSH(f0);
				DEBUGF("%f\n", f0);
			break;
			case I_OP_FTOI:
				DEBUGF("OP_FTOI\n");
				i0 = (int_t)stack_to_float(POP());
				PUSH(i0);
				DEBUGF("%d\n", i0);
			break;
			case I_OP_GLOAD:
				
			break;
			case I_OP_GSTORE:
				
			break;
			case I_OP_LOAD0:
				DEBUGF("OP_LOAD0\n");
				PUSH(0);
			break;
			case I_OP_ILOAD1:
				DEBUGF("OP_ILOAD1\n");
				PUSH(1);
			break;
			// Выделение памяти
			case I_OP_ALLOC:
				DEBUGF("OP_ALLOC\n");
				tmp1 = POP(); // размер берётся из стека
				tmp2 = heap->alloc(tmp1);
				//*(uint8_t*)(heap->get_addr(tmp2)) = 0; // сохранить тип в первый байт
				DEBUGF("id = %u, size = %u\n", tmp2, tmp1);
				PUSH(tmp2); // ид переносится в стек
			break;
			// Изменение величины блока
			case I_OP_REALLOC:
				DEBUGF("OP_REALLOC\n");
				tmp1 = POP(); // размер из стека
				tmp2 = POP(); // ид из стека
				tmp2 = heap->alloc(tmp1);
				DEBUGF("id = %u, size = %u\n", tmp2, tmp1);
				heap->realloc(tmp1, tmp2);
			break;
			case I_OP_FREE:
				DEBUGF("OP_FREE\n");
				tmp1 = POP(); // ид из стека
				heap->free(tmp1);
			break;
			case I_OP_IALOAD: // загрузка 32-битного числа
				DEBUGF("OP_IALOAD\n");
				tmp1 = POP(); // ид из стека
				tmp2 = POP(); // индекс из стека
				DEBUGF("id = %u, index = %u\n", tmp1, tmp2);
				if (((tmp2 + 1) * 4) > heap->get_length(tmp1)) {
					VM_ERR(VM_ERR_INDEX_OUT_OF_RANGE); /*Индекс за пределами*/
				}
				PUSH(*((int_t*)heap->get_addr(tmp1) + tmp2));
			break;
			case I_OP_IASTORE: // запись 32-битного числа
				DEBUGF("OP_IASTORE\n");
				tmp1 = POP(); // ид из стека
				tmp2 = POP(); // индекс из стека
				i0 = uint_to_int32(POP()); // результат выражения из стека
				DEBUGF("id = %u, index = %u, value = %d\n", tmp1, tmp2, i0);
				if (((tmp2 + 1) * 4) > heap->get_length(tmp1)) {
					VM_ERR(VM_ERR_INDEX_OUT_OF_RANGE); /*Индекс за пределами*/
				}
				*((int_t*)heap->get_addr(tmp1) + tmp2) = i0;
			break;
			case I_OP_BALOAD: // загрузка 8-битного числа
				DEBUGF("OP_BALOAD\n");
				tmp1 = POP(); // ид из стека
				tmp2 = POP(); // индекс из стека
				if (tmp2 + 1 > heap->get_length(tmp1)) {
					VM_ERR(VM_ERR_INDEX_OUT_OF_RANGE); /*Индекс за пределами*/
				}
				PUSH(((uint8_t*)heap->get_addr(tmp1))[tmp2]);
			break;
			case I_OP_BASTORE: // запись 8-битного числа
				DEBUGF("OP_BASTORE\n");
				tmp1 = POP(); // ид из стека
				tmp2 = POP(); // индекс из стека
				i0 = uint_to_int32(POP()); // результат выражения из стека
				if (tmp2 + 1 > heap->get_length(tmp1)) {
					VM_ERR(VM_ERR_INDEX_OUT_OF_RANGE); /*Индекс за пределами*/
				}
				((uint8_t*)heap->get_addr(tmp1))[tmp2] = i0;
			break;
			default:
				throw VM_ERR_UNKNOWN_INSTR;
			break;
		}
	} while (1);
vm_end:
	DEBUGF("End");
	return VM_NOERROR;
}

void VM::StackPush(stack_t val) {
	*++this->_sp = val;
}

stack_t VM::StackPop() {
	return *this->_sp--;
}

void VM::CleanMemory() {
	heap->delete_heap();
}


