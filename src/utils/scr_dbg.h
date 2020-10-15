
#pragma once
#ifndef SCR_DBG_H
#define SCR_DBG_H

const char* exc_msg[] {
	"None",
	"Unexpected exception",
	""
};

class DeclaredException
{
	public:
		enum class Type {
			LexicalAnalyzer,
			Parser,
		};
		enum class Id {
			None,
			Unexpected,
			EOFException,
			
		};
		DeclaredException(Type _type, Id _id);
		Type type;
		Id id;
};

class ExceptionHandler
{
	public:
		DeclaredException LastEx;
};

#endif