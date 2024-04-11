#include "terminal.hpp"

#include <iostream> 
#include <iomanip>
#include <ostream>

#include <boost/io/ios_state.hpp>

#include "console.hpp"
#include "bit_utils.hpp"

namespace akva{
namespace notify{

using ms_notify::Console;

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

    void Commit(const std::string& tag, const std::string& message) override{
		boost::io::ios_flags_saver saver(os_);
		Console con;

		WriteTag(con, os_, tag, Int2Type< lvlT >()); // typename 
		os_ << con.fg_default();
		os_ << message << std::endl;
    }

protected:
	void WriteTag(Console& con, std::ostream &os, const std::string& tag, typename Int2Type< trace >){
		os << con.fg_default() << "TRACE" << "\t";
		os << con.fg_white() << tag  << "\t";
	}

	void WriteTag(Console& con, std::ostream &os, const std::string& tag, typename Int2Type< debug >){
		os << con.fg_gray() << "DEBUG" << "\t";
		os << con.fg_white() << tag  << "\t";
	}

	void WriteTag(Console& con, std::ostream &os, const std::string& tag, typename Int2Type< info >){
		os << con.fg_blue() << "INFO" << "\t";
		os << con.fg_white() << tag  << "\t";
	}

	void WriteTag(Console& con, std::ostream &os, const std::string& tag, typename Int2Type< warning >){
		os << con.fg_yellow() << "WARNING" << "\t";
		os << con.fg_white() << tag  << "\t";
	}

	void WriteTag(Console& con, std::ostream &os, const std::string& tag, typename Int2Type< error >){
		os << con.fg_magenta() << "ERROR" << "\t";
		os << con.fg_white() << tag  << "\t";
	}

	void WriteTag(Console& con, std::ostream &os, const std::string& tag, typename Int2Type< fatal >){
		os << con.fg_red() << "FATAL" << "\t";
		os << con.fg_white() << tag  << "\t";
	}

private:
	std::ostream& os_;
};

struct NoWriter : public INotifyWriter{
    void Commit(const std::string& tag, const std::string& message) override
    { }
};

// +++ Notify2Console +++

Notify2Console::Notify2Console(uint32_t lvls){
    for(auto i(0); i < MAX_LVL_COUNT; ++i){
        writers_[i] = nullptr;

        switch(i){
        case LVL_TRACE:
            if(BitEnabled(lvls, trace))
                writers_[i] = new MessageWriter< trace >(std::cout);

            break;
    	case LVL_DEBUG:
            if(BitEnabled(lvls, debug))
                writers_[i] = new MessageWriter< debug >(std::cout);

            break;
    	case LVL_INFO:
            if(BitEnabled(lvls, info))
                writers_[i] = new MessageWriter< info >(std::cout);

            break;
    	case LVL_WARNING:
            if(BitEnabled(lvls, warning))
                writers_[i] = new MessageWriter< warning >(std::cerr);

            break;
    	case LVL_ERROR:
            if(BitEnabled(lvls, error))
                writers_[i] = new MessageWriter< error >(std::cerr);

            break;
    	case LVL_FATAL:
            if(BitEnabled(lvls, fatal))
                writers_[i] = new MessageWriter< fatal >(std::cerr);

            break;
        } // switch

        if(writers_[i] == nullptr)
            writers_[i] = new NoWriter();
    }
}

Notify2Console::~Notify2Console(){
    for(auto i(0); i < MAX_LVL_COUNT; ++i){
        delete writers_[i]; writers_[i] = nullptr;
    }
}

void Notify2Console::do_trace(const std::string& message, const std::string& type, Colour){
    writers_[LVL_TRACE]->Commit(type, message);
}

void Notify2Console::do_debug(const std::string& message, const std::string& type, Colour){
    writers_[LVL_DEBUG]->Commit(type, message);
}

void Notify2Console::do_info(const std::string& message, const std::string& type, Colour){
    writers_[LVL_INFO]->Commit(type, message);
}

void Notify2Console::do_warning(const std::string& message, const std::string& type, Colour){
    writers_[LVL_WARNING]->Commit(type, message);
}

void Notify2Console::do_error(const std::string& message, const std::string& type, Colour){
    writers_[LVL_ERROR]->Commit(type, message);
}

void Notify2Console::do_fatal(const std::string& message, const std::string& type, Colour){
    writers_[LVL_FATAL]->Commit(type, message);
}

} // notify
} // akva

