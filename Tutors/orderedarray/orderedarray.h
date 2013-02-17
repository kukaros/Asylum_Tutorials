
#ifndef _ORDEREDARRAY_H_
#define _ORDEREDARRAY_H_

#include <ostream>

/**
 * \brief Rendezett tömb int-ekhez
 *
 * A rendezett tömb elönye, hogy logaritmikus idöben lehet
 * keresni benne. A többi müveletre hasonló igaz, de még hozzájön
 * a másolásokból adódó plusz idö.
 */
class orderedarray
{
private:
	int* data;		/*!< Az adat */
	size_t mycap;	/*!< Mennyit tud tárolni */
	size_t mysize;	/*!< Hány elem van */

	//! Megmondja, hogy hova kéne berakni
	size_t _find(int value) const;

public:
	static const size_t npos = 0xffffffff;

	orderedarray();
	orderedarray(const orderedarray& other);
	~orderedarray();

	//! Berak egy elemet, ha még nincs bent
	bool insert(int value);

	//! Töröl egy elemet, ha bent van
	void erase(int value);

	//! Elöre lefoglal memoriát
	void reserve(size_t newcap);

	//! Kiüriti a tömböt és deallokálja a memóriát
	void destroy();

	//! Kiüriti a tömböt
	void clear();

	//! Ha benne van akkor az indexét adja, ha nem akkor npos-t
	size_t find(int value) const;

	//! Értékadás operátor
	orderedarray& operator =(const orderedarray& other);

	//! Csak olvasni engedjük
	inline const int& operator [](size_t index) const {
		return data[index];
	}

	//! Elemek száma
	inline size_t size() const {
		return mysize;
	}

	//! Kiírató operátor
	friend std::ostream& operator <<(std::ostream& os, orderedarray& oa);
};

#endif
