
#include <iostream>
#include <string>

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include "mtlist.hpp"

struct Apple
{
	std::string name;
};

int main()
{
	{
		typedef mtlist<TL3(int, float, Apple)> meta3_ifA;
		meta3_ifA l1;

		Apple a;
		a.name = "apple";

		l1.push_back(a);
		l1.push_back(5);
		l1.push_back(3.5f);
		l1.push_back(2);
		l1.push_back(0.1f);

		int cnt = 0;

		for( meta3_ifA::iterator it = l1.begin(); it != l1.end(); ++it )
		{
			if( cnt == 0 )
				std::cout << it.get<Apple>().name << "\n";
			if( cnt == 1 || cnt == 3 )
				std::cout << it.get<int>() << "\n";
			else if( cnt == 2 || cnt == 4 )
				std::cout << it.get<float>() << "\n";

			++cnt;
		}
	}

	_CrtDumpMemoryLeaks();

	system("pause");
	return 0;
}
