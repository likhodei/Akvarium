#include "notification.hpp"

namespace akva{

// +++ INotification +++

INotification::~INotification()
{ }

void INotification::Trace(const std::string& message, const std::string& tag, notify::Colour color){
    do_trace(message, tag, color);
}

void INotification::Debug(const std::string& message, const std::string& tag, notify::Colour color){
    do_debug(message, tag, color);
}

void INotification::Info(const std::string& message, const std::string& tag, notify::Colour color){
    do_info(message, tag, color);
}

void INotification::Warning(const std::string& message, const std::string& tag, notify::Colour color){
    do_warning(message, tag, color);
}

void INotification::Error(const std::string& message, const std::string& tag, notify::Colour color){
    do_error(message, tag, color);
}

void INotification::Fatal(const std::string& message, const std::string& tag, notify::Colour color){
    do_fatal(message, tag, color);
}

} // akva

using namespace akva::notify;

// +++ Backend +++

Backend::~Backend(){
	to_.clear();
}

void Backend::Attach(INotification *const assignment){
	to_.push_back(assignment);
}

void Backend::Detach(INotification *const assignment){
	to_.remove(assignment);
}

void Backend::do_trace(const std::string& message, const std::string& type, Colour colour){
	for(auto i : to_)
		i->Trace(message, type, colour);
}

void Backend::do_debug(const std::string& message, const std::string& type, Colour colour){
	for(auto i : to_)
		i->Debug(message, type, colour);
}

void Backend::do_info(const std::string& message, const std::string& type, Colour colour){
	for(auto i : to_)
		i->Info(message, type, colour);
}

void Backend::do_warning(const std::string& message, const std::string& type, Colour colour){
	for(auto i : to_)
		i->Warning(message, type, colour);
}

void Backend::do_error(const std::string& message, const std::string& type, Colour colour){
	for(auto i : to_)
		i->Error(message, type, colour);
}

void Backend::do_fatal(const std::string& message, const std::string& type, Colour colour){
	for(auto i : to_)
		i->Fatal(message, type, colour);
}
