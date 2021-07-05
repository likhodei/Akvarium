#pragma once
#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

#include <boost/static_assert.hpp>

namespace kernel_ns{

template< bool condition, class TypeIfTrue, class TypeIfFale >
struct StaticIf{
	typedef TypeIfTrue Result;
};

template< class TypeIfTrue, class TypeIfFale >
struct StaticIf< false, TypeIfTrue, TypeIfFale >{
	 typedef TypeIfFale Result;
};

template< unsigned size >
struct SelectSizeForLength{
	static const bool LessOrEq8 = size <= 0xff;
	static const bool LessOrEq16 = size <= 0xffff;

	typedef typename StaticIf<
			LessOrEq8,
			uint8_t,
			typename StaticIf< LessOrEq16, uint16_t, uint32_t>::Result >
			::Result Result;
};

template< int sizeT, class dataT = unsigned char >
class RingBufferV0{
public:
	typedef typename SelectSizeForLength< sizeT >::Result index_t;

public:
	inline bool Write(dataT value){
		if(IsFull()) return false;

		data_[writeCount_++ & mask_] = value;
		return true;
	}

	inline bool Read(dataT &value){
		if(IsEmpty()) return false;

		value = data_[readCount_++ & mask_];
		return true;
	}

	inline dataT First()const{
		return operator[](0);
	}

	inline dataT Last()const{
		return operator[](Count() - 1);
	}

	inline dataT& operator[] (index_t i){
		if(IsEmpty() || i > Count()) return none_;

		return data_[(readCount_ + i) & mask_];
	}

	inline const dataT operator[] (index_t i)const{
		if(IsEmpty() || i > Count()) return dataT();

		return data_[(readCount_ + i) & mask_];
	}

	inline bool IsEmpty() const{
		return writeCount_ == readCount_;
	}
		
	inline bool IsFull() const{
		return ((writeCount_ - readCount_) & (index_t)~(mask_)) != 0;
	}

	index_t Count() const{
		return (writeCount_ - readCount_) & mask_;
	}

	inline void Clear(){
		readCount_ = 0;
    	writeCount_ = 0;
	}

	inline unsigned Size()
	{ return sizeT; }

private:
	BOOST_STATIC_ASSERT((sizeT & (sizeT - 1)) == 0); // sizeT must be a power of 2

	dataT data_[sizeT];
	dataT none_ = dataT();

    volatile index_t readCount_;
    volatile index_t writeCount_;
	static const index_t mask_ = sizeT - 1;
};

} // kernel_ns

#endif // RING_BUFFER_H_