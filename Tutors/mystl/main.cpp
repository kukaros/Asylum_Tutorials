
#include <iostream>
#include <string>

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

#include "list.hpp"
#include "orderedarray.hpp"

#define DEBUG_METHOD(x)		std::cout << #x << "\n"; x;
#define ASSERT(x)			{ if( !(x) ) { std::cout << "ASSERTION FAILED: " << #x << "\n"; } }
#define SECTION(x)			std::cout << "\n\n" << x << "\n---------------------------------\n\n";

template <typename container_t>
void write(const std::string& name, const container_t& cont)
{
	std::cout << name << ": ";

	for( container_t::const_iterator it = cont.begin(); it != cont.end(); ++it )
		std::cout << *it << " ";

	std::cout << "\n";
}

int main()
{
	{
		SECTION("orderedarray: simple insertion");
		mystl::orderedarray<int> a1;
		size_t ind;

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
		mystl::orderedarray<int> a2 = a1;
		a2.insert(17);

		// operator =
		mystl::orderedarray<int> a3;

		a3 = a2;
		a3.insert(-1);

		std::cout << "a1: " << a1 << "\na2: " << a2 << "\na3: " << a3 << "\n";

		// keresés
		SECTION("orderedarray: find");
		{
			DEBUG_METHOD(ind = a2.find(2));

			if( ind == mystl::orderedarray<int>::npos )
				std::cout << "nincs benne a 2\n";
			else
			{
				const mystl::orderedarray<int>& a4 = a2;
				std::cout << "a2[a2.find(2)] == " << a4[ind] << "\n";
			}

			DEBUG_METHOD(ind = a2.find(-10));

			if( ind == mystl::orderedarray<int>::npos )
				std::cout << "nincs benne a -10\n";
			else
				std::cout << "benne van a -10\n";

			DEBUG_METHOD(ind = a2.find(30));

			if( ind == mystl::orderedarray<int>::npos )
				std::cout << "nincs benne a 30\n";
			else
				std::cout << "benne van a 30\n";
		}

		// törlés
		SECTION("orderedarray: erase");
		{
			std::cout << "erase(8)\n";
			a2.erase(8);

			DEBUG_METHOD(ind = a2.find(8));

			if( ind == mystl::orderedarray<int>::npos )
				std::cout << "nincs benne a 8\n";
			else
				std::cout << "benne van a 8\n";

			DEBUG_METHOD(a2.erase(-10));
			DEBUG_METHOD(a2.erase(30));
			DEBUG_METHOD(a2.erase(30));

			std::cout << "a2: " << a2 << "\n";
		}

		// clear
		SECTION("orderedarray: erase");
		{
			DEBUG_METHOD(a2.clear());
			std::cout << "\na2.size() == " << a2.size() << "\n";

			DEBUG_METHOD(ind = a2.find(30));

			if( ind == mystl::orderedarray<int>::npos )
				std::cout << "nincs benne a 30\n";
			else
				std::cout << "benne van a 30\n";

			a2.erase(10);
		}
	}

	{
		SECTION("list: simple insertion");

		mystl::list<int> l1;

		l1.push_back(10);
		l1.push_back(5);
		l1.push_back(1);
		l1.push_back(-7);
		l1.push_back(8);

		ASSERT(l1.front() == 10);
		ASSERT(l1.back() == 8);

		write("l1", l1);

		DEBUG_METHOD(l1.clear());
		write("l1", l1);

		SECTION("list: insert at front");

		l1.push_front(10);
		l1.push_front(3);
		l1.push_back(5);
		l1.push_front(6);
		l1.push_back(-1);

		write("l1", l1);

		SECTION("list: resize & copy");

		DEBUG_METHOD(l1.resize(10, 25));
		write("l1", l1);

		DEBUG_METHOD(mystl::list<int> l2(l1));
		mystl::list<int> l3;

		l3.push_back(34);
		l3.push_back(11);

		ASSERT(l2.front() == l1.front());
		ASSERT(l2.back() == l1.back());
		ASSERT(l3.back() == 11);

		DEBUG_METHOD(l2.push_front(2));
		DEBUG_METHOD(l3 = l1);

		write("l2", l2);
		write("l3", l3);

		SECTION("list: random insertion");
		{
			DEBUG_METHOD(l2.insert(l2.begin(), 55));
			DEBUG_METHOD(l2.insert(l2.end(), 44));

			ASSERT(l2.front() == 55);
			ASSERT(l2.back() == 44);

			write("l2", l2);

			mystl::list<int>::iterator it = l2.begin();

			for( int i = 0; i < 4; ++i )
				++it;

			std::cout << "*it == " << *it << "\n";

			DEBUG_METHOD(l2.insert(it, 3));
			DEBUG_METHOD(l2.insert(it, 5));
			DEBUG_METHOD(l2.insert(it, 1));

			write("l2", l2);
		}

		SECTION("list: iterators");
		{
			mystl::list<int>::iterator it1, it2;

			it1 = l1.begin();
			it2 = l2.end();

			DEBUG_METHOD(it1 == it2);
			DEBUG_METHOD(it1 != it2);
			DEBUG_METHOD(*it2);
			DEBUG_METHOD(++it2);
			DEBUG_METHOD(it2++);
			DEBUG_METHOD(--it1);
			DEBUG_METHOD(it1--);
		}

		SECTION("list: erase & remove");
		{
			write("l2", l2);

			DEBUG_METHOD(l2.erase(l2.begin()));
			DEBUG_METHOD(l2.erase(l2.end()));

			write("l2", l2);

			mystl::list<int>::iterator it = l2.begin();

			for( int i = 0; i < 4; ++i )
				++it;

			std::cout << "*it == " << *it << "\n";

			DEBUG_METHOD(l2.erase(it));
			DEBUG_METHOD(l2.erase(it));

			write("l2", l2);

			DEBUG_METHOD(l2.remove(25));
			write("l2", l2);
		}
	}

	{
		SECTION("list: types");

		class Apple
		{
		private:
			int i;

		public:
			Apple() {
				i = 0;
			}

			Apple(int j) {
				i = j;
			}

			void foo() {
				std::cout << "Apple::foo(): i == " << i << "\n";
			}
		};
        
		typedef mystl::list<Apple> applelist;
		typedef mystl::list<Apple*> appleptrlist;

		applelist apples;
		appleptrlist appleptrs;

		apples.push_back(6);
		apples.push_back(10);
		apples.push_front(-4);
		apples.push_back(5);

		appleptrs.push_back(new Apple(20));
		appleptrs.push_back(new Apple(1));
		appleptrs.push_front(new Apple(-4));
		appleptrs.push_back(new Apple(-3));
		appleptrs.push_front(new Apple(6));

		std::cout << "apples:\n";

		for( applelist::iterator it = apples.begin(); it != apples.end(); ++it )
			it->foo();

		std::cout << "\nappleptrs:\n";

		for( appleptrlist::iterator it = appleptrs.begin(); it != appleptrs.end(); ++it )
		{
			(*it)->foo();
			delete (*it);
		}
	}

	std::cout << "\n";
	_CrtDumpMemoryLeaks();

	system("pause");
	return 0;
}
