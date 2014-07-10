
#ifndef _ORDEREDARRAY_HPP_
#define _ORDEREDARRAY_HPP_

#include "functional.hpp"

namespace mystl
{
	template <typename value_type, typename compare = default_less<value_type> >
	class orderedarray
	{
	private:
		value_type* data;
		size_t mycap;
		size_t mysize;

		size_t _find(const value_type& value) const;

	public:
		static const size_t npos = 0xffffffff;

		compare comp;

		orderedarray();
		orderedarray(const orderedarray& other);
		~orderedarray();

		bool insert(const value_type& value);

		void erase(const value_type& value);
		void erase_at(size_t index);
		void reserve(size_t newcap);
		void destroy();
		void clear();
		void swap(orderedarray& other);

		size_t find(const value_type& value) const;
		size_t lower_bound(const value_type& value) const;
		size_t upper_bound(const value_type& value) const;

		orderedarray& operator =(const orderedarray& other);
	
		inline const value_type& operator [](size_t index) const {
			return data[index];
		}

		inline size_t size() const {
			return mysize;
		}

		inline size_t capacity() const {
			return mycap;
		}

		// uncommon methods
		void _fastcopy(const orderedarray& other);

		template <typename T, typename U>
		friend std::ostream& operator <<(std::ostream& os, const orderedarray<T, U>& oa);
	};

	template <typename value_type, typename compare>
	orderedarray<value_type, compare>::orderedarray()
	{
		data = 0;
		mysize = 0;
		mycap = 0;
	}
	
	template <typename value_type, typename compare>
	orderedarray<value_type, compare>::orderedarray(const orderedarray& other)
	{
		data = 0;
		mysize = 0;
		mycap = 0;

		this->operator =(other);
	}
	
	template <typename value_type, typename compare>
	orderedarray<value_type, compare>::~orderedarray()
	{
		destroy();
	}
	
	template <typename value_type, typename compare>
	size_t orderedarray<value_type, compare>::_find(const value_type& value) const
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
	bool orderedarray<value_type, compare>::insert(const value_type& value)
	{
		size_t i = 0;

		reserve(mysize + 1);

		if( mysize > 0 )
		{
			i = _find(value);

			if( i < mysize && !(comp(data[i], value) || comp(value, data[i])) )
				return false;

			size_t count = (mysize - i);
			new(data + mysize) value_type;

			for( size_t j = count; j > 0; --j )
				data[i + j] = data[i + j - 1];
		}
		else
			new(data) value_type;

		data[i] = value;
		++mysize;

		return true;
	}
	
	template <typename value_type, typename compare>
	void orderedarray<value_type, compare>::erase(const value_type& value)
	{
		size_t i = find(value);

		if( i != npos )
		{
			size_t count = (mysize - i) - 1;

			for( size_t j = 0; j < count; ++j )
				data[i + j] = data[i + j + 1];

			(data + i + count)->~value_type();
			--mysize;

			if( mysize == 0 )
				clear();
		}
	}
	
	template <typename value_type, typename compare>
	void orderedarray<value_type, compare>::erase_at(size_t index)
	{
		if( index < mysize )
		{
			size_t count = (mysize - index) - 1;

			for( size_t j = 0; j < count; ++j )
				data[index + j] = data[index + j + 1];

			(data + index + count)->~value_type();
			--mysize;

			if( mysize == 0 )
				clear();
		}
	}

	template <typename value_type, typename compare>
	void orderedarray<value_type, compare>::reserve(size_t newcap)
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
	void orderedarray<value_type, compare>::destroy()
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
	void orderedarray<value_type, compare>::clear()
	{
		for( size_t i = 0; i < mysize; ++i )
			(data + i)->~value_type();

		mysize = 0;
	}
	
	template <typename value_type, typename compare>
	size_t orderedarray<value_type, compare>::find(const value_type& value) const
	{
		if( mysize > 0 )
		{
			size_t ind = _find(value);

			if( ind < mysize )
			{
				if( !(comp(data[ind], value) || comp(value, data[ind])) )
					return ind;
			}
		}

		return npos;
	}

	template <typename value_type, typename compare>
	size_t orderedarray<value_type, compare>::lower_bound(const value_type& value) const
	{
		// returns the first that is not greater
		if( mysize > 0 )
		{
			size_t ind = _find(value);

			if( !(comp(data[ind], value) || comp(value, data[ind])) )
				return ind;
			else if( ind > 0 )
				return ind - 1;
		}

		return npos;
	}

	template <typename value_type, typename compare>
	size_t orderedarray<value_type, compare>::upper_bound(const value_type& value) const
	{
		// returns the first that is not smaller
		return _find(value);
	}
	
	template <typename value_type, typename compare>
	orderedarray<value_type, compare>& orderedarray<value_type, compare>::operator =(const orderedarray& other)
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
	void orderedarray<value_type, compare>::_fastcopy(const orderedarray& other)
	{
		if( &other != this )
		{
			reserve(other.mycap);
			mysize = other.mysize;

			memcpy(data, other.data, mysize * sizeof(value_type));
		}
	}

	template <typename value_type, typename compare>
	void orderedarray<value_type, compare>::swap(orderedarray& other)
	{
		if( &other == this )
			return;

		std::swap(mycap, other.mycap);
		std::swap(mysize, other.mysize);
		std::swap(data, other.data);
	}

	template <typename value_type, typename compare>
	std::ostream& operator <<(std::ostream& os, const orderedarray<value_type, compare>& oa)
	{
		for( size_t i = 0; i < oa.size(); ++i )
			os << oa[i] << " ";

		return os;
	}
}

#endif
