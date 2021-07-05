#pragma once 
#ifndef SUPPORT_LOGGER_H_
#define SUPPORT_LOGGER_H_

#include "notification_interface.hpp"
#include <boost/scoped_ptr.hpp>

namespace akva{
namespace notify{

struct ILoggerImpl{
	virtual void SetUp(const std::string& /*dir_for_logs*/) = 0;
	virtual void TearDown() = 0;

	virtual ~ILoggerImpl(){}
};

class Logger: public INotification{
public:
	Logger(const std::string& /*dir_for_logs*/, std::string prefix = std::string(""));
	virtual ~Logger();

protected:
	virtual void do_trace(const std::string& msg, const std::string& tag, tGlobalColor c);
	virtual void do_debug(const std::string& msg, const std::string& tag, tGlobalColor c);
	virtual void do_info(const std::string& msg, const std::string& tag, tGlobalColor c);
	virtual void do_warning(const std::string& msg, const std::string& tag, tGlobalColor c);
	virtual void do_error(const std::string& msg, const std::string& tag, tGlobalColor c);
	virtual void do_fatal(const std::string& msg, const std::string& tag, tGlobalColor c);

private:
	boost::scoped_ptr< ILoggerImpl > impl_;
};

} // notify
} // akva
#endif //SUPPORT_LOGGER_H_

