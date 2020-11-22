
#include "scr_def.h"
#include "HardwareSerial.h"

LangState::LangState() {
	
}

void LangState::Compile(int input_type, std::string s) {
	Lexer *l = new Lexer(input_type, s);
	parser_obj = new Parser(l);
	parser_obj->Parse();
	code_base = parser_obj->GetCode();
	consts = parser_obj->GetConsts();
}

uint8_t LangState::Run() {
	vm = new VM(256, code_base, consts);
	vm->Run();
	return 0;
}

void LangState::LoadString(std::string s) {
	Compile(LEX_INPUT_STRING, s);
}

void LangState::LoadFile(std::string s) {
	Compile(LEX_INPUT_FILE, s);
}

