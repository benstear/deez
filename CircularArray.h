#ifndef CircularArray_H
#define CircularArray_H

#include <cstring>
#include <inttypes.h>
#include <stdlib.h>
#include "Common.h"

template<class T>
class CircularArray {
	T* _records;

	size_t _size;
	size_t _start;
	size_t _extend;
	size_t _capacity;

public:
	CircularArray (size_t cap):
		_size(0), _start(0), _capacity(cap)
	{
		_records = new T[_capacity];
		_extend = _capacity; // by default, double ...
	}

	~CircularArray (void) 
	{
		if (_records) {
			delete[] _records;
			_records = 0;
		}
	}

	CircularArray (const CircularArray& a)
	{
		_size = a._size;
		_capacity = a._capacity;
		_extend = a._extend;
		_start = a._start;
		_records = new T[a._capacity];
		std::copy(a._records, a._records + a._size, _records);
	}

	CircularArray(CircularArray&& a): CircularArray()
	{
		swap(*this, a);
	}

	CircularArray& operator= (CircularArray a) 
	{
		swap(*this, a);
		return *this;
	}

	friend void swap(CircularArray& a, CircularArray& b) // nothrow
	{
		using std::swap;

		swap(a._start, b._start);
		swap(a._records, b._records);
		swap(a._size, b._size);
		swap(a._capacity, b._capacity);
		swap(a._extend, b._extend);
	}

public:
	void realloc (size_t sz) 
	{
		size_t newcap = sz + _extend;
		T *tmp = new T[newcap];

		size_t p1 = std::min(_capacity - _start, _size);
		size_t p2 = _size - p1;
		std::copy(_records + _start, _records + _start + p1, tmp);
		std::copy(_records, _records + p2, tmp + p1);

		_start = 0;
		_capacity = newcap;
		delete[] _records;
		_records = tmp;
	}

	void resize (size_t sz) 
	{
		while (sz > _capacity)
			realloc(sz);
		_size = sz;
		//fprintf(stderr,"Resized %d\n",_size);
	}

	// add element, realloc if needed
	void add (const T &t) 
	{
		if (_size == _capacity)
			realloc(_capacity);
		_records[(_start + _size) % _capacity] = t;
		_size++;
		//fprintf(stderr,"Resized %d\n",_size);

	}

	// add array, realloc if needed
	void add (const T *t, size_t sz) 
	{
		// can be faster!
		for (size_t i = 0; i < sz; i++)
			add(t[i]);
	}

	T &operator[] (size_t i) 
	{
		assert(i < _size);
		return _records[(_start + i) % _capacity];
	}

	size_t size (void) const 
	{
		return _size;
	}

	void remove_first_n (size_t k) 
	{
		assert (k <= _size);
		_size -= k;
		_start = (_start + k) % _capacity;
		//fprintf(stderr,"Resized %d\n",_size);
	}

	T *head (void) 
	{
		return _records + _start;
	}

	T *increase (T *x) 
	{
		if (x + 1 == _records + _capacity)
			return _records;
		else return x + 1;
	}

	/// 	 for memory checking
	size_t capacity (void) const {
		return _capacity;
	}

	T *data (void) {
		return _records;
	}

};

#endif