#include "manager.hpp"
#include "driver.hpp"

#include "support/logger.hpp"

#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/chrono_io.hpp>
#include <boost/bind.hpp>
#include <boost/assert.hpp>

#include <locale>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace akva;
using engine::Regedit;

// +++ Manager +++

Driver* Manager::impl_ = nullptr;
boost::once_flag Manager::flag_ = BOOST_ONCE_INIT;

void Driver::Init(){
    Manager::impl_ = new Driver;
    BOOST_ASSERT(Manager::impl_);
}

IDriver *const Manager::app(){
	boost::call_once(flag_, Driver::Init);
	return impl_;
}

void Manager::release(){
	if(impl_){
		delete impl_; impl_ = nullptr;
		flag_ = BOOST_ONCE_INIT;
	}
}

Manager::Manager(uint16_t kseed, const std::string& environment)
: timer_(io_), strand_(io_), kseed_(kseed), MAGIC(app()->Reset(environment, kseed)), topology_(0){
    RegistrateNote< notify::Logger >("logs", std::to_string(kseed_) + "-");
	reg()->AddNode(MAGIC);

	tick_ = impl_->tiker();

	tube_ = boost::make_shared< Tube >(this);
	pipe_ = boost::make_shared< Pipe >(this, MAGIC);

	timer_.expires_at(boost::asio::chrono::steady_clock::now());
	timer_.async_wait(boost::bind(&Manager::Loop, this));
}

Manager::~Manager(){
	pipe_.reset();
	tube_.reset();

	reg()->DelNode(MAGIC);
}

uint16_t Manager::kseed() const{
	return kseed_;
}

Regedit *const Manager::reg(){
	return impl_->reg();
}

INotification *const Manager::notify(){
	return impl_->notify();
}

uint16_t Manager::GenMagic() const{
	return impl_->reg()->GenMagic(kseed_);
}

int Manager::Run(int argc, char* argv[]){
	boost::asio::io_service::work work(io_);
	boost::thread th(boost::bind(&boost::asio::io_service::run, boost::ref(io_)));
	impl_->Run(this);
	th.join();
	return EXIT_SUCCESS;
}

int Manager::Exec(Pipeline* line, action_sh_t act){
	strand_.post(boost::bind(&Action::operator(), act, this, line));
	return EXIT_SUCCESS;
}

boost::asio::io_service& Manager::io(){
	return io_;
}

Pipeline* Manager::pipeline(){
	return impl_->pipeline();
}

bool Manager::leader() const{
	return impl_->leader();
}

void Manager::Edge(uint16_t key, bool online, bool pipe, const std::string& reason){
    auto i = Regedit::IxByMagic(key);
	if(!pipe){
		if(online)
			reg()->AddEdge(MAGIC, i);
		else
			reg()->DelEdge(MAGIC, i);
	}

	std::stringstream ss;
	ss << (online ? " + " : " - ");
	ss << (pipe ? "pipe " : "tube ");
	ss << std::hex << key << std::dec << " ~ " << std::setw(2) << i << " |-> ec (" << reason << ") ";
    ss << Regedit::IxByMagic(pipe_->id()) << " <-[" << kseed_ << "]-> " << Regedit::IxByMagic(tube_->id());

	notify()->Warning(ss.str(), "edge");
}

uint32_t Manager::tiker() const{
	return impl_->tiker();
}

bool Manager::Available(uint16_t magic){
	int i = Regedit::IxByMagic(magic);
	if((prefer_[i] == magic) && !Guilty(magic))
		return true;
	else
        return false;
}

bool Manager::Guilty(uint16_t magic){
	if(magic == MAGIC) // without me, please.
		return false;
	else
        return guilty_.find(magic) != guilty_.end();
}

std::vector< uint16_t > Manager::know_all() const{
	std::vector< uint16_t > magics;
	for(auto i(0); i < MAX_LINK_COUNT; ++i){
		if(prefer_[i] != 0){
			magics.push_back(prefer_[i]); // i pos
		}
	}

	return magics;
}

void Manager::Loop(){
	if(stopped()){
		std::cout << "STOPPED" << std::endl;
		timer_.cancel();
		io_.stop();
		return;
	}

	auto t = impl_->HistoryTiker();
	if(!connected() || reg()->Changed()){
        auto lg = reg()->Log();

		guilty_.clear();
		topology_ = lg.second;
		memset(prefer_.data(), 0, prefer_.size() * sizeof(uint16_t));
        for(auto r : reg()->world()){
			if(r->ok()){
				if(std::abs< ptrdiff_t >(t.first - r->tmark_) > BUZY_NODE_LIMIT)
					guilty_.insert(r->magic_);
				else{
					prefer_[Regedit::IxByMagic(r->magic_)] = r->magic_;
				}
			}
        }

        int pos = Regedit::IxByMagic(MAGIC);
        prefer_[pos] = MAGIC;

        uint16_t A(pos), B(pos);
        // <-- wave
		for(auto i(1); i <= prefer_.size(); ++i){
			A = (prefer_.size() + pos - i) % prefer_.size();
			if(prefer_[A])
				break;
		}

        // wave -->
		for(auto i(1); i <= prefer_.size(); ++i){
            B = (pos + i) % prefer_.size();
			if(prefer_[B])
				break;
		}

		bool online(false);
        if(pipe_->connected()){ 
            if(pipe_->id() != prefer_[A]){
                pipe_->Reject("reconnection");
                pipe_->Accept();
            }

            online = true;
        }
        else if(!pipe_->accepted()){
            pipe_->Accept();
            online = true;
        }

		if(!tube_->connected()){
            bool guilty = Guilty(prefer_[B]);
            tube_->Run(prefer_[B]);
		}
	}

	if(leader()){
		if((t.first - tick_) > SERVICE_UPDATE){ 
			akva::tmark_t x = std::make_pair(tick_, 0);
			impl_->Exec(x, boost::make_shared< cmd::Service >(x));
			tick_ += SERVICE_UPDATE;
		}
	}
	else
		tick_ = t.first;

	impl_->Exec(t, boost::make_shared< cmd::Overwatch >(t));
	timer_.expires_at(timer_.expiry() + boost::asio::chrono::milliseconds(REFRESH_INTERVAL));
	timer_.async_wait(boost::bind(&Manager::Loop, this));
}

bool Manager::connected() const{
	return pipe_->connected() && tube_->connected();
}

void Manager::Broadcast(mail::ptr_t m, bool local){
	auto t = impl_->HistoryTiker();
	impl_->Exec(nullptr, t, boost::make_shared< cmd::Transfer >(local), m);
}

std::list< message_t > Manager::Unpack(mail::ptr_t m){
	std::list< message_t > msgs;
	impl_->Unpack(m, msgs);
	return std::move(msgs);
}

void Manager::Direct(buffer_ptr_t b){
	pipe_->Direct(b);
}

bool Manager::stopped() const{
	return impl_->stopped();
}

void Manager::Terminate(){
	impl_->Terminate();
}

void Manager::Refresh(uint64_t users){
	impl_->Refresh(users);
}

void Manager::Service(Pipeline *const line, uint32_t tick)
{ }

void Manager::Play(Pipeline *const line, mail::ptr_t m)
{ }

