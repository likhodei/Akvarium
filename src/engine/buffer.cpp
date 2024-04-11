#include "buffer.hpp"

using namespace akva;

void intrusive_ptr_add_ref(Buffer* ptr){
	ptr->refcount_.fetch_add(1, std::memory_order_relaxed);
}

void intrusive_ptr_release(Buffer* ptr){
	int count = ptr->refcount_.fetch_sub(1, std::memory_order_release);
	if(count == 1){
		std::atomic_thread_fence(std::memory_order_acquire);
		delete ptr;
	}
}

// +++ Buffer +++

Buffer::Buffer(akva::IAllocator *const allocator)
: refcount_(0), allocator_(allocator), base_(nullptr, 0){
    base_ = allocator_->Aquare(0);
    ReSet();
}

Buffer::~Buffer(){
    if(base_.first)
        allocator_->Leave(base_.second);
}

uint8_t *const Buffer::base() const{
    return const_cast< uint8_t *const >((uint8_t*)base_.first);
}

uint8_t *const Buffer::end() const
{ return base() + allocator_->capacity(); }

size_t Buffer::size() const
{ return allocator_->capacity(); }

