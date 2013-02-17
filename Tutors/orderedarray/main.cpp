
#include <iostream>
#include <string>

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#if 0
// sima int-es változat
#include "orderedarray.h"

typedef orderedarray orderedarray_t;
#else
// sablonos változat
#include "../mystl/orderedarray.hpp"
#include "../mystl/orderedmultiarray.hpp"

typedef mystl::orderedarray<int> orderedarray_t;
#endif

struct test_struct
{
	int i;
	std::string str;
	std::string str2;

	bool operator <(const test_struct& other) const {
		return (i > other.i);
	}
};

int main()
{
	{
		mystl::orderedmultiarray<test_struct, mystl::default_less<test_struct> > a;
		test_struct t;

		t.i = 5;
		t.str = "5";
		t.str2 = "alma";
		a.insert(t);

		t.i = 1;
		t.str = "1";
		t.str2 = "citrom";
		a.insert(t);

		t.i = 2;
		t.str = "2";
		t.str2 = "kiwi";
		a.insert(t);

		t.i = 7;
		t.str = "7";
		t.str2 = "dio";
		a.insert(t);

		t.i = 3;
		t.str = "3";
		t.str2 = "banan";
		a.insert(t);

		for( size_t i = 0; i < a.size(); ++i )
		{
			std::cout << a[i].str << ", " << a[i].str2 << "\n";
		}
	}

    {
        orderedarray_t a1;

        // insert a végére
        a1.insert(2);
        a1.insert(6);
        a1.insert(10);

        // insert az elejére
        a1.insert(-5);
        a1.insert(-10);
        
        // dupla insert
        a1.insert(2);
        a1.insert(8);
        a1.insert(6);

        // dupla insert az elejére
        a1.insert(-10);
        a1.insert(-10);
        a1.insert(9);
        a1.insert(-10);
        
        // dupla insert a végére
        a1.insert(30);
        a1.insert(15);
        a1.insert(30);
        a1.insert(30);
        
        // copy konstruktor
        orderedarray_t a2 = a1;
        a2.insert(17);

        // operator =
        orderedarray_t a3;

        a3 = a2;
        a3.insert(-1);

        std::cout << "a1: " << a1 << "\na2: " << a2 << "\na3: " << a3 << "\n\n";

        // keresés
        size_t ind = a2.find(2);

        if( ind == orderedarray_t::npos )
            std::cout << "nincs benne a 2\n";
        else
        {
            const orderedarray_t& a4 = a2;
            std::cout << "a2[a2.find(2)] == " << a4[ind] << "\n";
        }

        ind = a2.find(-10);

        if( ind == orderedarray_t::npos )
            std::cout << "nincs benne a -10\n";
        else
            std::cout << "benne van a -10\n";

        ind = a2.find(30);

        if( ind == orderedarray_t::npos )
            std::cout << "nincs benne a 30\n";
        else
            std::cout << "benne van a 30\n";

        // törlés
        std::cout << "erase(8)\n";
        a2.erase(8);

        ind = a2.find(8);

        if( ind == orderedarray_t::npos )
            std::cout << "nincs benne a 8\n";
        else
            std::cout << "benne van a 8\n";

        std::cout << "erase(-10)\n";
        a2.erase(-10);
        
        std::cout << "erase(30)\n";
        a2.erase(30);
        a2.erase(30);

        std::cout << a2 << "\n";

        // clear
        a2.clear();
        std::cout << "\na2.clear()\na2.size() == " << a2.size() << "\n";

        ind = a2.find(30);

        if( ind == orderedarray_t::npos )
            std::cout << "nincs benne a 30\n";
        else
            std::cout << "benne van a 30\n";

        a2.erase(10);
    }

    // memory leakek
    _CrtDumpMemoryLeaks();

    system("pause");
    return 0;
}
