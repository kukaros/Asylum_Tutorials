
#include "interpreter.h"
#include <iostream>

extern Interpreter* interpreter;

void Interpreter::Print_Reg(void* arg1, void* arg2)
{
	int reg = reinterpret_cast<int>(arg1);
	std::cout << interpreter->registers[reg];
}

void Interpreter::Print_Memory(void* arg1, void* arg2)
{
	std::string* str = reinterpret_cast<std::string*>(arg1);

	if( str )
		std::cout << *str;
}
