#pragma once
#ifndef SUPPORT_SIMPLE_BLOCK_H_
#define SUPPORT_SIMPLE_BLOCK_H_

#include "support/bit_operation.hpp"

#include <boost/array.hpp>
#include <boost/assert.hpp>

#include <vector>
#include <cstdint>
#include <cstring>

namespace akva{

struct IBlock{
	virtual ~IBlock(){}

	virtual uint8_t *const base() const = 0;
	virtual uint8_t *const end() const = 0;

	virtual void set_rd_ptr(uint8_t *const ptr) = 0;
	virtual void set_wr_ptr(uint8_t *const ptr) = 0;

	virtual uint8_t *const rd_ptr() const = 0;
	virtual uint8_t *const wr_ptr() const = 0;

	virtual void set_offset_rd_ptr(size_t n) = 0;
	virtual void set_offset_wr_ptr(size_t n) = 0;

	virtual size_t space() const = 0;
	virtual size_t length() const = 0;
	virtual size_t size() const = 0;

	virtual void ReSet() = 0;
	virtual void set_time(time_t moment) = 0;
	virtual time_t time() const = 0;

	virtual ptrdiff_t set_flags(ptrdiff_t more_flags) = 0;
	virtual ptrdiff_t clr_flags(ptrdiff_t less_flags) = 0;

	virtual ptrdiff_t flags() const = 0;

	virtual ptrdiff_t set_self_flags(ptrdiff_t more_flags) = 0;
	virtual ptrdiff_t clr_self_flags(ptrdiff_t less_flags) = 0;
	virtual ptrdiff_t self_flags() const = 0;

	virtual bool Copy(const uint8_t* buffer, size_t n) = 0;
	virtual bool MemMove() = 0;
	virtual bool ReSize(size_t new_size) = 0;
};

namespace support{

class BaseBlock: public IBlock{
public:
	ptrdiff_t rd_offset_; // Pointer to beginning of next read.
	ptrdiff_t wr_offset_; // Pointer to beginning of next write.

	ptrdiff_t self_flags_; // burst block
	ptrdiff_t flags_;      // data block 
	time_t moment_;

	virtual ~BaseBlock(){
		ReSet();
	}

	virtual bool MemMove(){
		if(rd_offset_ != 0){
			size_t sz = length();
			memmove(base(), rd_ptr(), sz);
			rd_offset_ = 0;
			wr_offset_ = sz;
			return true;
		}
		else
			return false;
	}

	virtual bool ReSize(size_t new_size){
		return false;
	}

	virtual void set_rd_ptr(uint8_t *const ptr){
		rd_offset_ = ptr - base();
	}

	virtual void set_wr_ptr(uint8_t *const ptr){
		wr_offset_ = ptr - base();
	}

	virtual uint8_t *const rd_ptr() const{
		return base() + rd_offset_;
	}

	virtual uint8_t *const wr_ptr() const{
		return base() + wr_offset_;
	}

	virtual void set_offset_rd_ptr(size_t n){
		rd_offset_ += n;
	}

	virtual void set_offset_wr_ptr(size_t n){
		wr_offset_ += n;
	}

	virtual size_t space() const{
		return end() - wr_ptr();
	}

	virtual size_t length() const{
		return wr_offset_ - rd_offset_;
	}

	virtual void ReSet(){
		rd_offset_ = wr_offset_ = 0;
		self_flags_ = flags_ = 0;
		moment_ = 0;
	}

	virtual void set_time(time_t moment){
		moment_ = moment;
	}

	virtual time_t time() const{
		return moment_;
	}

	virtual ptrdiff_t set_flags(ptrdiff_t more_flags){
		SetBits(flags_, more_flags);
		return flags_;
	}

	virtual ptrdiff_t clr_flags(ptrdiff_t less_flags){
		ClrBits(flags_, less_flags);
		return flags_;
	}

	virtual ptrdiff_t flags() const{
		return flags_;
	}

	virtual ptrdiff_t set_self_flags(ptrdiff_t more_flags){
		SetBits(self_flags_, more_flags);
		return self_flags_;
	}

	virtual ptrdiff_t clr_self_flags(ptrdiff_t less_flags){
		ClrBits(self_flags_, less_flags);
		return self_flags_;
	}

	virtual ptrdiff_t self_flags() const{
		return self_flags_;
	}

	virtual bool Copy(const uint8_t* buffer, size_t n){
		// size_t len = static_cast<size_t>(this->end () - this->wr_ptr ());
		// Note that for this to work correct, end () *must* be >= mark ().
		size_t len = space();

		if((n == 0) || (len < n)) //error ENOSPC
			return false;
		else{
			BOOST_ASSERT(base());
			BOOST_ASSERT(wr_ptr());

			memcpy(wr_ptr(), buffer, n);
			set_offset_wr_ptr(n);

			return true;
		}
	}
};

template < int sizeT >
class Block: public BaseBlock{
public:
	enum{
		MAX_BUFFER_SIZE = sizeT
	};

	boost::array< uint8_t, MAX_BUFFER_SIZE > buffer_;

	Block(){
		memset(buffer_.data(), 0, MAX_BUFFER_SIZE);
		ReSet();
	}

	virtual uint8_t *const base() const{
		const uint8_t *const ptr = &buffer_[0];
		return const_cast<uint8_t *const>(ptr);
	}

	virtual uint8_t *const end() const{
		return base() + MAX_BUFFER_SIZE;
	}

	virtual size_t size() const{
		return MAX_BUFFER_SIZE;
	}
};

} // support
} // akva
#endif // SUPPORT_SIMPLE_BLOCK_H_
