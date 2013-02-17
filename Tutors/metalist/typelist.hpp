//=============================================================================================================
#ifndef _TYPELIST_HPP_
#define _TYPELIST_HPP_

/**
 * \brief Metalist of type names
 */
template <typename _head, typename _tail>
struct typelist
{
	typedef _head head;
	typedef _tail tail;
};

template <typename _head>
struct typelist<_head, void>
{
	typedef _head head;
	typedef void tail;
}; 

/**
 * \brief Gets a type from the metalist
 */
template <typename ml, int index>
struct type_at
{
	typedef typename type_at<typename ml::tail, index - 1>::value value;
};

template <typename ml>
struct type_at<ml, 0>
{
	typedef typename ml::head value;
};

/**
 * \brief Gets the length of the metalist
 */
template <typename ml>
struct length
{
	enum { value = length<typename ml::tail>::value + 1 };
};

template <>
struct length<void>
{
	enum { value = 0 };
};

// typedefs
#define TL1(a)        typelist<a, void>
#define TL2(a, b)     typelist<a, typelist<b, void> >
#define TL3(a, b, c)  typelist<a, typelist<b, typelist<c, void> > >

#endif
//=============================================================================================================
