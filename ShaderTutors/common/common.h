
#ifndef _COMMON_H_
#define _COMMON_H_

template <typename T>
struct state
{
	T prev;
	T curr;

	state& operator =(const T& t) {
		prev = curr = t;
		return *this;
	}

	T smooth(float alpha) {
		return prev + alpha * (curr - prev);
	}
};

#endif
