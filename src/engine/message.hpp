#pragma once
#ifndef ENGINE_MESSAGE_H_
#define ENGINE_MESSAGE_H_

#include "block.hpp"
#include <cstdint>

namespace akva{

class MessageView{
public:
	MessageView(burst_frame_ptr_t frame);
	~MessageView();

	uint8_t *const ptr();
	uint16_t length() const;

	template< class T >
	T* const As(){
		return (T *const)(ptr());
	}

protected:
	uint8_t* ptr_;
	burst_frame_ptr_t fr_;
};

struct Message{
	Message();
	Message(burst_frame_ptr_t body);
	Message(uint16_t size, data_frame_ptr_t data);

	bool ok() const;

	uint64_t time() const;
	void set_time(uint64_t /*time*/);

	uint16_t set_flags(uint16_t more_flags);
	uint16_t clr_flags(uint16_t less_flags);
	uint16_t flags() const;

	uint16_t Copy(const uint8_t* buffer, uint16_t n);
	MessageView view();
	burst_frame_ptr_t body_;
};

class FrameView{
public:
	FrameView(burst_frame_ptr_t frame);
	~FrameView();

	uint8_t *const ptr();
	uint16_t length() const;

protected:
	uint8_t* ptr_;
	burst_frame_ptr_t fr_;
};

} // akva
#endif // ENGINE_MESSAGE_H_

