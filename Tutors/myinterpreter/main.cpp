
#include <iostream>
#include "interpreter.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

int main()
{
	{
		Interpreter ip;

		//ip.Compile("programs/scopes.p");
		//ip.Compile("programs/arithmetics.p");
		//ip.Compile("programs/helloworld.p");
		//ip.Compile("programs/factorial.p");
		//ip.Compile("programs/lnko.p");
		ip.Compile("../myinterpreter/programs/bigtest.p");
		ip.Link();

		std::cout << "\n";
		ip.Disassemble();

		std::cout << "\n";
		ip.Run();
	}

	_CrtDumpMemoryLeaks();

	system("pause");
	return 0;
}

