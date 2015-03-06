
#ifndef _BLOCKINGQUEUE_HPP_
#define _BLOCKINGQUEUE_HPP_

#include "thread.h"

#define ASSERT(x) { if( !(x) ) throw 1; }

template <typename value_type>
class blockingqueue
{
	struct node
	{
		value_type value;
		node* next;
	};

private:
	Guard	guard;
	Signal	notempty;
	node	head;
	node*	last;
	size_t	mysize;

public:
	blockingqueue();
	~blockingqueue();

	void push(const value_type& value);
	value_type pop();

	bool empty() const;
	size_t size() const;
};

template <typename value_type>
blockingqueue<value_type>::blockingqueue()
{
	head.next	= &head;
	last		= &head;
	mysize		= 0;

	notempty.Halt();
}

template <typename value_type>
blockingqueue<value_type>::~blockingqueue()
{
	node* q = head.next;
	node* p;

	while( q != &head )
	{
		p = q->next;
		delete q;
		q = p;
	}

	head.next = &head;

	last = 0;
	mysize = 0;
}

template <typename value_type>
void blockingqueue<value_type>::push(const value_type& value)
{
	guard.Lock();
	{
		last->next = new node();
		last->next->value = value;

		last = last->next;
		last->next = &head;

		++mysize;
		notempty.Fire();
	}
	guard.Unlock();
}

template <typename value_type>
value_type blockingqueue<value_type>::pop()
{
	value_type val;
	bool popped = false;

	do
	{
		notempty.Wait();

		guard.Lock();
		{
			if( mysize > 0 )
			{
				node* q = head.next;
				head.next = q->next;
				val = q->value;

				delete q;
				--mysize;

				if( mysize == 0 )
					last = &head;

				popped = true;
			}

			if( mysize == 0 )
				notempty.Halt();
		}
		guard.Unlock();
	}
	while( !popped );

	return val;
}

template <typename value_type>
bool blockingqueue<value_type>::empty() const
{
	guard.Lock();
	size_t check = mysize;
	guard.Unlock();

	return (check == 0);
}

template <typename value_type>
size_t blockingqueue<value_type>::size() const
{
	guard.Lock();
	size_t check = mysize;
	guard.Unlock();

	return check;
}

#endif
