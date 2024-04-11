#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <boost/chrono/time_point.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/heap/priority_queue.hpp>

#include "akva.hpp"
#include "context.hpp"
#include "manager.hpp"

#include "engine/spin_lock.hpp"
#include "engine/regedit.hpp"
#include "engine/notification.hpp"

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
	ctx::ptr_t act_; // c_
	graph::ptr_t m_; // g_

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

	uint16_t Reset(const std::string& environment, uint16_t kseed) override;
	void Exec(Pipeline* line, tmark_t t, ctx::ptr_t c, graph::ptr_t g) override;
	void Exec(tmark_t t, ctx::ptr_t act);

	engine::Regedit *const reg();
	INotification *const notify() override;

	bool stopped() const;

    void Attach(Pipeline *const line) override; 
	void Detach(Pipeline *const line) override;

	void Terminate() override;
	void Status() override;

	uint32_t tiker() const override;
	uint64_t tiker64() const;
	std::pair< uint32_t, uint16_t > HistoryTiker() override;

	uint16_t Overwatch(tmark_t tick, Manager *const mngr) override;
	void Add(INotification *const note) override;

	static void Init(); // todo: env params

	bool leader() const override;
	Pipeline* pipeline();

	Message MakeMessage(uint16_t size) override;

	void Unpack(graph::ptr_t m, std::list< Message >& msgs);

	void Refresh(uint64_t users);
	void Run(Manager *const mngr);

protected:
	std::string tag() const override;
    ctx::ptr_t Service(Manager *const mngr, ctx::ptr_t c, uint32_t tick) override;
	ctx::ptr_t Play(Manager *const mngr, ctx::ptr_t c, graph::ptr_t m) override;
	uint16_t magic() const override;

	void statistica(std::stringstream& ss);
	bool no_tasks();

protected:
	volatile bool done_;
	uint16_t leader_; // his magic 
	uint16_t magic_;

	std::unique_ptr< engine::Regedit > reg_;
    boost::posix_time::ptime tbase_; // timestemp (1970.01.01)

	std::vector< INotification* > notes_;
	notify::Backend notify_;

	std::mutex mut_;
	std::condition_variable cond_;

	std::vector< Worker* > workers_;
	Task jobs_;

	std::mt19937 rng_;
	int task_seq_;

	volatile uint64_t tmark_;
	std::atomic_int tseq_;

	uint64_t seq4to_;
	remote_allocator_t* remote_;
	FrameStack stack_[MAX_STACK_COUNT];
	MeasureX *measure_;
};

} // namespace akva

