#pragma once
#ifndef AKVARIUM_ACTION_H_
#define AKVARIUM_ACTION_H_

#include "engine/mail.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <cstdint>
#include <string>

namespace akva{

class Action;
class Manager;
struct Pipeline;

typedef std::pair< uint32_t, uint16_t > tmark_t;
typedef boost::shared_ptr< Action > action_sh_t;

class Action: public boost::enable_shared_from_this< Action >{
public:
	enum priority_t{
		HIGH = 0,
		NORMAL = 0,
		LOW = 0
	};

	Action();
    virtual ~Action();

	virtual action_sh_t operator()(Manager *const mngr, Pipeline* line);
	virtual action_sh_t Play(Manager *const mngr, Pipeline* line, mail::ptr_t m);
	virtual void Complite(Manager *const mngr, Pipeline* line, mail::ptr_t m);
	virtual uint16_t priority() const;

protected:
	bool one_;
};

namespace cmd{

class Test: public Action{
public:
	Test(const std::string& msg);
	~Test();
	action_sh_t operator()(Manager *const mngr, Pipeline* line);
	action_sh_t Play(Manager *const mngr, Pipeline* line, mail::ptr_t m);
	void Complite(Manager *const mngr, Pipeline* line, mail::ptr_t m);

private:
	std::string msg_;
	size_t count_;
};

class Info: public Action{
public:
	action_sh_t operator()(Manager *const mngr, Pipeline* line);
};

class Service: public Action{
public:
	Service(tmark_t tmark);
	action_sh_t Play(Manager *const mngr, Pipeline* line, mail::ptr_t m);
	action_sh_t operator()(Manager *const mngr, Pipeline* line);
	void Complite(Manager *const mngr, Pipeline* line, mail::ptr_t m);
	uint16_t priority() const;

protected:
	tmark_t tmark_;
};

class Transfer: public Action{
public:
	Transfer(bool local);
	action_sh_t Play(Manager *const mngr, Pipeline* line, mail::ptr_t m);
	action_sh_t operator()(Manager *const mngr, Pipeline* line);
	uint16_t priority() const;

protected:
	bool local_;
};

class Overwatch: public Action{
public:
	Overwatch(tmark_t tmark);
	action_sh_t operator()(Manager *const mngr, Pipeline* line);

protected:
	tmark_t tmark_;
};

} // cmd
} // akva
#endif // AKVARIUM_ACTION_H_

