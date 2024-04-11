#pragma once
#include <memory>
#include <cstdint>
#include <string>
#include <thread>
#include <map>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/thread/once.hpp>

#include "engine/regedit.hpp"
#include "engine/regular_registr.hpp"
#include "engine/spin_lock.hpp"
#include "engine/protocol.hpp"
#include "engine/notification.hpp"

#include "akva.hpp"
#include "context.hpp"

namespace akva{

class Driver;

struct IDriver{
	virtual INotification *const notify() = 0;
	virtual void Add(INotification *const note) = 0;

    virtual void Attach(Pipeline *const line) = 0; 
	virtual void Detach(Pipeline *const line) = 0;

	virtual uint16_t Reset(const std::string& environment, uint16_t kseed) = 0;
	virtual void Exec(Pipeline* line, tmark_t t, ctx::ptr_t act, graph::ptr_t m) = 0;
	virtual uint16_t Overwatch(tmark_t tick, Manager *const mngr) = 0;

	virtual Message MakeMessage(uint16_t size) = 0;

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

	template < typename M, typename ... T >
	std::unique_ptr< M > Build(const T& ... args){
		return std::make_unique< M >(static_cast< M::manager_t *const >(this), args ...);
	}

	template < typename T >
	graph::ptr_t Pack(graph::type_t type, uint16_t spec, const T& msgs){
        graph::ptr_t m = std::make_shared< graph::Graph >(spec, type);
        if(!msgs.empty()){
            std::list< frame_ptr_t > to;
            for(auto& msg : msgs){
                burst_ptr_t c = msg.body_;
                while(c){
                    frame_ptr_t d = c->data();
                    if(to.empty() || (to.back()->key() != d->key())){
                        to.push_back(d);
                    }

                    c = c->cont();
                }
            }

            m->Append(to);
        }

		// fill graph header
        auto t = Manager::app()->HistoryTiker();
        m->hdr()->tmark = t.first;
        m->hdr()->number = t.second;
		m->hdr()->magic = 0;
        m->Visit(MAGIC);
		return m;
	}

	std::list< Message > Unpack(graph::ptr_t m);

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
	int Exec(Pipeline* line, ctx::ptr_t c);
	bool leader() const;

    virtual void Service(Pipeline *const line, uint32_t tick);
	virtual void Play(Pipeline *const line, graph::ptr_t m);

	boost::asio::io_service& io();

	void Direct(buffer_ptr_t b);
	void Refresh(uint64_t users);
	const uint16_t MAGIC;

	engine::Regedit *const reg();
	void Broadcast(graph::ptr_t m, bool local = false);

protected:
	void Loop();

	std::set< uint16_t > guilty_;
	std::array< uint16_t, MAX_LINK_COUNT > prefer_;
	uint64_t topology_;

	boost::asio::io_service io_;
	boost::asio::steady_timer timer_;
	boost::asio::io_service::strand strand_;

	uint16_t kseed_;
	std::shared_ptr< Tube > tube_;
	std::shared_ptr< Pipe > pipe_;

	uint32_t tick_;

public:
	static IDriver *const app(); // Get pointer to a default Environment.
	static void release();

	static Driver* impl_;
	static boost::once_flag flag_;
};

template < class noteT, typename ... T >
void RegistrateNote(T ... args){
	Manager::app()->Add(new noteT(args ...));
}

} // akva

