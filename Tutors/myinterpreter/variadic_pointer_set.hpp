//=============================================================================================================
#ifndef _VARIADIC_POINTER_SET_HPP_
#define _VARIADIC_POINTER_SET_HPP_

#include <set>

class variadic_pointer_set
{
	struct base_item
	{
		virtual ~base_item() {}

		virtual void tidy() = 0;
		virtual void* ptr() = 0;
	};

	template <typename value_type>
	struct item;

	struct compare
	{
		bool operator ()(base_item* a, base_item* b) const {
			return (a->ptr() < b->ptr());
		}
	};

	typedef std::set<base_item*, compare> container;

private:
	container values;

public:
	variadic_pointer_set();
	~variadic_pointer_set();

	void clear();
	void erase(void* ptr);
	
	template <typename value_type>
	void insert(value_type* ptr) {
		base_item* bi = new item<value_type>(ptr);
		container::iterator it = values.find(bi);

#if defined(_MSC_VER) && defined(_DEBUG)
		// If the program stops here that means that you
		// used 'delete' instead of 'interpreter->Deallocate()'.
		// In a worse case it wont stop here but in clear().
		if( it != values.end() )
			::_CrtDbgBreak();
#endif

		values.insert(bi);
	}
};

template <typename value_type>
struct variadic_pointer_set::item : variadic_pointer_set::base_item
{
	value_type* value;

	item(value_type* ptr)
		: value(ptr) {}

	void tidy();

	inline void* ptr() {
		return value;
	}
};

template <typename value_type>
void variadic_pointer_set::item<value_type>::tidy()
{
	if( value )
	{
		delete value;
		value = 0;
	}
}

#endif
//=============================================================================================================
