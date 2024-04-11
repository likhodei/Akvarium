#pragma once
#include <atomic>
#include <cstdint>

#include <boost/intrusive_ptr.hpp>

#include "allocator.hpp"
#include "block.hpp"

class Buffer: public akva::block::General{
public:
	Buffer(akva::IAllocator *const allocator);
	~Buffer();

	Buffer(const Buffer&) = delete;
	Buffer& operator=(Buffer&) = delete;

    uint8_t *const base() const override;
	uint8_t *const end() const override;
	size_t size() const override;

	mutable std::atomic_int refcount_;

protected:
	akva::IAllocator::pointer_t base_;
	akva::IAllocator* const allocator_;
};

void intrusive_ptr_add_ref(Buffer* b);
void intrusive_ptr_release(Buffer* b);

namespace akva{

typedef boost::intrusive_ptr< Buffer > buffer_ptr_t;

} //akva
