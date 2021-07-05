#pragma once
#ifndef ENGINE_BUFFER_H_
#define ENGINE_BUFFER_H_

#include "block_interface.hpp"
#include "support/simple_block.hpp"

#include <boost/core/noncopyable.hpp>
#include <boost/atomic.hpp>
#include <boost/intrusive_ptr.hpp>

#include <cstdint>

namespace akva{
class Buffer;
} // akva

void intrusive_ptr_add_ref(const akva::Buffer* b);
void intrusive_ptr_release(const akva::Buffer* b);

namespace akva{

class Buffer: public support::BaseBlock, private boost::noncopyable{
public:
	Buffer(IAllocator *const allocator);
	~Buffer();

    uint8_t *const base() const;
	uint8_t *const end() const;
	size_t size() const;

	mutable boost::atomic_int refcount_;

protected:
	IAllocator::pointer_t base_;
	IAllocator* const allocator_;
};

typedef boost::intrusive_ptr< Buffer > buffer_ptr_t;

} //akva
#endif // ENGINE_BUFFER_H_
