#pragma once
#include <cstdint>
#include <stack>
#include <atomic>

#include <boost/intrusive_ptr.hpp>

#include "allocator.hpp"
#include "block.hpp"

class Frame;
class Burst;

namespace akva{

struct DataHdr;
struct BurstHdr;
struct FrameStack;
struct IAllocator;

} // akva

typedef boost::intrusive_ptr< Burst > burst_ptr_t;
typedef boost::intrusive_ptr< Frame > frame_ptr_t;

class Frame{
	friend class Burst;
	friend class FrameStack;

public:
	Frame(uint64_t key, akva::IAllocator *const allocator);
	Frame(akva::IAllocator *const allocator, bool force = false);
	~Frame();

	Frame(const Frame&) = delete;
	Frame& operator=(Frame&) = delete;

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

	frame_ptr_t Allocate();
	frame_ptr_t Make(burst_ptr_t& burst, int size);
	static uint16_t ceil4(uint16_t x);

//	burst_ptr_t Clone(burst_ptr_t frame);

	mutable std::atomic_int lnk_count_, refcount_;

//protected:
	frame_ptr_t add_link();
	void free_link(frame_ptr_t);
	size_t link_count() const;

	uint32_t* Head(akva::BurstHdr*& hdr);
	uint32_t* Next(uint32_t* c, akva::BurstHdr*& hdr);

	akva::DataHdr* hdr_;

private:
	akva::IAllocator::pointer_t base_;
	akva::IAllocator* const allocator_;
};

class Burst{
	friend class Frame;

public:
	Burst(Frame *const data, akva::BurstHdr *const hdr, uint32_t* b, uint32_t* e);
	~Burst();

	Burst(const Burst&) = delete;
	Burst& operator=(Burst&) = delete;

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

	Frame *const data() const;
	Burst *const cont() const;

	inline akva::BurstHdr* hdr(){
		return hdr_;
	}

	//protected:
	void set_cont(Burst* other);

	mutable std::atomic_int refcount_;

private:
	akva::BurstHdr* hdr_;
	uint32_t* b_, *e_;

	burst_ptr_t cont_; // Pointer to next message block in the chain.
	frame_ptr_t data_;
};

void intrusive_ptr_add_ref(Frame* p);
void intrusive_ptr_release(Frame* p);

void intrusive_ptr_add_ref(Burst* p);
void intrusive_ptr_release(Burst* p);

namespace akva{

struct FrameStack{
	FrameStack();
	~FrameStack();

	FrameStack* Head(frame_ptr_t df);
	FrameStack* Next(frame_ptr_t df);

	bool Complited() const;
	void CleanUp();

	burst_ptr_t Take();

	uint32_t *b2_, *e2_;
	burst_ptr_t f_;
};

} // akva
