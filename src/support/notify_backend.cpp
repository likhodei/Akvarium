#include "notify_backend.hpp"

namespace akva{
namespace notify{

NotifyBackend::~NotifyBackend(){
	to_.clear();
}

void NotifyBackend::Attach(INotification *const assignment){
	to_.push_back(assignment);
}

void NotifyBackend::Detach(INotification *const assignment){
	to_.remove(assignment);
}

void NotifyBackend::do_trace(const std::string& message, const std::string& type, tGlobalColor color){
	for(auto i : to_)
		i->Trace(message, type, color);
}

void NotifyBackend::do_debug(const std::string& message, const std::string& type, tGlobalColor color){
	for(auto i : to_)
		i->Debug(message, type, color);
}

void NotifyBackend::do_info(const std::string& message, const std::string& type, tGlobalColor color){
	for(auto i : to_)
		i->Info(message, type, color);
}

void NotifyBackend::do_warning(const std::string& message, const std::string& type, tGlobalColor color){
	for(auto i : to_)
		i->Warning(message, type, color);
}

void NotifyBackend::do_error(const std::string& message, const std::string& type, tGlobalColor color){
	for(auto i : to_)
		i->Error(message, type, color);
}

void NotifyBackend::do_fatal(const std::string& message, const std::string& type, tGlobalColor color){
	for(auto i : to_)
		i->Fatal(message, type, color);
}

} // notify
} // akva
