#pragma once 
#include <memory>

#include "notification.hpp"

namespace akva{
namespace notify{

struct ILoggerImpl{
	virtual void SetUp(const std::string& dir_for_logs) = 0;
	virtual void TearDown() = 0;

	virtual ~ILoggerImpl(){}
};

class Logger: public INotification{
public:
	Logger(const std::string& dir_for_logs, std::string prefix = std::string(""));
	virtual ~Logger();

protected:
	void do_trace(const std::string& msg, const std::string& tag, Colour c) override;
	void do_debug(const std::string& msg, const std::string& tag, Colour c) override;
	void do_info(const std::string& msg, const std::string& tag, Colour c) override;
	void do_warning(const std::string& msg, const std::string& tag, Colour c) override;
	void do_error(const std::string& msg, const std::string& tag, Colour c) override;
	void do_fatal(const std::string& msg, const std::string& tag, Colour c) override;

private:
	std::unique_ptr< ILoggerImpl > impl_;
};

} // notify
} // akva

