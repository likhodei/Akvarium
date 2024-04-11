#pragma once
#include <list>
#include <string>

namespace akva{
namespace notify{

enum SeverityLevel{
    trace = 1 << 1,
    debug = 1 << 2,
    info = 1 << 3,
    warning = 1 << 4,
    error = 1 << 5,
    fatal = 1 << 6,
    DANGER_LEVELS = (warning | error | fatal)
};

enum Colour{
    COLOUR_NOT_SET = -1,
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
    virtual ~INotification();

    void Trace(const std::string& message, const std::string& tag = "", notify::Colour color = notify::BLACK);
    void Debug(const std::string& message, const std::string& tag = "", notify::Colour color = notify::BLACK);
    void Info(const std::string& message, const std::string& tag = "", notify::Colour color = notify::BLACK);
    void Warning(const std::string& message, const std::string& tag = "", notify::Colour color = notify::BLACK);
    void Error(const std::string& message, const std::string& tag = "", notify::Colour color = notify::BLACK);
    void Fatal(const std::string& message, const std::string& tag = "", notify::Colour color = notify::BLACK);

protected:
    virtual void do_trace(const std::string& msg, const std::string& tag, notify::Colour c) = 0;
    virtual void do_debug(const std::string& msg, const std::string& tag, notify::Colour c) = 0;
    virtual void do_info(const std::string& msg, const std::string& tag, notify::Colour c) = 0;
    virtual void do_warning(const std::string& msg, const std::string& tag, notify::Colour c) = 0;
    virtual void do_error(const std::string& msg, const std::string& tag, notify::Colour c) = 0;
    virtual void do_fatal(const std::string& msg, const std::string& tag, notify::Colour c) = 0;
};

namespace notify{

class Backend: public INotification{
public:
    virtual ~Backend();

    void Attach(INotification *const assignment);
    void Detach(INotification *const assignment);

protected:
    void do_trace(const std::string& msg, const std::string& tag, Colour c) override;
    void do_debug(const std::string& msg, const std::string& tag, Colour c) override;
    void do_info(const std::string& msg, const std::string& tag, Colour c) override;
    void do_warning(const std::string& msg, const std::string& tag, Colour c) override;
    void do_error(const std::string& msg, const std::string& tag, Colour c) override;
    void do_fatal(const std::string& msg, const std::string& tag, Colour c) override;

private:
    std::list< INotification* > to_;
};

} // notify
} // akva
