
#include "orderedarray.h"
#include <cstring>

orderedarray::orderedarray()
{
	data = 0;
	mysize = 0;
	mycap = 0;
}

orderedarray::orderedarray(const orderedarray& other)
{
	// konstruktorok nem hívhatják egymást
	data = 0;
	mysize = 0;
	mycap = 0;

	this->operator =(other);
}

orderedarray::~orderedarray()
{
	destroy();
}

size_t orderedarray::_find(int value) const
{
	size_t low = 0;
	size_t high = mysize;
	size_t mid = (low + high) / 2;

	// logaritmikus keresés
	while( low < high )
	{
		if( data[mid] < value )
			low = mid + 1;
		else if( data[mid] > value )
			high = mid;
		else
			return mid;

		mid = (low + high) / 2;
	}

	return low;
}

bool orderedarray::insert(int value)
{
	size_t i = 0;

	// lefoglalunk helyet
	reserve(mysize + 1);

	if( mysize > 0 )
	{
		// megkeressük a helyét
		i = _find(value);

		// ha már benne van
		if( i < mysize && data[i] == value )
			return false;

		// arrébb toljuk az elemeket
		size_t count = (mysize - i);

		if( count > 0 )
			memmove(data + i + 1, data + i, count * sizeof(int));
	}

	// berakjuk a helyére
	data[i] = value;
	++mysize;

	return true;
}

void orderedarray::erase(int value)
{
	size_t i = find(value);

	if( i != npos )
	{
		size_t count = (mysize - i);

		if( count > 0 )
			memmove(data + i, data + i + 1, count * sizeof(int));

		--mysize;

		if( mysize == 0 )
			clear();
	}
}

void orderedarray::reserve(size_t newcap)
{
	// csak akkor foglaljuk ujra, ha szükséges
	if( mycap < newcap )
	{
		// legalább 10-et lefoglalunk, így hatékonyabb lesz
		size_t diff = newcap - mycap;
		diff = std::max<size_t>(diff, 10);

		// ha a memcpy()-ban hiba történne akkor a régi adat még meglegyen
		int* tmp = new int[mycap + diff];

		// if( data ) itt nem jó!!
		if( mysize > 0 )
			memcpy(tmp, data, mysize * sizeof(int));

		if( data )
			delete data;

		data = tmp;
		mycap = mycap + diff;
	}

	// egyébként nem kell csinálni semmit
}

void orderedarray::destroy()
{
	if( data )
		delete[] data;

	data = NULL;
	mysize = 0;
	mycap = 0;
}

void orderedarray::clear()
{
	mysize = 0;
}

size_t orderedarray::find(int value) const
{
	if( mysize > 0 )
	{
		size_t ind = _find(value);

		if( ind < mysize )
			return (data[ind] == value ? ind : npos);
	}

	return npos;
}

orderedarray& orderedarray::operator =(const orderedarray& other)
{
	if( &other == this )
		return *this;

	reserve(other.mycap);
	mysize = other.mysize;

	memcpy(data, other.data, mysize * sizeof(int));
	return *this;
}

std::ostream& operator <<(std::ostream& os, orderedarray& oa)
{
	for( size_t i = 0; i < oa.size(); ++i )
		os << oa.data[i] << " ";

	return os;
}
