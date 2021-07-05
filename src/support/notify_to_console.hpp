#pragma once
#ifndef SUPPORT_NOTIFY_TO_CONSOLE_H_
#define SUPPORT_NOTIFY_TO_CONSOLE_H_

#include "notification_interface.hpp"

#include <boost/array.hpp>

#include <cstdint>
#include <string>

namespace akva{
namespace notify{

struct INotifyWriter{
    virtual ~INotifyWriter(){}

    virtual void Commit(const std::string& tag, const std::string& message) = 0;
};

class Notify2Console: public INotification{
    enum{
        LVL_TRACE = 0,
        LVL_DEBUG,
        LVL_INFO,
        LVL_WARNING,
        LVL_ERROR,
        LVL_FATAL,
        MAX_LVL_COUNT
    };

public:
    Notify2Console(uint32_t lvls = DANGER_LEVELS);
    ~Notify2Console();

protected:
    virtual void do_trace(const std::string& /*message*/, const std::string& /*type*/, tGlobalColor /*color*/);
    virtual void do_debug(const std::string& /*message*/, const std::string& /*type*/, tGlobalColor /*color*/);
    virtual void do_info(const std::string& /*message*/, const std::string& /*type*/, tGlobalColor /*color*/);
    virtual void do_warning(const std::string& /*message*/, const std::string& /*type*/, tGlobalColor /*color*/);
    virtual void do_error(const std::string& /*message*/, const std::string& /*type*/, tGlobalColor /*color*/);
    virtual void do_fatal(const std::string& /*message*/, const std::string& /*type*/, tGlobalColor /*color*/);

private:
    boost::array< INotifyWriter*, MAX_LVL_COUNT > writers_;
};

} // notify
} // akva
#endif // SUPPORT_NOTIFY_TO_CONSOLE_H_
