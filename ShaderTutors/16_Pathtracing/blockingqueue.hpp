//=============================================================================================================
#ifndef _BLOCKINGQUEUE_HPP_
#define _BLOCKINGQUEUE_HPP_

#include <queue>
#include "../11_Particles/thread.h"

template <typename value_type>
class blockingqueue
{
protected:
	std::queue<value_type> mycont;
	size_t mycap;
	size_t mysize;

	Signal notempty;
	Signal notfull;
	Guard guard;

public:
	blockingqueue();
	blockingqueue(size_t cap);
	
	// common methods
	void push(const value_type& v);
	value_type pop();

	// uncommon methods for optimized access
	void _nlpush(const value_type& v);

	inline void _beginpush() {
		notfull.Wait();
		guard.Lock();
	}

	inline void _endpush() {
		notempty.Fire();
		guard.Unlock();
	}

	inline size_t size() const {
		return mysize;
	}

	inline const Signal& _notempty() const {
		return notempty;
	}

	inline const Signal& _notfull() const {
		return notfull;
	}
};

template <typename value_type>
blockingqueue<value_type>::blockingqueue()
{
	mycap = 10;
	mysize = 0;

	notfull.Fire();
	notempty.Halt();
}

template <typename value_type>
blockingqueue<value_type>::blockingqueue(size_t cap)
{
	mycap = cap;
	mysize = 0;

	notfull.Fire();
	notempty.Halt();
}

template <typename value_type>
void blockingqueue<value_type>::push(const value_type& v)
{
	notfull.Wait();
		
	guard.Lock();
	{
		mycont.push(v);
		++mysize;

		if( mysize == mycap )
			notfull.Halt();

		notempty.Fire();
	}
	guard.Unlock();
}

template <typename value_type>
void blockingqueue<value_type>::_nlpush(const value_type& v)
{
	mycont.push(v);
	++mysize;

	if( mysize == mycap )
		notfull.Halt();
}

template <typename value_type>
value_type blockingqueue<value_type>::pop()
{
	value_type ret;

	notempty.Wait();

	guard.Lock();
	{
		ret = mycont.front();

		mycont.pop();
		--mysize;

		if( mysize == 0 )
			notempty.Halt();

		notfull.Fire();
	}
	guard.Unlock();

	return ret;
}

#endif
//=============================================================================================================
