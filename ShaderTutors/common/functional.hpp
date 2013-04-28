
#ifndef _FUNCTIONAL_HPP_
#define _FUNCTIONAL_HPP_

#include <iostream>

#define myerror(r, e, x) { if( !(x) ) { std::cout << "* MYSTL ERROR: " << e << "\n"; return r; } }
#define mynerror(r, e, x) { if( (x) ) { std::cout << "* MYSTL ERROR: " << e << "\n"; return r; } }

namespace mystl
{
	template <typename T>
	struct default_less
	{
		inline bool operator ()(const T& a, const T& b) const {
			return a < b;
		}
	};
}

#endif
