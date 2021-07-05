#include "block.hpp"
#include "support/bit_operation.hpp"

#include <boost/thread.hpp>

// +++ functions ++

void intrusive_ptr_add_ref(const akva::DataFrame* ptr){
	ptr->refcount_.fetch_add(1, boost::memory_order_relaxed);
}

void intrusive_ptr_release(const akva::DataFrame* ptr){
	int count = ptr->refcount_.fetch_sub(1, boost::memory_order_release);
	if(count == 1){
		boost::atomic_thread_fence(boost::memory_order_acquire);
		delete ptr;
	}
}

void intrusive_ptr_add_ref(const akva::BurstFrame* ptr){
	ptr->refcount_.fetch_add(1, boost::memory_order_relaxed);
}

void intrusive_ptr_release(const akva::BurstFrame* ptr){
	int count = ptr->refcount_.fetch_sub(1, boost::memory_order_release);
	if(count == 1){
		boost::atomic_thread_fence(boost::memory_order_acquire);
		delete ptr;
	}
}

namespace akva{

#pragma pack(push, 4)

struct DataHdr{
	uint16_t counter;
	uint16_t pointer; // sizeof(len) in words !!!!
	uint64_t time;
};

struct BurstHdr{
	uint16_t len;
	uint16_t ext; // flags
};

#pragma pack(pop)

// +++ BurstFrame +++

BurstFrame::BurstFrame(DataFrame *const dblock, BurstHdr *const hdr, uint32_t* b, uint32_t* e)
	: refcount_(0), cont_(nullptr), b_(b), e_(e), hdr_(hdr){
	BOOST_ASSERT(dblock);
	data_ = dblock->add_link();
}

BurstFrame::~BurstFrame(){
	data_->free_link(data_);
	data_ = data_frame_ptr_t(); // Нужно, тк держит пока идет освобождение
}

uint8_t *const BurstFrame::ptr() const{
	return (uint8_t*)b_;
}

uint16_t BurstFrame::len() const{
	return (hdr_) ? hdr_->len : 0;
}

uint16_t BurstFrame::size() const{
	return ((e_ - b_) << 2);
}

uint16_t BurstFrame::space() const{
	return data_->space(e_);
}

uint16_t BurstFrame::set_flags(uint16_t more_flags){
	if(hdr_){
		SetBits(hdr_->ext, more_flags);
		return hdr_->ext;
	}
	else
		return 0;
}

uint16_t BurstFrame::clr_flags(uint16_t less_flags){
	if(hdr_){
		ClrBits(hdr_->ext, less_flags);
		return hdr_->ext;
	}
	else
		return 0;
}

uint64_t BurstFrame::time() const{
	return data()->time();
}

void BurstFrame::set_time(uint64_t time){
	data()->set_time(time);
}

uint16_t BurstFrame::flags() const{
	return (hdr_) ? hdr_->ext : 0;
}

DataFrame *const BurstFrame::data() const{
	BOOST_ASSERT(data_.get());
	return data_.get();
}

BurstFrame *const BurstFrame::cont() const{
	return cont_.get();
}

void BurstFrame::set_cont(BurstFrame* other){
	if(other){
		cont_ = burst_frame_ptr_t(other);
		// duplicate propertys
	}
	else
		cont_.reset();
}

bool BurstFrame::head() const{
	return ((uint32_t*)hdr_ == (b_ - 1)) ? true : false;
}

bool BurstFrame::complited() const{
	if(hdr_){
		uint16_t total(0);
		for(BurstFrame const* f = this; f != nullptr; f = f->cont())
			total += f->size();

		return (hdr_->len <= total) ? true : false;
	}
	else
		return false;
}

bool BurstFrame::fractional() const{
	return (hdr_ && (size() < hdr_->len)) ? true : false;
}

uint16_t BurstFrame::Copy(const uint8_t* buffer, uint16_t n){
	uint16_t writed = 0;
	BurstFrame* f = this;
	while(f && (writed < n)){
		uint16_t l = std::min< uint16_t >((n - writed), f->size());
		std::memcpy(f->ptr(), buffer + writed, l);
		writed += l;

		f = f->cont();
	}
	return writed;
}

// +++ DataFrame +++

/*
int ceil_int(double x){
	int i;
	const float round_towards_p_i = -0.5f;
	__asm
	{
		fld x
		fadd st, st(0)
		fsubr round_towards_p_i
		fistp i
		sar i, 1
	}
	return (-i);
}
*/

uint16_t DataFrame::ceil4(uint16_t x){
	uint16_t y = x >> 2;
	switch(x & 0x3){
	case 1:
	case 2:
	case 3:
		++y;
	} // switch
	return y;
}

DataFrame::DataFrame(uint64_t key, IAllocator *const allocator)
	: refcount_(0), lnk_count_(0), allocator_(allocator), base_(nullptr, 0){
	auto max_size = allocator_->capacity();

	base_ = allocator_->Aquare(key);
	hdr_ = (DataHdr*)base_.first;
}

DataFrame::DataFrame(IAllocator* const allocator, bool force)
	: refcount_(0), lnk_count_(0), allocator_(allocator), base_(nullptr, 0){
	auto max_size = allocator_->capacity();

	if(force || (allocator_->hot().first == nullptr)){
		base_ = allocator_->Aquare(0);
	}
	else{
		base_ = allocator_->hot();
	}

	hdr_ = (DataHdr*)base_.first;
}

DataFrame::~DataFrame(){
	if(base_.first){
		allocator_->Leave(base_.second);
	}
}

const uint64_t& DataFrame::key() const{
	return base_.second;
}

void DataFrame::set_counter(uint16_t counter){
	hdr_->counter = counter;
}

/*
burst_frame_ptr_t DataFrame::Clone(burst_frame_ptr_t frame){
    BOOST_ASSERT(frame->head());
    burst_frame_ptr_t burst = Make();
    // copy properties
    burst->hdr_->ext = frame->hdr_->ext;
    burst->set_flags(frame->flags());

    uint16_t seek = 0, len = frame->len();
    for(BurstFrame *c = frame.get(); c != nullptr; c = c->cont()){
        uint16_t n = std::min< uint16_t >(c->size(), len - seek);
        seek += burst->Copy(c->ptr(), n);
    }

    BOOST_ASSERT(seek == len);
    return burst;
}
*/

data_frame_ptr_t DataFrame::Allocate(){
	data_frame_ptr_t data = new DataFrame(allocator_, true);
	data->set_counter(hdr_->counter + 1);
	return data;
}

data_frame_ptr_t DataFrame::Make(burst_frame_ptr_t& burst, int sz){
	if(sz <= 0)
		return this;

	data_frame_ptr_t data = this;
	if(burst){ // large
		uint32_t* b = beg();
		uint32_t* e = b + ceil4(sz);
		if(e > end()){
			e = end();
			data = Allocate();
			hdr_->pointer = 0xffff;
		}
		else{
			hdr_->pointer = ceil4(sz);
		}

		burst_frame_ptr_t nburst = new BurstFrame(this, burst->hdr_, b, e);
		burst->set_cont(nburst.get());

		return data->Make(nburst, sz - nburst->size());
	}
	else{ // normal
		BurstHdr* h = nullptr;
		for(uint32_t* b = Head(h); b < end(); b = Next(b, h)){
			if(h->len > 0)
				continue;

			h->len = sz;
			++b; // head

			uint32_t* e = b + ceil4(sz);
			if(e > end()){
				e = end();
				data = Allocate();
			}

			burst = new BurstFrame(this, h, b, e);
			return data->Make(burst, sz - burst->size());
		}

		return Allocate()->Make(burst, sz);
	}
}

// A: prefix  Hbbbs.......
// B: middle  H...sbbbb...
// C: suffix  H......sbbbb
// D: all     Hbbbbbbbbbbb

uint32_t* DataFrame::Head(BurstHdr*& hdr){
	uint32_t *n = beg();
	if(hdr_->pointer == 0xffff){
		hdr = nullptr;
		n = end();
	}
	else if(hdr_->pointer > 0){
		n = (uint32_t*)(n + hdr_->pointer);
		hdr = (BurstHdr*)(n);
	}
	else{
		hdr = (BurstHdr*)(n);
		if(hdr->len > 0){
			n += 1; // move to data

			n = n + ceil4(hdr->len);
			if(n > end()) // data overhead
				n = end();
		}
	}

	return n;
}

uint32_t* DataFrame::Next(uint32_t* n, BurstHdr*& hdr){
	BOOST_ASSERT(n);
	hdr = (BurstHdr*)(n);

	if(hdr->len > 0){
		n += 1; // move to data

		n = n + ceil4(hdr->len);
		if(n > end()) // data overhead
			n = end();
	}

	return n;
}

uint16_t DataFrame::counter() const{
	return hdr_->counter;
}

uint16_t DataFrame::pointer() const{
	return hdr_->pointer;
}

uint64_t DataFrame::time() const{
	return hdr_->time;
}

void DataFrame::set_time(uint64_t time){
	hdr_->time = time;
}

uint8_t *const DataFrame::base() const{
	return (uint8_t*)base_.first;
}

uint32_t *const DataFrame::beg() const{
	return (uint32_t*)(base() + sizeof(DataHdr));
}

uint32_t *const DataFrame::end() const{
	return (uint32_t*)(base() + capacity());
}

uint16_t DataFrame::space(uint32_t* e) const{
	return ((end() - e) << 2);
}

uint16_t DataFrame::size() const{
	return ((uint8_t*)end() - (uint8_t*)beg());
}

uint16_t DataFrame::capacity() const{
	return allocator_->capacity();
}

data_frame_ptr_t DataFrame::add_link(){
	lnk_count_.fetch_add(1, boost::memory_order_seq_cst);
	return data_frame_ptr_t(this);
}

void DataFrame::free_link(data_frame_ptr_t holder){
	lnk_count_.fetch_sub(1, boost::memory_order_seq_cst);
}

size_t DataFrame::link_count() const{
	return lnk_count_.load(boost::memory_order_acquire);
}

// +++ FrameStack +++

FrameStack::FrameStack()
	: b2_(nullptr), e2_(nullptr){}

FrameStack::~FrameStack(){
	CleanUp();
}

void FrameStack::CleanUp(){
	f_ = burst_frame_ptr_t();
	b2_ = nullptr;
	e2_ = nullptr;
}

burst_frame_ptr_t FrameStack::Take(){
	BOOST_ASSERT(Complited());
	return f_;
}

bool FrameStack::Complited() const{
	if(f_){
		uint16_t bytes(0);
		BurstFrame* tail = f_.get();
		while(tail){
			bytes += tail->size();
			tail = tail->cont();
		}

		return (bytes >= f_->len()) ? true : false;
	}
	else
		return false;
}

FrameStack* FrameStack::Head(data_frame_ptr_t data){
	b2_ = (uint32_t*)data->beg();
	e2_ = (uint32_t*)data->end();

	if(b2_ == e2_)
		return nullptr;

	DataHdr* dh = data->hdr_;
	if(dh->pointer == 0xffff){
		uint16_t sz = (e2_ - b2_) << 2;
		if(f_){
			BurstFrame* tail = f_.get();
			while(tail->cont()){
				tail = tail->cont();
			}

			tail->set_cont(new BurstFrame(data.get(), f_->hdr(), b2_, e2_));
		}

		b2_ = e2_;
	}
	else if(dh->pointer > 0){
		uint32_t *c = b2_ + dh->pointer;

		if(c > e2_) // data overhead
			c = e2_;

		uint16_t sz = (c - b2_) << 2;
		if(f_){
			BurstFrame* tail = f_.get();
			while(tail->cont()){
				tail = tail->cont();
			}

			tail->set_cont(new BurstFrame(data.get(), f_->hdr(), b2_, c));
		}

		b2_ = c;
	}
	else{
		BurstHdr* bh = (BurstHdr*)b2_;
		if(bh->len > 0){
			b2_++;
			uint32_t *c = b2_ + DataFrame::ceil4(bh->len);

			if(c > e2_) // data overhead
				c = e2_;

			f_ = new BurstFrame(data.get(), bh, b2_, c);
			b2_ = c;
		}
		else
			return nullptr;
	}

	return this;
}

FrameStack* FrameStack::Next(data_frame_ptr_t data){
	if(b2_ == e2_)
		return nullptr;

	BurstHdr* bh = (BurstHdr*)b2_;
	if(bh->len > 0){
		b2_++;
		uint32_t *c = b2_ + DataFrame::ceil4(bh->len);

		if(c > e2_) // data overhead
			c = e2_;

		f_ = new BurstFrame(data.get(), bh, b2_, c);
		b2_ = c;
	}
	else
		return nullptr;

	return this;
}

} // akva

