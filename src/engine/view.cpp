#include "view.hpp"

using namespace akva;

// +++ FrameView +++

FrameView::FrameView(burst_ptr_t frame)
: fr_(frame), ptr_(nullptr){
	if(fr_->fractional()){
		uint16_t l = fr_->len();
		ptr_ = new uint8_t[l];

		uint16_t offset(0);
		for(Burst *more = fr_.get(); more; more = more->cont()){
			auto sz = std::min< uint16_t >(l - offset, more->size());
			memcpy(ptr_ + offset, more->ptr(), sz);
			offset += sz;
		}
	}
	else
		ptr_ = fr_->ptr();
}

FrameView::~FrameView(){
	if(ptr_ != fr_->ptr()){
		delete [] ptr_;
		ptr_ = nullptr;
	}
}

uint8_t* const FrameView::ptr(){
	return ptr_;
}

uint16_t FrameView::length() const{
	return fr_->len();
}

// +++ Message +++

Message::Message()
{ }

Message::Message(burst_ptr_t body)
: body_(body)
{ }

Message::Message(uint16_t size, frame_ptr_t data){
	data->Make(body_, size);
	BOOST_ASSERT(body_);
}

MessageView Message::view(){
	return MessageView(body_);
}

uint16_t Message::Copy(const uint8_t* buffer, uint16_t n){
	return body_->Copy(buffer, n);
}

uint64_t Message::time() const{
	return body_->time();
}

void Message::set_time(uint64_t time){
	return body_->set_time(time);
}

uint16_t Message::set_flags(uint16_t more_flags){
	return body_->set_flags(more_flags);
}

uint16_t Message::clr_flags(uint16_t less_flags){
	return body_->clr_flags(less_flags);
}

uint16_t Message::flags() const{
	return body_->flags();
}

bool Message::ok() const{
	return (body_)? true : false;
}

// +++ MessageView +++

MessageView::MessageView(burst_ptr_t frame)
: fr_(frame), ptr_(nullptr){
	if(fr_->fractional()){
		uint16_t l = fr_->len();
		ptr_ = new uint8_t[l];

		uint16_t offset(0);
		for(Burst *more = fr_.get(); more; more = more->cont()){
			auto sz = std::min< uint16_t >(l - offset, more->size());
			memcpy(ptr_ + offset, more->ptr(), sz);
			offset += sz;
		}
	}
	else
		ptr_ = fr_->ptr();
}

MessageView::~MessageView(){
	if(ptr_ != fr_->ptr()){
		delete [] ptr_;
		ptr_ = nullptr;
	}
}

uint8_t* const MessageView::ptr(){
	return ptr_;
}

uint16_t MessageView::length() const{
	return fr_->len();
}
