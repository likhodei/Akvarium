#include "frame.hpp"
#include "bit_utils.hpp"

// +++ functions ++

void intrusive_ptr_add_ref(Frame* ptr){
	ptr->refcount_.fetch_add(1, std::memory_order_relaxed);
}

void intrusive_ptr_add_ref(Burst* ptr){
	ptr->refcount_.fetch_add(1, std::memory_order_relaxed);
}

void intrusive_ptr_release(Frame* ptr){
	int count = ptr->refcount_.fetch_sub(1, std::memory_order_release);
	if(count == 1){
		std::atomic_thread_fence(std::memory_order_acquire);
		delete ptr;
	}
}

void intrusive_ptr_release(Burst* ptr){
	int count = ptr->refcount_.fetch_sub(1, std::memory_order_release);
	if(count == 1){
		std::atomic_thread_fence(std::memory_order_acquire);
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

} // akva

using namespace akva;

// +++ Burst +++

Burst::Burst(Frame *const dblock, akva::BurstHdr *const hdr, uint32_t* b, uint32_t* e)
	: refcount_(0), cont_(nullptr), b_(b), e_(e), hdr_(hdr){
	BOOST_ASSERT(dblock);
	data_ = dblock->add_link();
}

Burst::~Burst(){
	data_->free_link(data_);
	data_ = frame_ptr_t(); // Нужно, тк держит пока идет освобождение
}

uint8_t *const Burst::ptr() const{
	return (uint8_t*)b_;
}

uint16_t Burst::len() const{
	return (hdr_) ? hdr_->len : 0;
}

uint16_t Burst::size() const{
	return ((e_ - b_) << 2);
}

uint16_t Burst::space() const{
	return data_->space(e_);
}

uint16_t Burst::set_flags(uint16_t more_flags){
	if(hdr_){
		SetBits(hdr_->ext, more_flags);
		return hdr_->ext;
	}
	else
		return 0;
}

uint16_t Burst::clr_flags(uint16_t less_flags){
	if(hdr_){
		ClrBits(hdr_->ext, less_flags);
		return hdr_->ext;
	}
	else
		return 0;
}

uint64_t Burst::time() const{
	return data()->time();
}

void Burst::set_time(uint64_t time){
	data()->set_time(time);
}

uint16_t Burst::flags() const{
	return (hdr_) ? hdr_->ext : 0;
}

Frame *const Burst::data() const{
	BOOST_ASSERT(data_.get());
	return data_.get();
}

Burst *const Burst::cont() const{
	return cont_.get();
}

void Burst::set_cont(Burst* other){
	if(other){
		cont_ = burst_ptr_t(other);
		// duplicate propertys
	}
	else
		cont_.reset();
}

bool Burst::head() const{
	return ((uint32_t*)hdr_ == (b_ - 1)) ? true : false;
}

bool Burst::complited() const{
	if(hdr_){
		uint16_t total(0);
		for(Burst const* f = this; f != nullptr; f = f->cont())
			total += f->size();

		return (hdr_->len <= total) ? true : false;
	}
	else
		return false;
}

bool Burst::fractional() const{
	return (hdr_ && (size() < hdr_->len)) ? true : false;
}

uint16_t Burst::Copy(const uint8_t* buffer, uint16_t n){
	uint16_t writed = 0;
	Burst* f = this;
	while(f && (writed < n)){
		uint16_t l = std::min< uint16_t >((n - writed), f->size());
		std::memcpy(f->ptr(), buffer + writed, l);
		writed += l;

		f = f->cont();
	}
	return writed;
}

// +++ Frame +++

uint16_t Frame::ceil4(uint16_t x){
	uint16_t y = x >> 2;
	switch(x & 0x3){
	case 1:
	case 2:
	case 3:
		++y;
	} // switch
	return y;
}

Frame::Frame(uint64_t key, IAllocator *const allocator)
	: refcount_(0), lnk_count_(0), allocator_(allocator), base_(nullptr, 0){
	auto max_size = allocator_->capacity();

	base_ = allocator_->Aquare(key);
	hdr_ = (akva::DataHdr*)base_.first;
}

Frame::Frame(IAllocator* const allocator, bool force)
	: refcount_(0), lnk_count_(0), allocator_(allocator), base_(nullptr, 0){
	auto max_size = allocator_->capacity();

	if(force || (allocator_->hot().first == nullptr)){
		base_ = allocator_->Aquare(0);
	}
	else{
		base_ = allocator_->hot();
	}

	hdr_ = (akva::DataHdr*)base_.first;
}

Frame::~Frame(){
	if(base_.first){
		allocator_->Leave(base_.second);
	}
}

const uint64_t& Frame::key() const{
	return base_.second;
}

void Frame::set_counter(uint16_t counter){
	hdr_->counter = counter;
}

/*
burst_ptr_t Frame::Clone(burst_ptr_t frame){
    BOOST_ASSERT(frame->head());
    burst_ptr_t burst = Make();
    // copy properties
    burst->hdr_->ext = frame->hdr_->ext;
    burst->set_flags(frame->flags());

    uint16_t seek = 0, len = frame->len();
    for(Burst *c = frame.get(); c != nullptr; c = c->cont()){
        uint16_t n = std::min< uint16_t >(c->size(), len - seek);
        seek += burst->Copy(c->ptr(), n);
    }

    BOOST_ASSERT(seek == len);
    return burst;
}
*/

frame_ptr_t Frame::Allocate(){
	frame_ptr_t data = new Frame(allocator_, true);
	data->set_counter(hdr_->counter + 1);
	return data;
}

frame_ptr_t Frame::Make(burst_ptr_t& burst, int sz){
	if(sz <= 0)
		return this;

	frame_ptr_t data = this;
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

		burst_ptr_t nburst = new Burst(this, burst->hdr_, b, e);
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

			burst = new Burst(this, h, b, e);
			return data->Make(burst, sz - burst->size());
		}

		return Allocate()->Make(burst, sz);
	}
}

