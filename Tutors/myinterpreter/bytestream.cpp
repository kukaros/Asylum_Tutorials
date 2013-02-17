//=============================================================================================================
#include "bytestream.h"
#include <cstring>

bytestream::bytestream()
{
	mysize = 0;
	mycap = 0;
	mydata = 0;
}

bytestream::bytestream(const bytestream& other)
{
	mysize = 0;
	mycap = 0;
	mydata = 0;

	operator =(other);
}

bytestream::~bytestream()
{
	if( mydata )
	{
		delete[] mydata;
		mydata = 0;
	}
}
//=============================================================================================================
void bytestream::clear()
{
	mysize = 0;
}
//=============================================================================================================
void bytestream::reserve(size_t newcap)
{
	if( mycap >= newcap )
		return;

	char* newdata = new char[newcap];

	if( mydata && mysize > 0 )
		memcpy(newdata, mydata, mysize);

	if( mydata )
		delete[] mydata;

	mydata = newdata;
	mycap = newcap;
}
//=============================================================================================================
void bytestream::replace(void* what, void* with, size_t size)
{
	size_t start = 0;

	// bruteforce...
	while( (start + size) <= mysize )
	{
		if( memcmp(mydata + start, what, size) == 0 )
		{
			memcpy(mydata + start, with, size);
			start += size;
		}
		else
			++start;
	}
}
//=============================================================================================================
bytestream& bytestream::operator <<(unsigned char value)
{
	if( (mysize + sizeof(unsigned char)) > mycap )
		reserve(mysize + 1024);

	*((unsigned char*)(mydata + mysize)) = value;
	mysize += sizeof(unsigned char);

	return *this;
}
//=============================================================================================================
bytestream& bytestream::operator <<(int value)
{
	if( (mysize + sizeof(int)) > mycap )
		reserve(mysize + 1024);

	*((int*)(mydata + mysize)) = value;
	mysize += sizeof(int);

	return *this;
}
//=============================================================================================================
bytestream& bytestream::operator <<(const bytestream& other)
{
	if( other.mydata && other.mysize > 0 )
	{
		if( (mysize + other.mysize) > mycap )
			reserve(mysize + other.mysize + 512);

		memcpy(mydata + mysize, other.mydata, other.mysize);
		mysize += other.mysize;
	}

	return *this;
}
//=============================================================================================================
bytestream& bytestream::operator =(const bytestream& other)
{
	if( &other != this )
	{
		if( mycap < other.mycap )
			reserve(other.mycap);

		memcpy(mydata, other.mydata, other.mysize);
		mysize = other.mysize;
	}

	return *this;
}
//=============================================================================================================
