/*
**
*/

#pragma once
#ifndef SCR_DEF_H
#define SCR_DEF_H

#include <vector>
#include "scr_parser.h"
#include "scr_vm.h"
#include "scr_types.h"

class LangState {
	public:
		LangState(void);
		void Compile(int input_type, std::string s);
		void LoadString(std::string s);
		void LoadFile(std::string s);
		uint8_t Run(void);
	private:
		Parser *parser_obj;
		VM *vm;
		uint8_t *code_base;
		num_t *consts;
};


#endif