// A: prefix  Hbbbs.......
// B: middle  H...sbbbb...
// C: suffix  H......sbbbb
// D: all     Hbbbbbbbbbbb

uint32_t* Frame::Head(BurstHdr*& hdr){
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

uint32_t* Frame::Next(uint32_t* n, BurstHdr*& hdr){
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

uint16_t Frame::counter() const{
	return hdr_->counter;
}

uint16_t Frame::pointer() const{
	return hdr_->pointer;
}

uint64_t Frame::time() const{
	return hdr_->time;
}

void Frame::set_time(uint64_t time){
	hdr_->time = time;
}

uint8_t *const Frame::base() const{
	return (uint8_t*)base_.first;
}

uint32_t *const Frame::beg() const{
	return (uint32_t*)(base() + sizeof(akva::DataHdr));
}

uint32_t *const Frame::end() const{
	return (uint32_t*)(base() + capacity());
}

uint16_t Frame::space(uint32_t* e) const{
	return ((end() - e) << 2);
}

uint16_t Frame::size() const{
	return ((uint8_t*)end() - (uint8_t*)beg());
}

uint16_t Frame::capacity() const{
	return allocator_->capacity();
}

frame_ptr_t Frame::add_link(){
	lnk_count_.fetch_add(1, std::memory_order_seq_cst);
	return frame_ptr_t(this);
}

void Frame::free_link(frame_ptr_t holder){
	lnk_count_.fetch_sub(1, std::memory_order_seq_cst);
}

size_t Frame::link_count() const{
	return lnk_count_.load(std::memory_order_acquire);
}

namespace akva{

// +++ FrameStack +++

FrameStack::FrameStack()
: b2_(nullptr), e2_(nullptr)
{ }

FrameStack::~FrameStack(){
	CleanUp();
}

void FrameStack::CleanUp(){
	f_ = burst_ptr_t();
	b2_ = nullptr;
	e2_ = nullptr;
}

burst_ptr_t FrameStack::Take(){
	BOOST_ASSERT(Complited());
	return f_;
}

bool FrameStack::Complited() const{
	if(f_){
		uint16_t bytes(0);
		Burst* tail = f_.get();
		while(tail){
			bytes += tail->size();
			tail = tail->cont();
		}

		return (bytes >= f_->len()) ? true : false;
	}
	else
		return false;
}

FrameStack* FrameStack::Head(frame_ptr_t data){
	b2_ = (uint32_t*)data->beg();
	e2_ = (uint32_t*)data->end();

	if(b2_ == e2_)
		return nullptr;

	akva::DataHdr* dh = data->hdr_;
	if(dh->pointer == 0xffff){
		uint16_t sz = (e2_ - b2_) << 2;
		if(f_){
			Burst* tail = f_.get();
			while(tail->cont()){
				tail = tail->cont();
			}

			tail->set_cont(new Burst(data.get(), f_->hdr(), b2_, e2_));
		}

		b2_ = e2_;
	}
	else if(dh->pointer > 0){
		uint32_t *c = b2_ + dh->pointer;

		if(c > e2_) // data overhead
			c = e2_;

		uint16_t sz = (c - b2_) << 2;
		if(f_){
			Burst* tail = f_.get();
			while(tail->cont()){
				tail = tail->cont();
			}

			tail->set_cont(new Burst(data.get(), f_->hdr(), b2_, c));
		}

		b2_ = c;
	}
	else{
		BurstHdr* bh = (BurstHdr*)b2_;
		if(bh->len > 0){
			b2_++;
			uint32_t *c = b2_ + Frame::ceil4(bh->len);

			if(c > e2_) // data overhead
				c = e2_;

			f_ = new Burst(data.get(), bh, b2_, c);
			b2_ = c;
		}
		else
			return nullptr;
	}

	return this;
}

FrameStack* FrameStack::Next(frame_ptr_t data){
	if(b2_ == e2_)
		return nullptr;

	BurstHdr* bh = (BurstHdr*)b2_;
	if(bh->len > 0){
		b2_++;
		uint32_t *c = b2_ + Frame::ceil4(bh->len);

		if(c > e2_) // data overhead
			c = e2_;

		f_ = new Burst(data.get(), bh, b2_, c);
		b2_ = c;
	}
	else
		return nullptr;

	return this;
}

} // akva


