#pragma once
#ifndef ENGINE_MEM_BLOCK_H_
#define ENGINE_MEM_BLOCK_H_

#include "support/simple_block.hpp"
#include "block_interface.hpp"

#include <boost/atomic.hpp>
#include <boost/utility.hpp>
#include <boost/intrusive_ptr.hpp>

#include <cstdint>
#include <stack>

namespace akva{
class DataFrame;
class BurstFrame;
} // akva

void intrusive_ptr_add_ref(const akva::DataFrame* p);
void intrusive_ptr_release(const akva::DataFrame* p);

void intrusive_ptr_add_ref(const akva::BurstFrame* p);
void intrusive_ptr_release(const akva::BurstFrame* p);

typedef boost::intrusive_ptr< akva::BurstFrame > burst_frame_ptr_t;
typedef boost::intrusive_ptr< akva::DataFrame > data_frame_ptr_t;

namespace akva{

struct DataHdr;
struct BurstHdr;

struct IAllocator;

struct FrameStack{
	FrameStack();
	~FrameStack();

	FrameStack* Head(data_frame_ptr_t df);
	FrameStack* Next(data_frame_ptr_t df);

	bool Complited() const;
	void CleanUp();

	burst_frame_ptr_t Take();

	uint32_t *b2_, *e2_;
	burst_frame_ptr_t f_;
};

class DataFrame: private boost::noncopyable{
	friend class BurstFrame;
	friend class FrameStack;
	friend void ::intrusive_ptr_add_ref(const DataFrame*);
	friend void ::intrusive_ptr_release(const DataFrame*);

public:
	DataFrame(uint64_t key, IAllocator *const allocator);
	DataFrame(IAllocator *const allocator, bool force = false);
	~DataFrame();

	const uint64_t& key() const;

	uint16_t counter() const;
	void set_counter(uint16_t value);
	uint16_t pointer() const;

	uint8_t* const base() const; // for all frame
	uint16_t capacity() const;   // for all frame

	uint32_t *const beg() const;
	uint32_t *const end() const;

	uint16_t size() const;
	uint16_t space(uint32_t* e) const;

	uint64_t time() const;
	void set_time(uint64_t /*time*/);

	data_frame_ptr_t Allocate();
	data_frame_ptr_t Make(burst_frame_ptr_t& burst, int size);
//	burst_frame_ptr_t Clone(burst_frame_ptr_t frame);
	static uint16_t ceil4(uint16_t x);

protected:
	data_frame_ptr_t add_link();
	void free_link(data_frame_ptr_t);
	size_t link_count() const;

	uint32_t* Head(BurstHdr*& hdr);
	uint32_t* Next(uint32_t* c, BurstHdr*& hdr);

private:
	DataHdr* hdr_;

	IAllocator::pointer_t base_;
	IAllocator* const allocator_;

	mutable boost::atomic_int lnk_count_, refcount_;
};

class BurstFrame: private boost::noncopyable{
	friend class DataFrame;
	friend void ::intrusive_ptr_add_ref(const BurstFrame*);
	friend void ::intrusive_ptr_release(const BurstFrame*);

public:
	BurstFrame(DataFrame *const data, BurstHdr *const hdr, uint32_t* b, uint32_t* e);
	~BurstFrame();

	uint16_t Copy(const uint8_t* buffer, uint16_t n);

	uint8_t *const ptr() const;
	uint16_t len() const;
	uint16_t space() const;
	uint16_t size() const;

	bool head() const;       // this part is top
	bool complited() const;  // all parts exist
	bool fractional() const; // frame have more one parts

	uint16_t set_flags(uint16_t more_flags);
	uint16_t clr_flags(uint16_t less_flags);
	uint16_t flags() const;

	uint64_t time() const;
	void set_time(uint64_t tmark);

	DataFrame *const data() const;
	BurstFrame *const cont() const;

	inline BurstHdr* hdr(){
		return hdr_;
	}

	//protected:
	void set_cont(BurstFrame* other);

private:
	BurstHdr* hdr_;
	uint32_t* b_, *e_;

	burst_frame_ptr_t cont_; // Pointer to next message block in the chain.
	data_frame_ptr_t data_;
	mutable boost::atomic_int refcount_;
};

} // akva
#endif // ENGINE_MEM_BLOCK_H_
