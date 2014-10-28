#ifdef MSVC
#pragma once
#endif

#ifndef _FIFO_Q_INCLUDE
#define _FIFO_Q_INCLUDE

#include <iostream>
#include <iterator>     // std::istream_iterator

using namespace std;

template <class T>
class FifoQueue {
private:
	T * m_buf;
	unsigned long m_head;
	unsigned long m_tail;
	size_t m_size;

public:
	FifoQueue(size_t size)
	{
		m_buf = new T[size];
		m_head = 0;
		m_tail = 0;
		m_size = size;
	}

	~FifoQueue() { delete [] m_buf; }
	size_t size() const { return (m_head - m_tail + m_size) % m_size; }
	size_t capacity() const { return m_size - 1; }
	bool full() const { return ((m_head + 1) % m_size) == m_tail; }
	bool empty() const { return m_head == m_tail; }
	T put(T const&);
	T get();
	unsigned long read(T *, size_t);
	unsigned long write(T *, size_t);
	void clear() { m_head = m_tail = 0; }

};

template <class T>
unsigned long FifoQueue<T>::read(T *buf, size_t count) {
	unsigned long i;
     T * p = buf;

	 for(i=0; i < count; i++){
          if( m_tail != m_head ){ //see if any data is available
               *p++ = m_buf[m_tail];  //grab a byte from the buffer
               m_tail++;  //increment the tail
               if( m_tail == m_size ){  //check for wrap-around
                    m_tail = 0;
               }
          } else {
               return i; //number of bytes read
          }
     }

     return count;
}

template <class T>
unsigned long FifoQueue<T>::write(T *buf, size_t count) {
     unsigned long i;
     T * p = buf;

     for(i=0; i < count; i++){
           //first check to see if there is space in the buffer
           if( (m_head + 1 == m_tail) ||
                ( (m_head + 1 == m_size) && (m_tail == 0) ) ){
                 return i; //no more room
           } else {
               m_buf[m_head] = *p++;
               m_head++;  //increment the head
               if( m_head == m_size ){  //check for wrap-around
                    m_head = 0;
               }
           }
     }

     return count;
}

template <class T>
T FifoQueue<T>::put(T const&n) {
	m_buf[m_head] = n;
    m_head = (m_head + 1) % m_size;
	return n;
}

template <class T>
T FifoQueue<T>::get() {
	T n = m_buf[m_tail];
	m_tail = (m_tail + 1) % m_size;
	return n;
}

template <class T>
ostream& operator<< (ostream& os, FifoQueue<T>& q) {

	while(!q.empty()) {
		os << q.get();
	}

	return os;
}

//Note how this is declared outside of the class body, so it is a free function instead of a memberfunction
template<class T> inline std::ostream& operator<<(std::ostream& out, const FifoQueue<T>& val){

	while(!val.empty()) {
		out << val.get();
	}

	return out;
}
#endif
