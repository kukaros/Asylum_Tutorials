//=============================================================================================================
#ifndef _ORDEREDMULTIARRAY_HPP_
#define _ORDEREDMULTIARRAY_HPP_

#include <functional>

template <typename value_type, typename compare = default_less<value_type> >
class orderedmultiarray
{
private:
	value_type* data;
	size_t mycap;
	size_t mysize;

	size_t _find(const value_type& value) const;

public:
	typedef std::pair<size_t, size_t> pairii;
	static const size_t npos = 0xffffffff;

	compare comp;

	orderedmultiarray();
	orderedmultiarray(const orderedmultiarray& other);
	~orderedmultiarray();

	size_t insert(const value_type& value);

	void erase(const value_type& value);
	void reserve(size_t newcap);
	void destroy();
	void clear();
	void pop_back();

	pairii equal_range(const value_type& value) const;

	size_t find(const value_type& value) const;
	size_t lower_bound(const value_type& value) const;
	size_t upper_bound(const value_type& value) const;

	orderedmultiarray& operator =(const orderedmultiarray& other);
	
	inline const value_type& operator [](size_t index) const {
		return data[index];
	}
	
	inline size_t size() const {
		return mysize;
	}
};

template <typename value_type, typename compare>
orderedmultiarray<value_type, compare>::orderedmultiarray()
{
	data = 0;
	mysize = 0;
	mycap = 0;
}
	
template <typename value_type, typename compare>
orderedmultiarray<value_type, compare>::orderedmultiarray(const orderedmultiarray& other)
{
	data = 0;
	mysize = 0;
	mycap = 0;

	this->operator =(other);
}
	
template <typename value_type, typename compare>
orderedmultiarray<value_type, compare>::~orderedmultiarray()
{
	destroy();
}
	
template <typename value_type, typename compare>
size_t orderedmultiarray<value_type, compare>::_find(const value_type& value) const
{
	size_t low = 0;
	size_t high = mysize;
	size_t mid = (low + high) / 2;

	while( low < high )
	{
		if( comp(data[mid], value) )
			low = mid + 1;
		else if( comp(value, data[mid]) )
			high = mid;
		else
			return mid;

		mid = (low + high) / 2;
	}
	
	return low;
}
	
template <typename value_type, typename compare>
size_t orderedmultiarray<value_type, compare>::insert(const value_type& value)
{
	size_t i = 0;

	reserve(mysize + 1);

	if( mysize > 0 )
	{
		i = _find(value);

		size_t count = (mysize - i);

		new(data + mysize) value_type();

		for( size_t j = count; j > 0; --j )
			data[i + j] = data[i + j - 1];
	}
	else
		new(data) value_type();

	data[i] = value;
	++mysize;

	return i;
}
	
template <typename value_type, typename compare>
void orderedmultiarray<value_type, compare>::erase(const value_type& value)
{
	pairii p = equal_range(value);

	if( p.first != npos )
	{
		size_t count = (mysize - p.second);

		for( size_t j = 0; j < count; ++j )
			data[p.first + j] = data[p.second + j];

		for( size_t i = p.first + count; i < mysize; ++i )
			(data + i)->~value_type();

		mysize -= (p.second - p.first);

		if( mysize == 0 )
			clear();
	}
}
	
template <typename value_type, typename compare>
void orderedmultiarray<value_type, compare>::reserve(size_t newcap)
{
	if( mycap < newcap )
	{
		size_t diff = newcap - mycap;
		diff = std::max<size_t>(diff, 10);

		value_type* newdata = (value_type*)malloc((mycap + diff) * sizeof(value_type));

		for( size_t i = 0; i < mysize; ++i )
			new(newdata + i) value_type(data[i]);

		for( size_t i = 0; i < mysize; ++i )
			(data + i)->~value_type();

		if( data )
			free(data);

		data = newdata;
		mycap = mycap + diff;
	}
}
	
template <typename value_type, typename compare>
void orderedmultiarray<value_type, compare>::destroy()
{
	if( data )
	{
		for( size_t i = 0; i < mysize; ++i )
			(data + i)->~value_type();

		free(data);
	}

	data = 0;
	mysize = 0;
	mycap = 0;
}
	
template <typename value_type, typename compare>
void orderedmultiarray<value_type, compare>::clear()
{
	for( size_t i = 0; i < mysize; ++i )
		(data + i)->~value_type();

	mysize = 0;
}
	
template <typename value_type, typename compare>
void orderedmultiarray<value_type, compare>::pop_back()
{
	if( mysize > 0 )
	{
		--mysize;
		(data + mysize)->~value_type();
	}
}

template <typename value_type, typename compare>
typename orderedmultiarray<value_type, compare>::pairii
orderedmultiarray<value_type, compare>::equal_range(const value_type& value) const
{
	pairii range;

	range.first = npos;
	range.second = npos;

	if( mysize > 0 )
	{
		range.first = _find(value);

		if( range.first < mysize )
		{
			if( comp(data[range.first], value) || comp(value, data[range.first]) )
			{
				// doesn't exist
				range.first = npos;
			}
			else
			{
				range.second = range.first + 1;

				// look for first and last index
				while( range.first > 0 )
				{
					if( comp(data[range.first - 1], value) )
						break;

					--range.first;
				}

				while( range.second < mysize )
				{
					if( comp(value, data[range.second]) )
						break;

					++range.second;
				}
			}
		}
		else
			range.first = npos;
	}

	return range;
}
	
template <typename value_type, typename compare>
size_t orderedmultiarray<value_type, compare>::find(const value_type& value) const
{
	return lower_bound(value);
}
	
template <typename value_type, typename compare>
size_t orderedmultiarray<value_type, compare>::lower_bound(const value_type& value) const
{
	// returns the first that is greater or equal
	if( mysize == 0 )
		return 0;

	size_t ind = _find(value);

	if( ind < mysize )
	{
		if( comp(value, data[ind]) )
		{
			// first greater
			return ind;
		}
		else if( !comp(data[ind], value) )
		{
			// equal, find first equal
			do
			{
				if( ind == 0 || comp(data[ind - 1], value) )
					break;

				--ind;
			}
			while( true );
				
			return ind;
		}
	}

	return npos;
}
	
template <typename value_type, typename compare>
size_t orderedmultiarray<value_type, compare>::upper_bound(const value_type& value) const
{
	// returns the first that is greater
	if( mysize == 0 )
		return 0;

	size_t ind = _find(value);

	if( ind < mysize )
	{
		if( comp(value, data[ind]) )
		{
			// first greater
			return ind;
		}
		else if( !comp(data[ind], value) )
		{
			// equal, find first greater
			++ind;

			while( ind < mysize )
			{
				if( comp(value, data[ind]) )
					break;

				++ind;
			}

			if( ind != mysize )
				return ind;
		}
	}

	return npos;
}
	
template <typename value_type, typename compare>
orderedmultiarray<value_type, compare>& orderedmultiarray<value_type, compare>::operator =(const orderedmultiarray& other)
{
	if( &other == this )
		return *this;

	clear();

	reserve(other.mycap);
	mysize = other.mysize;

	for( size_t i = 0; i < mysize; ++i )
		new(data + i) value_type(other.data[i]);

	return *this;
}
	
template <typename value_type, typename compare>
std::ostream& operator <<(std::ostream& os, orderedmultiarray<value_type, compare>& oa)
{
	for( size_t i = 0; i < oa.size(); ++i )
		os << oa.data[i] << " ";

	return os;
}

#endif
//=============================================================================================================
