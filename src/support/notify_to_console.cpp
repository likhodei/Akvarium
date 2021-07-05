#include "notify_to_console.hpp"
#include "console.hpp"
#include "bit_operation.hpp"

#include <boost/io/ios_state.hpp>

#include <iostream> 
#include <iomanip>
#include <ostream>

using std::cout;
using std::cerr;

namespace akva{
namespace notify{

using support_ns::Console;

template < int lvlT >
struct MessageWriter : public INotifyWriter{
	template< int T >
	struct Int2Type{
		enum{ value = T };
	};

	MessageWriter(std::ostream &os)
	: os_(os)
	{ }

	void operator()(const std::string& tag, const std::string& message)
    { Commit(tag, message); }

    virtual void Commit(const std::string& tag, const std::string& message){
		boost::io::ios_flags_saver saver(os_);
		Console con;

		WriteTag(con, os_, tag, typename Int2Type< lvlT >());
		os_ << con.fg_default();
		os_ << message << std::endl;
    }

protected:
	void WriteTag(Console& con, std::ostream &os, const std::string& tag, typename Int2Type< LEVEL_TRACE > /*trait*/){
		os << con.fg_default() << "TRACE" << "\t";
		os << con.fg_white() << tag  << "\t";
	}

	void WriteTag(Console& con, std::ostream &os, const std::string& tag, typename Int2Type< LEVEL_DEBUG > /*trait*/){
		os << con.fg_gray() << "DEBUG" << "\t";
		os << con.fg_white() << tag  << "\t";
	}

	void WriteTag(Console& con, std::ostream &os, const std::string& tag, typename Int2Type< LEVEL_INFO > /*trait*/){
		os << con.fg_blue() << "INFO" << "\t";
		os << con.fg_white() << tag  << "\t";
	}

	void WriteTag(Console& con, std::ostream &os, const std::string& tag, typename Int2Type< LEVEL_WARNING > /*trait*/){
		os << con.fg_yellow() << "WARNING" << "\t";
		os << con.fg_white() << tag  << "\t";
	}

	void WriteTag(Console& con, std::ostream &os, const std::string& tag, typename Int2Type< LEVEL_ERROR > /*trait*/){
		os << con.fg_magenta() << "ERROR" << "\t";
		os << con.fg_white() << tag  << "\t";
	}

	void WriteTag(Console& con, std::ostream &os, const std::string& tag, typename Int2Type< LEVEL_FATAL > /*trait*/){
		os << con.fg_red() << "FATAL" << "\t";
		os << con.fg_white() << tag  << "\t";
	}

private:
	std::ostream& os_;
};

struct NoWriter : public INotifyWriter{
    virtual void Commit(const std::string& tag, const std::string& message)
    { }
};

// +++ Notify2Console +++

Notify2Console::Notify2Console(uint32_t lvls){
    for(auto i(0); i < MAX_LVL_COUNT; ++i){
        switch(i){
        case LVL_TRACE:
            if(BitEnabled(lvls, LEVEL_TRACE))
                writers_[i] = new MessageWriter< LEVEL_TRACE >(cout);
            else
                writers_[i] = new NoWriter();

            break;
    	case LVL_DEBUG:
            if(BitEnabled(lvls, LEVEL_DEBUG))
                writers_[i] = new MessageWriter< LEVEL_DEBUG >(cout);
            else
                writers_[i] = new NoWriter();

            break;
    	case LVL_INFO:
            if(BitEnabled(lvls, LEVEL_INFO))
                writers_[i] = new MessageWriter< LEVEL_INFO >(cout);
            else
                writers_[i] = new NoWriter();

            break;
    	case LVL_WARNING:
            if(BitEnabled(lvls, LEVEL_WARNING))
                writers_[i] = new MessageWriter< LEVEL_WARNING >(cerr);
            else
                writers_[i] = new NoWriter();

            break;
    	case LVL_ERROR:
            if(BitEnabled(lvls, LEVEL_ERROR))
                writers_[i] = new MessageWriter< LEVEL_ERROR >(cerr);
            else
                writers_[i] = new NoWriter();

            break;
    	case LVL_FATAL:
            if(BitEnabled(lvls, LEVEL_FATAL))
                writers_[i] = new MessageWriter< LEVEL_FATAL >(cerr);
            else
                writers_[i] = new NoWriter();

            break;
        } // switch
    }
}

Notify2Console::~Notify2Console(){
    for(auto i(0); i < MAX_LVL_COUNT; ++i){
        delete writers_[i]; writers_[i] = NULL;
    }
}

void Notify2Console::do_trace(const std::string& message, const std::string& type, tGlobalColor){
    writers_[LVL_TRACE]->Commit(type, message);
}

void Notify2Console::do_debug(const std::string& message, const std::string& type, tGlobalColor){
    writers_[LVL_DEBUG]->Commit(type, message);
}

void Notify2Console::do_info(const std::string& message, const std::string& type, tGlobalColor){
    writers_[LVL_INFO]->Commit(type, message);
}

void Notify2Console::do_warning(const std::string& message, const std::string& type, tGlobalColor){
    writers_[LVL_WARNING]->Commit(type, message);
}

void Notify2Console::do_error(const std::string& message, const std::string& type, tGlobalColor){
    writers_[LVL_ERROR]->Commit(type, message);
}

void Notify2Console::do_fatal(const std::string& message, const std::string& type, tGlobalColor){
    writers_[LVL_FATAL]->Commit(type, message);
}

} // notify
} // akva

