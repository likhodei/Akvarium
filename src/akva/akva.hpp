#pragma once
#include <cstdint>
#include <list>
#include <string>
#include <map>
#include <set>
#include <memory>

#include <boost/asio.hpp>

#include "engine/buffer.hpp"
#include "engine/view.hpp"
#include "engine/memory.hpp"
#include "engine/spin_lock.hpp"
#include "engine/protocol.hpp"

#include "context.hpp"
#include "declaration.hpp"

namespace akva{

class Manager;

struct Pipeline{
	Pipeline();

	virtual ctx::ptr_t Service(Manager *const mngr, ctx::ptr_t c, uint32_t tick);
	virtual ctx::ptr_t Play(Manager *const mngr, ctx::ptr_t c, graph::ptr_t m);
	virtual void Complite(Manager *const mngr, graph::ptr_t m);

	virtual std::string tag() const = 0;

	graph::ptr_t Broadcast(Manager *const mngr, graph::type_t type, uint16_t spec, const std::list< Message >& msgs);
	graph::ptr_t Broadcast(Manager *const mngr, graph::type_t type, uint16_t spec, const std::list< Message >& msgs, ctx::ptr_t c);
	graph::ptr_t WaitAnnonce(graph::ptr_t m, ctx::ptr_t c);
	buffer_ptr_t MakeData();

	virtual void statistica(std::stringstream& ss);
	virtual uint16_t magic() const = 0;

	Pipeline *const white(uint16_t group);
	Pipeline *const black(uint16_t magic); 

	Pipeline *next_;
	Pipeline *prev_;

	bool filter(graph::ptr_t m);

protected:
	std::map< size_t, ctx::ptr_t > holds_;
	std::set< uint16_t > white_, black_; // filters
	Allocator< mem::EasySpace, engine::MicroSpinLock > local_;
};

class Tube: public Pipeline, public std::enable_shared_from_this< Tube >{
public:
	enum{
		MAX_LINK_COUNT = 64,
		MAX_FRAME_SIZE = 16 * 1024
	};

	Tube(Manager *const mngr);
	~Tube();

	inline uint16_t id() const{
		return id_;
	}

	bool Run(uint16_t id);

	void AsyncRead();
	void HandleRead(const boost::system::error_code& ec, size_t transferred, buffer_ptr_t buf);

	bool connected() const;
	std::string tag() const override;
	uint16_t magic() const override;

    buffer_ptr_t Aquare();

protected:
	std::array< engine::AkvaReader*, MAX_LINK_COUNT > physical_;
	std::vector< uint8_t > buf_;

private:
	Manager *const gm_;
	HANDLE hNp_;
	uint16_t id_;
};

class Pipe: public Pipeline, public std::enable_shared_from_this< Pipe >{
public:
	typedef buffer_ptr_t block_ptr_t;

	Pipe(Manager *const gm, uint16_t key);
	~Pipe();

	void Accept();
	void Reject(const std::string& reason = "");
	void Handle(const boost::system::error_code& ec);

	void AsyncWrite();
	void HandleWrite(const boost::system::error_code& ec, buffer_ptr_t buf);

	ctx::ptr_t Play(Manager *const mngr, ctx::ptr_t c, graph::ptr_t m) override;

	bool accepted() const;
	bool connected() const;

	inline uint16_t id() const{
		return id_;
	}

	std::string tag() const override;
	uint16_t magic() const override;

    buffer_ptr_t Aquare();
	void Direct(buffer_ptr_t b);

protected:
	std::deque< buffer_ptr_t > rbuf_;
	engine::AkvaWriter< Pipe > writer_;
	engine::MicroSpinLock guard_;

private:
	Manager *const gm_;
	HANDLE hNp_;
	uint32_t ppid_;
	uint16_t id_;

	bool corresponding_, accepted_;
};

template < class handlerT, typename managerT = Manager >
class Akva: public Pipeline{
public:
	typedef typename managerT manager_t;

	Akva(manager_t *const gm, uint32_t delay);
	~Akva();

	std::string tag() const override;
	uint16_t magic() const override;

	typename handlerT* from_this(){
		return (handlerT*)this;
	}

protected:
	const uint16_t MAGIC;
	manager_t *const gm_;
	uint32_t delay_;
};

// +++ Akva +++

template < class handlerT, typename managerT >
Akva< handlerT, managerT >::Akva(manager_t *const gm, uint32_t delay)
: MAGIC(gm->GenMagic()), gm_(gm), delay_(delay){
	Manager::app()->Attach(this);
}

template < class handlerT, typename managerT >
Akva< handlerT, managerT >::~Akva(){
	Manager::app()->Detach(this);
}

template < class handlerT, typename managerT >
std::string Akva< handlerT, managerT >::tag() const{
	return "G";
}

template < class handlerT, typename managerT >
uint16_t Akva< handlerT, managerT >::magic() const{
	return MAGIC;
}

} // akva

