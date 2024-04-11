#include <random>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <boost/program_options.hpp>

#include "akva/driver.hpp"
#include "akva/manager.hpp"
#include "akva/akva.hpp"
#include "engine/terminal.hpp"

namespace po = boost::program_options;

using akva::Manager;
//using akva::ctx;
using akva::Message;


BOOL WINAPI ConsoleHandler(DWORD cEvent){
	BOOL out = TRUE;
	switch(cEvent){
	case CTRL_C_EVENT:
		Manager::app()->Terminate();
		break;
	case CTRL_BREAK_EVENT:
		Manager::app()->Status();
		break;
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		break;
	} // switch

	return out;
}

class Runner: public akva::Akva< Runner >{
	enum{
		DATA_MAGIC = 1024 * 8,
	};

public:
	Runner(Manager *const mngr, const std::string& tag);

	akva::ctx::ptr_t Play(Manager *const mngr, akva::ctx::ptr_t c, graph::ptr_t g);
    akva::ctx::ptr_t Service(Manager *const mngr, akva::ctx::ptr_t c, uint32_t tick);
	std::string tag() const;

private:
	std::mt19937 rng_;
	std::vector< uint8_t > payload_;

	uint32_t nums_;
	std::pair< size_t, size_t > transfered_;
	std::string tag_;
	int calls_;
	uint32_t t0_;
};

int main(int argc, char* argv[]){
	int RetVal(EXIT_FAILURE);

    if(SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE) == FALSE)
		return RetVal;

	try{
		// 0) load options
		po::options_description desc("Generic options");
		desc.add_options()
			("seed,k", po::value< uint32_t >(), "user seed");

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		uint16_t kseed = 1;
		if(vm.count("seed")){
			kseed = vm["seed"].as< uint32_t >();
		}

		std::cout << "K-SEED = " << kseed << std::endl;
		Manager tank(kseed);

		akva::RegistrateNote< akva::notify::Notify2Console >(akva::notify::DANGER_LEVELS);

		auto A = tank.Build< Runner >("A");

		//auto B = tank.Build< Runner >("B");
		//auto C = tank.Build< Runner >("C");

		RetVal = tank.Run(argc, argv);
	}
	catch(const std::exception& ex){
		Manager::app()->notify()->Error(ex.what());
	}
	catch(...){
		Manager::app()->notify()->Fatal("...");
	}

	std::cout << "Done processing ["<< std::boolalpha << RetVal << "]:)" << std::endl;
    Manager::release();
	return RetVal;
}

// +++ Runner +++

Runner::Runner(Manager *const mngr, const std::string& tag)
: Akva(mngr, 100), transfered_(0, 0), rng_(mngr->kseed()), tag_(tag), calls_(4), t0_(mngr->tiker()), nums_(0){
    { // for test
        payload_.resize(DATA_MAGIC * 2);
        for(auto i(0); i < payload_.size(); ++i)
            payload_[i] = i % 256;

        std::random_device rd;
        std::mt19937 g(rd());

        //std::shuffle(payload_.begin(), payload_.end(), g);
    }
}

akva::ctx::ptr_t Runner::Play(Manager *const mngr, akva::ctx::ptr_t c, graph::ptr_t m){
	auto gr = m->hdr()->group;
	if(gr != 400)
		return akva::ctx::ptr_t();

	auto bs = m->bytes();
	auto hs = m->hash_value();
	size_t sz = 0;

	std::list< Message > msgs = mngr->Unpack(m);
	for(auto& msg : msgs){
		sz += msg.body_->len();
		akva::MessageView v = msg.view();
		auto p = v.ptr();
		auto l = v.length();

		transfered_.first += l;
	}

//	transfered_.second += msgs.size();

	if(nums_ < m->hdr()->tmark)
        nums_ = m->hdr()->tmark;
	else{
		std::cout << "!!!! RE ORDER [" << nums_ << " > " << m->hdr()->tmark << "] !!!!" << std::endl;
	}

	std::stringstream ss;

	ss << tag();
	ss << " | " << m->hdr()->magic;
	ss << " t: " << m->hdr()->tmark;
	ss << " n: " << m->hdr()->number;
	ss << " bytes: " << bs;
	ss << " sz: " << sz;
	ss << " hash : " << hs;
	ss << " -> " << __FUNCTION__;
	std::cout << ss.str() << std::endl;

	return akva::ctx::ptr_t();
}

std::string Runner::tag() const{
	return tag_;
}

akva::ctx::ptr_t Runner::Service(Manager *const mngr, akva::ctx::ptr_t c, uint32_t tick){
	uint32_t now = mngr->tiker();
	if((now - t0_) > 1000){
//        std::cout << tag() << " BYTES:" << std::setw(10) << transfered_.first << " COUNTS:" << std::setw(3) << transfered_.second << std::endl;

        transfered_ = std::make_pair(0, 0);
		t0_ = now;
	}

	return akva::ctx::ptr_t();
}
