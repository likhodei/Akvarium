#pragma once
#ifndef AKVARIUM_MANAGER_H_
#define AKVARIUM_MANAGER_H_

#include "akva.hpp"
#include "action.hpp"

#include "engine/regedit.hpp"
#include "engine/regular_registr.hpp"
#include "engine/spin_lock.hpp"
#include "engine/protocol.hpp"
#include "support/notify_backend.hpp"

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/once.hpp>

#include <cstdint>
#include <string>
#include <map>

namespace akva{

class Driver;

struct IDriver{
	virtual INotification *const notify() = 0;

    virtual void Attach(Pipeline *const line) = 0; 
	virtual void Detach(Pipeline *const line) = 0;

	virtual uint16_t Reset(const std::string& environment, uint16_t kseed) = 0;
	virtual void Exec(Pipeline* line, tmark_t t, action_sh_t act, mail::ptr_t m) = 0;
	virtual uint16_t Overwatch(tmark_t tick, Manager *const mngr) = 0;

	virtual message_t Message(uint16_t size) = 0;

	virtual uint32_t tiker() const = 0;
	virtual tmark_t HistoryTiker() = 0;

	virtual void Terminate() = 0;
	virtual void Status() = 0; // async call

	virtual bool leader() const = 0;
	virtual bool stopped() const = 0;
	virtual ~IDriver()
	{ }
};

class Manager{
	typedef boost::shared_ptr< Tube > tube_sh_t;
	typedef boost::shared_ptr< Pipe > pipe_sh_t;

public:
	typedef buffer_ptr_t block_ptr_t;

	enum{
		MAGIC_SHIFT_MASK = 0x3f,
		MEM_DEEP = 0xffff,
		MAX_LINK_COUNT = 64,
		REFRESH_INTERVAL = 20,
		BUZY_NODE_LIMIT = 200, // msec
		SERVICE_UPDATE = 100
	};

	Manager(uint16_t kseed, const std::string& environment = "akva");
	~Manager();

	uint32_t tiker() const;
	uint16_t kseed() const;

	INotification *const notify();

	template < class noteT, typename ... T >
	void RegistrateNote(T ... args){
		impl_->RegistrateNote(new noteT(args ...));
	}

	template < typename M, typename ... T >
	std::unique_ptr< M > Build(const T& ... args){
		return std::make_unique< M >(static_cast< M::manager_t *const >(this), args ...);
	}

	template < typename T >
	mail::ptr_t Pack(mail::type_t type, uint16_t spec, const T& msgs){
        mail::ptr_t m = boost::make_shared< mail::Mail >(spec, type);
        if(!msgs.empty()){
            std::list< data_frame_ptr_t > to;
            for(auto& msg : msgs){
                burst_frame_ptr_t c = msg.body_;
                while(c){
                    data_frame_ptr_t d = c->data();
                    if(to.empty() || (to.back()->key() != d->key())){
                        to.push_back(d);
                    }

                    c = c->cont();
                }
            }

            m->Append(to);
        }

		// fill mail header
        auto t = Manager::app()->HistoryTiker();
        m->hdr()->tmark = t.first;
        m->hdr()->number = t.second;
		m->hdr()->magic = 0;
        m->Visit(MAGIC);
		return m;
	}

	std::list< message_t > Unpack(mail::ptr_t m);

	bool connected() const;
	bool stopped() const;

	void Edge(uint16_t key, bool online, bool pipe, const std::string& reason = "");

	void Terminate();
	uint16_t GenMagic() const;

	std::vector< uint16_t > know_all() const;
	bool Guilty(uint16_t magic);
	bool Available(uint16_t magic);

	Pipeline* pipeline();

	int Run(int argc, char* argv[]);
	int Exec(Pipeline* line, action_sh_t act);
	bool leader() const;

    virtual void Service(Pipeline *const line, uint32_t tick);
	virtual void Play(Pipeline *const line, mail::ptr_t m);
	boost::asio::io_service& io();

	void Direct(buffer_ptr_t b);
	void Refresh(uint64_t users);
	const uint16_t MAGIC;

	engine::Regedit *const reg();
	void Broadcast(mail::ptr_t m, bool local = false);

protected:
	void Loop();

	std::set< uint16_t > guilty_;
	std::array< uint16_t, MAX_LINK_COUNT > prefer_;
	uint64_t topology_;

	boost::asio::io_service io_;
	boost::asio::steady_timer timer_;
	boost::asio::io_service::strand strand_;

	uint16_t kseed_;
	pipe_sh_t pipe_;
	tube_sh_t tube_;
	uint32_t tick_;

public:
	static IDriver *const app(); // Get pointer to a default Environment.
	static void release();

	static Driver* impl_;
	static boost::once_flag flag_;
};

} // akva
#endif // AKVARIUM_MANAGER_H_

