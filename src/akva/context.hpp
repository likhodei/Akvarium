#pragma once
#include <cstdint>
#include <string>
#include <memory>

#include "engine/graph.hpp"

namespace akva{

class Context;
class Manager;
struct Pipeline;

typedef std::pair< uint32_t, uint16_t > tmark_t;

namespace ctx{

typedef std::shared_ptr< Context > ptr_t;

} // ctx

class Context: public std::enable_shared_from_this< Context >{
public:
	enum priority_t{
		HIGH = 0,
		NORMAL = 0,
		LOW = 0
	};

	Context();
    virtual ~Context();

	virtual ctx::ptr_t operator()(Manager *const mngr, Pipeline* line);
	virtual ctx::ptr_t Play(Manager *const mngr, Pipeline* line, graph::ptr_t m);
	virtual void Complite(Manager *const mngr, Pipeline* line, graph::ptr_t m);
	virtual uint16_t priority() const;

protected:
	bool one_;
};

namespace cmd{

class Test: public Context{
public:
	Test(const std::string& msg);
	~Test();

	ctx::ptr_t operator()(Manager *const mngr, Pipeline* line) override;
	ctx::ptr_t Play(Manager *const mngr, Pipeline* line, graph::ptr_t m) override;
	void Complite(Manager *const mngr, Pipeline* line, graph::ptr_t m) override;

private:
	std::string msg_;
	size_t count_;
};

class Info: public Context{
public:
	ctx::ptr_t operator()(Manager *const mngr, Pipeline* line);
};

class Service: public Context{
public:
	Service(tmark_t tmark);

	ctx::ptr_t Play(Manager *const mngr, Pipeline* line, graph::ptr_t m) override;
	ctx::ptr_t operator()(Manager *const mngr, Pipeline* line) override;
	void Complite(Manager *const mngr, Pipeline* line, graph::ptr_t m) override;
	uint16_t priority() const override;

protected:
	tmark_t tmark_;
};

class Transfer: public Context{
public:
	Transfer(bool local);

	ctx::ptr_t Play(Manager *const mngr, Pipeline* line, graph::ptr_t m) override;
	ctx::ptr_t operator()(Manager *const mngr, Pipeline* line) override;
	uint16_t priority() const override;

protected:
	bool local_;
};

class Overwatch: public Context{
public:
	Overwatch(tmark_t tmark);

	ctx::ptr_t operator()(Manager *const mngr, Pipeline* line) override;

protected:
	tmark_t tmark_;
};

} // cmd
} // akva
