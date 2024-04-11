#pragma once
#include <cstdint>
#include <utility>
#include <cstring>

#include <boost/thread/lock_guard.hpp>

namespace akva{

struct IAllocator{
	enum policy_t{
		WRITE = 1,
		READ = 2
	};

	typedef std::pair< void*, uint64_t > pointer_t;

	virtual pointer_t Aquare(uint64_t marker) = 0;
	virtual void Leave(uint64_t marker) = 0;
	virtual void* at(uint64_t marker) = 0;

	virtual bool is_owner(uint64_t marker) const = 0;
	virtual uint16_t basket(uint64_t marker) const = 0;
	virtual uint32_t capacity() = 0;
	virtual uint64_t bytes() const = 0;
	virtual pointer_t hot() const = 0;
};

namespace mem{
#pragma pack(push, 4)

struct UniMessage{
	enum: uint64_t{
		MASK_LOG = 0xffffffffffc00000,
		MASK_POS = 0x00000000003fffc0,
		MASK_SHIFT = 0x000000000000003f
	};

	uint64_t shift : 6, pos : 16, log : 42;

	inline UniMessage(){
		shift = 0;
		pos = 0;
		log = 0;
	}

	inline uint64_t Pack() const{
		uint64_t message(0);
		memcpy(&message, this, sizeof(message));
		return message;
	}

	inline bool UnPack(uint64_t message){
		if(message > 0){
			memcpy(this, &message, sizeof(message));
			return true;
		}
		else
			return false;
	}
};

#pragma pack(pop)

} // namespace mem

struct WithOutLock{
	inline void init(){}

	inline bool try_lock(){
		return true;
	}

	inline void lock(){}
	inline void unlock(){}
};

template< typename spaceT, typename guardT = WithOutLock >
class Allocator: public IAllocator{
public:
	template< typename A >
	Allocator(uint16_t magic, A a)
		: space_(magic, a), hot_(nullptr, 0){
		guard_.init();
	}

	virtual ~Allocator();

	pointer_t Aquare(uint64_t marker);
	void Leave(uint64_t marker);
	void* at(uint64_t marker);

	bool is_owner(uint64_t marker) const;
	uint16_t basket(uint64_t marker) const;
	uint32_t capacity();
	uint64_t bytes() const;
	pointer_t hot() const;

	spaceT space_;
	pointer_t hot_;
	mutable guardT guard_;
};

// +++ Allocator +++

template< typename spaceT, typename guardT >
Allocator< spaceT, guardT >::~Allocator(){}

template< typename spaceT, typename guardT >
IAllocator::pointer_t Allocator< spaceT, guardT >::hot() const{
	boost::lock_guard< guardT > lk(guard_);
	return hot_;
}

template< typename spaceT, typename guardT >
void Allocator< spaceT, guardT >::Leave(uint64_t marker){
	boost::lock_guard< guardT > lk(guard_);
	space_.Free(marker);
}

template< typename spaceT, typename guardT >
IAllocator::pointer_t Allocator< spaceT, guardT >::Aquare(uint64_t marker){
	if((marker == 0) || is_owner(marker)){
		boost::lock_guard< guardT > lk(guard_);
		hot_ = space_.Write(marker);
		return hot_;
	}
	else{
		boost::lock_guard< guardT > lk(guard_);
		return space_.Read(marker);
	}
}

template< typename spaceT, typename guardT >
uint32_t Allocator< spaceT, guardT >::capacity(){
	boost::lock_guard< guardT > lk(guard_);
	return space_.Capacity();
}

template< typename spaceT, typename guardT >
uint64_t Allocator< spaceT, guardT >::bytes() const{
	return 0; // ToDo: ...
}

template< typename spaceT, typename guardT >
void* Allocator< spaceT, guardT >::at(uint64_t marker){
	boost::lock_guard< guardT > lk(guard_);
	return space_.at(marker);
}

template< typename spaceT, typename guardT >
bool Allocator< spaceT, guardT >::is_owner(uint64_t marker) const{
	return space_.is_owner(marker);
}

template< typename spaceT, typename guardT >
uint16_t Allocator< spaceT, guardT >::basket(uint64_t marker) const{
	mem::UniMessage u;
	u.UnPack(marker);
	return u.shift;
}

} // akva

