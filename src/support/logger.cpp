#include "logger.hpp"

#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/utility/record_ordering.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>

#include <string>
#include <stdexcept>

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;

using namespace boost::log;

enum severity_level{
    trace,
    debug,
    info,
	warning,
    error,
    fatal
};

inline std::ostream& operator<< (std::ostream& strm, severity_level level){
    switch (level)
    {
    case trace:
        strm << "trace";
        break;
    case debug:
        strm << "debug";
        break;
    case info:
        strm << "info";
        break;
    case warning:
        strm << "warning";
        break;
    case error:
        strm << "error";
        break;
    case fatal:
        strm << "fatal";
        break;
    default:
        strm << static_cast< int >(level);
        break;
    }

    return strm;
}

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(test_lg, src::severity_logger_mt< severity_level >)

namespace akva{
namespace notify{

// +++ LoggerImpl +++

class LoggerImpl: public ILoggerImpl{
    typedef sinks::synchronous_sink< sinks::text_file_backend > file_sink_t;

public:
    LoggerImpl(const std::string& prefix)
        : fmask_(prefix){
        fmask_.append("%Y%m%d_%H%M%S_%5N.log");
    }

    virtual void SetUp(const std::string& dir_for_logs){
        sink_.reset(new file_sink_t(
            //	        keywords::order = logging::make_attr_ordering("Line #", std::less< uint32_t >()), // apply record ordering
            keywords::file_name = fmask_, /*"%Y%m%d_%H%M%S_%5N.log",*/ // file name pattern
            keywords::rotation_size = 1024 * 1024)); // rotation size, in characters // 16384

        // Set up where the rotated files will be stored
        sink_->locked_backend()->set_file_collector(sinks::file::make_collector(
            keywords::target = dir_for_logs, // where to store rotated files
            keywords::max_size = 64 * 1024 * 1024, // maximum total size of the stored files, in bytes
            keywords::min_free_space = 2000 * 1024 * 1024 // minimum free space on the drive, in bytes
        ));

        // Upon restart, scan the target directory for files matching the file_name pattern
        sink_->locked_backend()->scan_for_files();

        sink_->set_formatter
        (
            expr::format("%1%\t[%2%]\t[%3%]\t{%4%}\t%5%")
            % expr::attr< uint32_t >("Line #")
            % expr::attr< boost::posix_time::ptime >("TimeStamp")
            % expr::attr< boost::thread::id >("ThreadID")
            % expr::attr< severity_level >("Severity")
            % expr::smessage
        );

        // Add it to the core
        logging::core::get()->add_sink(sink_);

        // Add some attributes too
        logging::core::get()->add_global_attribute("TimeStamp", attrs::local_clock());
        logging::core::get()->add_global_attribute("RecordID", attrs::counter< unsigned int >());
    }

    virtual void TearDown(){
        // Flush all buffered records
        sink_->flush();
        //sink_->feed_records(boost::posix_time::seconds(0));
    }

private:
    boost::shared_ptr< file_sink_t > sink_;
    std::string fmask_;
};

// +++ Logger +++

Logger::Logger(const std::string& dir_for_logs, std::string prefix)
    : impl_(new LoggerImpl(prefix)){
    try{
        impl_->SetUp(dir_for_logs);
    }
    catch(const std::exception& ex){
        std::cerr << "ERROR : " << ex.what() << std::endl;
    }
}

Logger::~Logger(){
    impl_->TearDown();
}

void Logger::do_trace(const std::string& msg, const std::string& type, tGlobalColor){
    BOOST_LOG_SCOPED_THREAD_TAG("ThreadID", boost::this_thread::get_id());
    BOOST_LOG_SEV(test_lg::get(), trace) << type << "\t" << msg;
}

void Logger::do_debug(const std::string& msg, const std::string& type, tGlobalColor){
    BOOST_LOG_SCOPED_THREAD_TAG("ThreadID", boost::this_thread::get_id());
    BOOST_LOG_SEV(test_lg::get(), debug) << type << "\t" << msg;
}

void Logger::do_info(const std::string& msg, const std::string& type, tGlobalColor){
    BOOST_LOG_SCOPED_THREAD_TAG("ThreadID", boost::this_thread::get_id());
    BOOST_LOG_SEV(test_lg::get(), info) << type << "\t" << msg;
}

void Logger::do_warning(const std::string& msg, const std::string& type, tGlobalColor){
    BOOST_LOG_SCOPED_THREAD_TAG("ThreadID", boost::this_thread::get_id());
    BOOST_LOG_SEV(test_lg::get(), warning) << type << "\t" << msg;
}

void Logger::do_error(const std::string& msg, const std::string& type, tGlobalColor){
    BOOST_LOG_SCOPED_THREAD_TAG("ThreadID", boost::this_thread::get_id());
    BOOST_LOG_SEV(test_lg::get(), error) << type << "\t" << msg;
}

void Logger::do_fatal(const std::string& msg, const std::string& type, tGlobalColor){
    BOOST_LOG_SCOPED_THREAD_TAG("ThreadID", boost::this_thread::get_id());
    BOOST_LOG_SEV(test_lg::get(), fatal) << type << "\t" << msg;
}

} // notify
} // akva
