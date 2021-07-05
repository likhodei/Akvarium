#pragma once
#ifndef AKVARIUM_DRIVER_H_
#define AKVARIUM_DRIVER_H_

#include "akva.hpp"
#include "action.hpp"
#include "manager.hpp"

#include "engine/spin_lock.hpp"
#include "engine/regedit.hpp"
#include "support/notify_backend.hpp"

#include <boost/chrono/time_point.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/heap/priority_queue.hpp>

#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/atomic.hpp>

#include <cstdint>
#include <vector>
#include <deque>
#include <string>

namespace akva{

class Driver;

#define USE_PRIORITY

struct MeasureX{
	MeasureX* next_;
	MeasureX* prev_;
	Measure impl_;
};

struct Task;

struct Worker{
	Worker *next_;
	Worker *prev_;

	Pipeline *line_;
	uint64_t marker_;
	Task* job_;
	//uint16_t key_;

	uint64_t On(Manager *const mngr, tmark_t t);
	bool empty();

	void Plain(Task* job);
	void Leave(Task* job);
};

struct Task{
	Task *next_;
	Task *prev_;

	Worker *wk_;
	uint32_t looks_;
	uint64_t marker_;

	tmark_t t_;
	action_sh_t act_;
	mail::ptr_t m_;

	void Put(Worker *);
	void Del(Worker *);
	Worker* Pop();
	size_t count();
	bool empty();

	bool Complite(size_t workers); // fix: dynamic workers
};

class Driver: public IDriver, protected Pipeline{
public:
	Driver();
    ~Driver();

	enum{
		MAX_STACK_COUNT = 64,
        MAX_MEASURES = 100,
        MESH_INTERVAL = 100,
		REFRESH_INTERVAL = 50 // msec
	};

	typedef Allocator< mem::RegularRegistr, engine::MicroSpinLock > remote_allocator_t;

	uint16_t Reset(const std::string& environment, uint16_t kseed);
	void Exec(Pipeline* line, tmark_t t, action_sh_t act, mail::ptr_t m);
	void Exec(tmark_t t, action_sh_t act);

	engine::Regedit *const reg();
	INotification *const notify();

	bool stopped() const;

    void Attach(Pipeline *const line); 
	void Detach(Pipeline *const line);

	void Terminate();
	void Status();

	uint32_t tiker() const;
	uint64_t tiker64() const;
	std::pair< uint32_t, uint16_t > HistoryTiker();

	uint16_t Overwatch(tmark_t tick, Manager *const mngr);
	void RegistrateNote(INotification *const note);

	static void Init(); // todo: env params

	bool leader() const;
	Pipeline* pipeline();

	message_t Message(uint16_t size);

	void Unpack(mail::ptr_t m, std::list< message_t >& msgs);

	void Refresh(uint64_t users);
	void Run(Manager *const mngr);

protected:
	std::string tag() const;
    action_sh_t Service(Manager *const mngr, action_sh_t act, uint32_t tick);
	action_sh_t Play(Manager *const mngr, action_sh_t act, mail::ptr_t m);
	uint16_t magic() const;
	void statistica(std::stringstream& ss);
	bool no_tasks();

protected:
	volatile bool done_;
	uint16_t leader_; // his magic 
	uint16_t magic_;

	boost::scoped_ptr< engine::Regedit > reg_;
    boost::posix_time::ptime tbase_; // timestemp (1970.01.01)

	std::vector< INotification* > notes_;
	notify::NotifyBackend notify_;

	boost::mutex mut_;
	boost::condition_variable cond_;
	std::vector< Worker* > workers_;
	Task jobs_;

	boost::random::mt19937 rng_;
	int task_seq_;

	volatile uint64_t tmark_;
	boost::atomic_int tseq_;

	uint64_t seq4to_;
	remote_allocator_t* remote_;
	FrameStack stack_[MAX_STACK_COUNT];
	MeasureX *measure_;
};

} // namespace akva
#endif // AKVARIUM_DRIVER_H_

