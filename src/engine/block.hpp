#pragma once
#include <vector>
#include <cstdint>
#include <cstring>
#include <array>

#include <boost/assert.hpp>

#include "bit_utils.hpp"

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

namespace block{

class General: public IBlock{
public:
	ptrdiff_t rd_offset_; // Pointer to beginning of next read.
	ptrdiff_t wr_offset_; // Pointer to beginning of next write.

	ptrdiff_t self_flags_; // burst block
	ptrdiff_t flags_;      // data block 
	time_t moment_;

	virtual ~General(){
		ReSet();
	}

	bool MemMove() override{
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

	bool ReSize(size_t new_size) override{
		return false;
	}

	void set_rd_ptr(uint8_t *const ptr) override{
		rd_offset_ = ptr - base();
	}

	void set_wr_ptr(uint8_t *const ptr) override{
		wr_offset_ = ptr - base();
	}

	uint8_t *const rd_ptr() const override{
		return base() + rd_offset_;
	}

	uint8_t *const wr_ptr() const override{
		return base() + wr_offset_;
	}

	void set_offset_rd_ptr(size_t n) override{
		rd_offset_ += n;
	}

	void set_offset_wr_ptr(size_t n) override{
		wr_offset_ += n;
	}

	size_t space() const override{
		return end() - wr_ptr();
	}

	size_t length() const override{
		return wr_offset_ - rd_offset_;
	}

	void ReSet() override{
		rd_offset_ = wr_offset_ = 0;
		self_flags_ = flags_ = 0;
		moment_ = 0;
	}

	void set_time(time_t moment) override{
		moment_ = moment;
	}

	time_t time() const override{
		return moment_;
	}

	ptrdiff_t set_flags(ptrdiff_t more_flags) override{
		SetBits(flags_, more_flags);
		return flags_;
	}

	ptrdiff_t clr_flags(ptrdiff_t less_flags) override{
		ClrBits(flags_, less_flags);
		return flags_;
	}

	ptrdiff_t flags() const override{
		return flags_;
	}

	ptrdiff_t set_self_flags(ptrdiff_t more_flags) override{
		SetBits(self_flags_, more_flags);
		return self_flags_;
	}

	ptrdiff_t clr_self_flags(ptrdiff_t less_flags) override{
		ClrBits(self_flags_, less_flags);
		return self_flags_;
	}

	ptrdiff_t self_flags() const override{
		return self_flags_;
	}

	bool Copy(const uint8_t* buffer, size_t n) override{
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
class Array: public General{
public:
	enum{
		MAX_BUFFER_SIZE = sizeT
	};

	std::array< uint8_t, MAX_BUFFER_SIZE > buffer_;

	Array(){
		memset(buffer_.data(), 0, MAX_BUFFER_SIZE);
		ReSet();
	}

	uint8_t *const base() const override{
		const uint8_t *const ptr = &buffer_[0];
		return const_cast<uint8_t *const>(ptr);
	}

	uint8_t *const end() const override{
		return base() + MAX_BUFFER_SIZE;
	}

	size_t size() const override{
		return MAX_BUFFER_SIZE;
	}
};

} // block 
} // akva
