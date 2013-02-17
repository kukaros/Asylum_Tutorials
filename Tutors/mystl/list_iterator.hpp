//=============================================================================================================
#ifndef _LIST_ITERATOR_HPP_
#define _LIST_ITERATOR_HPP_

namespace mystl
{
    template <typename value_type>
    class list<value_type>::iterator
    {
        friend class list;

    private:
        list* container;
        link* ptr;

        iterator(list* l, link* n)
            : container(l), ptr(n) {}

    public:
        typedef value_type& reference;
        typedef value_type* pointer;

        iterator()
            : container(NULL), ptr(NULL) {}

        iterator(const iterator& other)
            : container(other.container), ptr(other.ptr) {}
        
        inline iterator& operator ++() {
            mynerror(*this, "list::iterator::operator ++(): iterator not incrementable", ptr == container->head);

            ptr = ptr->next;
            return *this;
        }

        inline iterator operator ++(int) {
            mynerror(*this, "list::iterator::operator ++(int): iterator not incrementable", ptr == container->head);
            iterator tmp = *this;

            ptr = ptr->next;
            return tmp;
        }

        inline iterator& operator --() {
            mynerror(*this, "list::iterator::operator --(): iterator not decrementable", ptr->prev == container->head);

            ptr = ptr->prev;
            return *this;
        }

        inline iterator operator --(int) {
            mynerror(*this, "list::iterator::operator --(int): iterator not decrementable", ptr->prev == container->head);
            iterator tmp = *this;

            ptr = ptr->prev;
            return tmp;
        }

        inline iterator& operator =(const iterator& other) {
            container = other.container;
            ptr = other.ptr;
            return *this;
        }
        
        inline reference operator *() const {
            mynerror(container->head->value, "list::iterator::operator *(): iterator not dereferencable", ptr == container->head || !ptr);
            return ptr->value;
        }

        inline pointer operator ->() const {
            mynerror(NULL, "list::iterator::operator ->(): iterator not dereferencable", ptr == container->head || !ptr);
            return &ptr->value;
        }

        inline bool operator !=(const iterator& other) const {
            myerror(true, "list::iterator::operator !=(): iterators incompatible", container == other.container);
            return (ptr != other.ptr);
        }

        inline bool operator ==(const iterator& other) const {
            myerror(false, "list::iterator::operator ==(): iterators incompatible", container == other.container);
            return (ptr == other.ptr);
        }
    };

    template <typename value_type>
    class list<value_type>::const_iterator
    {
        friend class list;

    private:
        const list* container;
        link* ptr;

        const_iterator(const list* l, link* n)
            : container(l), ptr(n) {}

    public:
        typedef const value_type& reference;
        typedef const value_type* pointer;

        const_iterator()
            : container(NULL), ptr(NULL) {}

        const_iterator(const const_iterator& other)
            : container(other.container), ptr(other.ptr) {}

        const_iterator(const iterator& other)
            : container(other.container), ptr(other.ptr) {}

        inline const_iterator& operator ++() {
            mynerror(*this, "list::const_iterator::operator ++(): iterator not incrementable", ptr == container->head);

            ptr = ptr->next;
            return *this;
        }

        inline const_iterator operator ++(int) {
            mynerror(*this, "list::const_iterator::operator ++(int): iterator not incrementable", ptr == container->head);
            const_iterator tmp = *this;

            ptr = ptr->next;
            return tmp;
        }

        inline const_iterator& operator --() {
            mynerror(*this, "list::const_iterator::operator --(): iterator not decrementable", ptr->prev == container->head);

            ptr = ptr->prev;
            return *this;
        }

        inline const_iterator operator --(int) {
            mynerror(*this, "list::const_iterator::operator --(int): iterator not decrementable", ptr->prev == container->head);
            const_iterator tmp = *this;

            ptr = ptr->prev;
            return tmp;
        }

        inline const_iterator& operator =(const const_iterator& other) {
            container = other.container;
            ptr = other.ptr;
            return *this;
        }

        inline const_iterator& operator =(const iterator& other) {
            container = other.container;
            ptr = other.ptr;
            return *this;
        }

        inline reference operator *() const {
            mynerror(container->head->value, "list::const_iterator::operator *(): iterator not dereferencable", ptr == container->head || !ptr);
            return ptr->value;
        }

        inline pointer operator->() const {
            myerror(NULL, "list::const_iterator::operator ->(): iterator not dereferencable", ptr == container->head || !ptr);
            return &ptr->value;
        }

        inline bool operator !=(const const_iterator& other) const {
            myerror(true, "list::const_iterator::operator !=(): iterators incompatible", container == other.container);
            return (ptr != other.ptr);
        }

        inline bool operator !=(const iterator& other) const {
            myerror(true, "list::const_iterator::operator !=(): iterators incompatible", container == other.container);
            return (ptr != other.ptr);
        }

        inline bool operator ==(const const_iterator& other) const {
            myerror(false, "list::const_iterator::operator ==(): iterators incompatible", container == other.container);
            return (ptr == other.ptr);
        }

        inline bool operator ==(const iterator& other) const {
            myerror(false, "list::const_iterator::operator ==(): iterators incompatible", container == other.container);
            return (ptr == other.ptr);
        }
    };
}

#endif
//=============================================================================================================
