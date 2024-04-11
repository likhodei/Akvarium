#pragma once
#include <cstdint>
#include <string>
#include <array>

#include "notification.hpp"

namespace akva{
namespace notify{

struct INotifyWriter{
    virtual ~INotifyWriter()
    { }

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
    void do_trace(const std::string& msg, const std::string& tag, Colour c) override;
    void do_debug(const std::string& msg, const std::string& tag, Colour c) override;
    void do_info(const std::string& msg, const std::string& tag, Colour c) override;
    void do_warning(const std::string& msg, const std::string& tag, Colour c) override;
    void do_error(const std::string& msg, const std::string& tag, Colour c) override;
    void do_fatal(const std::string& msg, const std::string& tag, Colour c) override;

private:
    std::array< INotifyWriter*, MAX_LVL_COUNT > writers_;
};

} // notify
} // akva
