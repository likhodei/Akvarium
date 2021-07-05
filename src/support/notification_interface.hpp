#pragma once
#ifndef NOTIFICATION_INTERFACE_H_
#define NOTIFICATION_INTERFACE_H_

#include <string>

namespace akva{
namespace notify{

enum eSupportNotifyLvl{
    LEVEL_TRACE   = 1 << 1,
    LEVEL_DEBUG   = 1 << 2,
    LEVEL_INFO    = 1 << 3,
    LEVEL_WARNING = 1 << 4,
    LEVEL_ERROR   = 1 << 5,
    LEVEL_FATAL   = 1 << 6, 
    DANGER_LEVELS = (LEVEL_WARNING | LEVEL_ERROR | LEVEL_FATAL)
};

enum tGlobalColor{
    COLOR_NOT_SET = -1,
    WHITE = 0,
    BLACK = 1,
    RED = 2,
    DARKRED = 3,
    GREEN = 4,
    DARKGREEN = 5,
    BLUE = 6,
    DARKBLUE = 7,
    CYAN = 8,
    DARKCYAN = 9,
    MAGENTA = 10,
    DARKMAGENTA = 11,
    YELLOW = 12,
    DARKYELLOW = 13,
    GRAY = 14,
    DARKGRAY = 15,
    LIGHTGRAY = 16
};

} // notify

struct INotification{
    inline virtual ~INotification(){}

    inline void Trace(const std::string& message, const std::string& tag = "", notify::tGlobalColor color = notify::BLACK){
        do_trace(message, tag, color);
    }

    inline void Debug(const std::string& message, const std::string& tag = "", notify::tGlobalColor color = notify::BLACK){
        do_debug(message, tag, color);
    }

    inline void Info(const std::string& message, const std::string& tag = "", notify::tGlobalColor color = notify::BLACK){
        do_info(message, tag, color);
    }

    inline void Warning(const std::string& message, const std::string& tag = "", notify::tGlobalColor color = notify::BLACK){
        do_warning(message, tag, color);
    }

    inline void Error(const std::string& message, const std::string& tag = "", notify::tGlobalColor color = notify::BLACK){
        do_error(message, tag, color);
    }

    inline void Fatal(const std::string& message, const std::string& tag = "", notify::tGlobalColor color = notify::BLACK){
        do_fatal(message, tag, color);
    }

protected:
    virtual void do_trace(const std::string& msg, const std::string& tag, notify::tGlobalColor /*color*/) = 0;
    virtual void do_debug(const std::string& msg, const std::string& tag, notify::tGlobalColor /*color*/) = 0;
    virtual void do_info(const std::string& msg, const std::string& tag, notify::tGlobalColor /*color*/) = 0;
    virtual void do_warning(const std::string& msg, const std::string& tag, notify::tGlobalColor /*color*/) = 0;
    virtual void do_error(const std::string& msg, const std::string& tag, notify::tGlobalColor /*color*/) = 0;
    virtual void do_fatal(const std::string& msg, const std::string& tag, notify::tGlobalColor /*color*/) = 0;
};

} // akva
#endif // NOTIFICATION_INTERFACE_H_