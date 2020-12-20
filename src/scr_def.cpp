
#include "scr_def.h"
#include "HardwareSerial.h"

LangState::LangState() {
	heap_size = 256;
}

void LangState::Compile(int input_type, std::string s) {
	Lexer *l = new Lexer(input_type, s);
	parser_obj = new Parser(l);
	int exit_code = parser_obj->Parse();
	code_base = parser_obj->GetCode();
	consts = parser_obj->GetConsts();
}

int LangState::Run() {
	int vm_exit = 0;
	vm = new VM(heap_size, code_base, consts, nullptr, 0);
	try {
		vm_exit = vm->Run();
	} catch (int code) {
		vm_exit = code;
	}
	return vm_exit;
}

void LangState::LoadString(std::string s) {
	Compile(LEX_INPUT_STRING, s);
}

void LangState::LoadFile(std::string s) {
	Compile(LEX_INPUT_FILE, s);
}

