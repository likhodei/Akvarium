#pragma once
#ifndef NOTIFY_BACKEND_H_
#define NOTIFY_BACKEND_H_

#include "notification_interface.hpp"
#include <list>

namespace akva{
namespace notify{

class NotifyBackend: public INotification{
public:
    virtual ~NotifyBackend();

    void Attach(INotification *const /*assignment*/);
    void Detach(INotification *const /*assignment*/);

protected:
    virtual void do_trace(const std::string& /*message*/, const std::string& /*type*/, tGlobalColor /*color*/);
    virtual void do_debug(const std::string& /*message*/, const std::string& /*type*/, tGlobalColor /*color*/);
    virtual void do_info(const std::string& /*message*/, const std::string& /*type*/, tGlobalColor /*color*/);
    virtual void do_warning(const std::string& /*message*/, const std::string& /*type*/, tGlobalColor /*color*/);
    virtual void do_error(const std::string& /*message*/, const std::string& /*type*/, tGlobalColor /*color*/);
    virtual void do_fatal(const std::string& /*message*/, const std::string& /*type*/, tGlobalColor /*color*/);

private:
    std::list< INotification* > to_;
};

} // notify
} // akva

#endif // NOTIFY_BACKEND_H_ 