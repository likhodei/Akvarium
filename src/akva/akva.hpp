#pragma once
#ifndef AKVARIUM_H_
#define AKVARIUM_H_

#include "engine/buffer.hpp"
#include "engine/message.hpp"
#include "engine/block_environment.hpp"
#include "engine/spin_lock.hpp"
#include "engine/protocol.hpp"

#include "action.hpp"
#include "declaration.hpp"

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>

#include <cstdint>
#include <list>
#include <string>
#include <set>

namespace akva{

typedef Message message_t;

class Manager;

struct Pipeline{
	Pipeline();

	virtual action_sh_t Service(Manager *const mngr, action_sh_t act, uint32_t tick);
	virtual action_sh_t Play(Manager *const mngr, action_sh_t act, mail::ptr_t m);
	virtual void Complite(Manager *const mngr, mail::ptr_t m);

	virtual std::string tag() const = 0;

	mail::ptr_t Broadcast(Manager *const mngr, mail::type_t type, uint16_t spec, const std::list< message_t >& msgs);
	mail::ptr_t Broadcast(Manager *const mngr, mail::type_t type, uint16_t spec, const std::list< message_t >& msgs, action_sh_t act);
	mail::ptr_t WaitAnnonce(mail::ptr_t m, action_sh_t act);
	buffer_ptr_t MakeData();

	virtual void statistica(std::stringstream& ss);
	virtual uint16_t magic() const = 0;

	Pipeline *const white(uint16_t group);
	Pipeline *const black(uint16_t magic); 

	Pipeline *next_;
	Pipeline *prev_;

	bool filter(mail::ptr_t m);

protected:
	std::map< size_t, action_sh_t > holds_;
	std::set< uint16_t > white_, black_; // filters
	Allocator< mem::EasySpace, engine::MicroSpinLock > local_;
};

class Tube: public Pipeline, public boost::enable_shared_from_this< Tube >{
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
	std::string tag() const;
	uint16_t magic() const;

    buffer_ptr_t Aquare();

protected:
	std::array< engine::AkvaReader*, MAX_LINK_COUNT > physical_;
	std::vector< uint8_t > buf_;

private:
	Manager *const gm_;
	HANDLE hNp_;
	uint16_t id_;
};

class Pipe: public Pipeline, public boost::enable_shared_from_this< Pipe >{
public:
	typedef buffer_ptr_t block_ptr_t;

	Pipe(Manager *const gm, uint16_t key);
	~Pipe();

	void Accept();
	void Reject(const std::string& reason = "");
	void Handle(const boost::system::error_code& ec);

	void AsyncWrite();
	void HandleWrite(const boost::system::error_code& ec, buffer_ptr_t buf);

	action_sh_t Play(Manager *const mngr, action_sh_t act, mail::ptr_t m);

	bool accepted() const;
	bool connected() const;

	inline uint16_t id() const{
		return id_;
	}

	std::string tag() const;
	uint16_t magic() const;

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

	std::string tag() const;
	uint16_t magic() const;

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
#endif // AKVARIUM_H_

