//=============================================================================================================
#include "variadic_pointer_set.hpp"

variadic_pointer_set::variadic_pointer_set()
{
}

variadic_pointer_set::~variadic_pointer_set()
{
	clear();
}
//=============================================================================================================
void variadic_pointer_set::clear()
{
	for( container::iterator it = values.begin(); it != values.end(); ++it )
	{
		(*it)->tidy();
		delete (*it);
	}

	values.clear();
}
//=============================================================================================================
void variadic_pointer_set::erase(void* ptr)
{
	item<void> tmp(ptr);
	container::iterator it = values.find(&tmp);

	if( it != values.end() )
	{
		base_item* bi = (*it);
		values.erase(it);

		bi->tidy();
		delete bi;
	}
}
//=============================================================================================================
