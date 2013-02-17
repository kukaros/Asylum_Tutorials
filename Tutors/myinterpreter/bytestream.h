//=============================================================================================================
#ifndef _BYTESTREAM_H_
#define _BYTESTREAM_H_

class bytestream
{
private:
	char* mydata;
	size_t mysize;
	size_t mycap;

public:
	bytestream();
	bytestream(const bytestream& other);
	~bytestream();

	void clear();
	void reserve(size_t newcap);
	void replace(void* what, void* with, size_t size);

	bytestream& operator <<(unsigned char value);
	bytestream& operator <<(int value);
	bytestream& operator <<(const bytestream& other);

	bytestream& operator =(const bytestream& other);

	inline size_t size() const {
		return mysize;
	}

	inline char* data() {
		return mydata;
	}

	inline char* seek_set(size_t off) {
		return (mydata + off);
	}

	inline char* seek_end(size_t off) {
		return (mydata + (mysize - off));
	}
};

#endif
//=============================================================================================================

