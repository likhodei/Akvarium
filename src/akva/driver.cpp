#include "driver.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <boost/bind.hpp>
#include <boost/intrusive/circular_list_algorithms.hpp>

#include "declaration.hpp"
#include "engine/logger.hpp"
#include "engine/regular_registr.hpp"

namespace pt = boost::posix_time;
namespace ipc = boost::interprocess;

using namespace akva;
using engine::Regedit;

#if defined(BOOST_ENABLE_ASSERT_HANDLER)
namespace boost
{
	void assertion_failed(char const * expr, char const * function, char const * file, long line){
		std::cerr << "ASSERT[" << expr << "][" << function << "][" << file << "]=" << line << std::endl;
	}

	void assertion_failed_msg(char const * expr, char const * msg, char const * function, char const * file, long line){
		std::cerr << "ASSERT MSG[" << expr << "][" << function << "][" << file << "]=" << line << std::endl;
	}
}
#endif

template< typename T >
struct ListTraits{
	typedef T node;
	typedef T* node_ptr;
	typedef const T* const_node_ptr;

	static node_ptr get_next(const_node_ptr n){ return n->next_; }
	static void set_next(node_ptr n, node_ptr next){ n->next_ = next; }
	static node* get_previous(const_node_ptr n){ return n->prev_; }
	static void set_previous(node_ptr n, node_ptr prev){ n->prev_ = prev; }
};

typedef boost::intrusive::circular_list_algorithms< ListTraits< Pipeline > > PipeAlgo;
typedef boost::intrusive::circular_list_algorithms< ListTraits< MeasureX > > MeshAlgo;
typedef boost::intrusive::circular_list_algorithms< ListTraits< Worker > > WorkAlgo;
typedef boost::intrusive::circular_list_algorithms< ListTraits< Task > > TaskAlgo;

// +++ Task +++

void Task::Put(Worker* w){
    if(wk_)
        WorkAlgo::link_before(wk_, w);
    else{
        WorkAlgo::init_header(w);
        wk_ = w;
    }
}

Worker* Task::Pop(){
	if(empty())
		return nullptr;
	else{
        Worker* w = wk_;
		wk_ = wk_->next_;
		if(WorkAlgo::unique(w))
			wk_ = nullptr;
		else
            WorkAlgo::unlink(w);

		return w;
	}
}

void Task::Del(Worker* wk){
	BOOST_ASSERT(!empty());

    Worker* x = wk_;
    do{
        if(x == wk){
			if(wk == wk_){ // first
				if(WorkAlgo::unique(wk_)) // alone
					wk_ = nullptr;
				else
                    wk_ = wk_->next_;
			}

            WorkAlgo::unlink(x);
			break;
        }
        else
            x = x->next_;
    }
    while(x != wk_);
}

size_t Task::count(){
    return empty() ? 0 : WorkAlgo::count(wk_);
}

bool Task::empty(){
	return (wk_ == nullptr) ? true : false;
}

bool Task::Complite(size_t workers){
    return (empty() && (looks_ == workers)) ? true : false;
}

// +++ Worker +++

uint64_t Worker::On(Manager *const mngr, tmark_t now){
    auto t = pt::microsec_clock::universal_time();
	if(job_){
		BOOST_ASSERT(marker_ <= job_->marker_);
        marker_ = job_->marker_;

        //std::cout << "+ : " << line_->tag() << " | "  << marker_ << " : " << t.first << "-" << t.second << std::endl;

        ctx::ptr_t ctx = (job_->m_) ? job_->act_->Play(mngr, line_, job_->m_) : (*(job_->act_))(mngr, line_);
		if(!ctx){
            job_->looks_ += 1; // complite count
            job_->Del(this);
            job_ = job_->next_;
            job_->Put(this);
		}
		else{
			// loops
			char ch = 0x00;
		}

		if(job_->marker_ == 0){ // idle
			Leave(job_);
		}
	}

    return (pt::microsec_clock::universal_time() - t).total_microseconds();
}

bool Worker::empty(){
	return job_ ? false : true;
}

void Worker::Plain(Task* job){
	BOOST_ASSERT(job_ == nullptr);
	job_ = job;
}

void Worker::Leave(Task* job){
	BOOST_ASSERT(job_ == job);
	job_ = nullptr;
}

// +++ Driver +++

Driver::Driver()
: remote_(nullptr), seq4to_(0), done_(false), leader_(0), magic_(0), tseq_(0), measure_(nullptr),
    tbase_(pt::time_from_string("1970-Jan-1 00:00:00.000000")), task_seq_(0), rng_(42){
	tmark_ = tiker64();

	measure_ = new MeasureX;
    MeshAlgo::init_header(measure_);
	for(auto i(1); i < MAX_MEASURES; ++i){
		MeshAlgo::link_before(measure_, new MeasureX);
		measure_->prev_->impl_.Reset();
	}

    measure_->impl_.Reset();
    measure_->impl_.tmark_ = tiker();

    PipeAlgo::init_header(this);

	{ // task
		memset(&jobs_, 0, sizeof(jobs_));
		TaskAlgo::init_header(&jobs_);

        Worker* wk = new Worker();
		{
			memset(wk, 0, sizeof(wk));
			wk->line_ = this;
			//wk->key_ = workers_.size();

			WorkAlgo::init_header(wk);
			workers_.push_back(wk);
		}
		jobs_.Put(wk);
	}
}

Driver::~Driver(){
	reg_.reset();

	for(auto n : notes_){
        notify_.Detach(n);
		delete n;
	}

	for(auto& s : stack_)
		s.CleanUp();

	if(remote_)
		delete remote_;

	{ //clear
		for(auto w : workers_)
			delete w;

		workers_.clear();
	}
}

Pipeline* Driver::pipeline()
{ return this; }

bool Driver::no_tasks(){
	return TaskAlgo::unique(&jobs_);
}

void Driver::Refresh(uint64_t users){
	if(remote_)
        remote_->space_.Update(users);
}

Message Driver::MakeMessage(uint16_t size){
	frame_ptr_t frame(new Frame(remote_));
    return Message(size, frame);
}

void Driver::Unpack(graph::ptr_t m, std::list< Message >& msgs){
	for(auto i(0); i < m->hdr()->count; ++i){
		auto k = m->d_[i];
		if(remote_->is_owner(k)){
			auto j = remote_->basket(k);
#if 0
			frame_ptr_t body(new Frame(k, remote_));
			for(FrameStack* c = stack_[j].Head(body); c != nullptr; c = c->Next(body)){
				if(c->Complited()){
					msgs.push_back(Message(c->Take()));
				}
			}
#endif
		}
		else{
			auto j = remote_->basket(k);
			frame_ptr_t body(new Frame(k, remote_));
			for(FrameStack* c = stack_[j].Head(body); c != nullptr; c = c->Next(body)){
				if(c->Complited()){
					msgs.push_back(Message(c->Take()));
				}
			}
		}
	}
}

bool Driver::leader() const{
	return leader_ == magic_ ? true : false;
}

void Driver::Add(INotification *const note){
    notes_.push_back(note);
    notify_.Attach(note);
}

void Driver::Attach(Pipeline *const line){ // T->M->X.Y.Z->P->
	if(PipeAlgo::unique(this))
		PipeAlgo::link_before(this, line);
	else
        PipeAlgo::link_after(this, line);

    Worker* wk = new Worker();
    {
        WorkAlgo::init(wk);
        wk->line_ = line;
        //wk->key_ = workers_.size();
        workers_.push_back(wk);
    }

	jobs_.Put(wk);
}

void Driver::Detach(Pipeline *const line){
    PipeAlgo::unlink(line);
}

uint16_t Driver::Reset(const std::string& environment, uint16_t kseed){
	reg_.reset(new Regedit(environment, ipc::ipcdetail::get_current_process_id()));
	magic_ = reg_->GenMagic(kseed);

	if(remote_)
		delete remote_;

	remote_ = new remote_allocator_t(magic_, Manager::MEM_DEEP);
	return magic_;
}

Regedit *const Driver::reg(){
    return reg_.get();
}

INotification *const Driver::notify(){
    return &notify_;
}

bool Driver::stopped() const{
	return done_;
}

void Driver::Terminate(){
	done_ = true;
    cond_.notify_all();
}

uint16_t Driver::magic() const
{ return magic_; }

void Driver::Status(){
	std::string msg = "test [" + std::to_string(++seq4to_) + "]";
	{
		auto t = HistoryTiker();
		Exec(this, t, std::make_shared< cmd::Test >(msg), graph::ptr_t());
	}
	{
		auto t = HistoryTiker();
		Exec(this, t, std::make_shared< cmd::Info >(), graph::ptr_t());
	}
}

void Driver::Exec(tmark_t t, ctx::ptr_t c){
	Exec(this, t, c, graph::ptr_t());
}

void Driver::Exec(Pipeline* line, tmark_t t, ctx::ptr_t c, graph::ptr_t m){
    Task *task = new Task;

    TaskAlgo::init(task);
    task->wk_ = nullptr;
	task->looks_ = 0;

	task->t_ = t;
	task->act_ = c;
	task->m_ = m;

	{ // safe
		std::lock_guard< std::mutex > lk(mut_);

        task->marker_ = ++task_seq_;
        TaskAlgo::link_before(&jobs_, task);

        auto sz = TaskAlgo::count(&jobs_);
		cond_.notify_all();
	}

	//TaskAlgo::count(&jobs_);
}

uint64_t Driver::tiker64() const{
    auto now = pt::microsec_clock::universal_time();
    return (now - tbase_).total_milliseconds();
}

uint32_t Driver::tiker() const{
	return (uint32_t)(tiker64() & 0xfffffffful);
}

tmark_t Driver::HistoryTiker(){
	uint64_t now = tiker64();
	if(tmark_ == now){
		tseq_.fetch_add(1, std::memory_order_relaxed);
	}
	else{
		tmark_ = now;
		tseq_.store(0);
	}

	uint16_t tseq = tseq_.load(std::memory_order_acquire);
	return std::make_pair((uint32_t)(now & 0xfffffffful), tseq);
}

void Driver::Run(Manager *const mngr){
    std::uniform_int_distribution< > gen(0, workers_.size() - 1);
    uint32_t qcount(0);
	std::vector< Task* > kf;
	while(!done_){
		std::unique_lock< std::mutex > lk(mut_);

        cond_.wait(lk, [&]{
            return done_ || !no_tasks();
        });

		while(!TaskAlgo::unique(&jobs_)){
            Task* task = jobs_.next_;
            if(!jobs_.empty()){
                auto idels = jobs_.count();
                for(auto j(0); j < idels; ++j){
                    Worker* wk = jobs_.Pop();
                    if(wk->marker_ < task->marker_){
                        task->Put(wk);
                        wk->Plain(task);
                    }
                    else
                        jobs_.Put(wk);
                }

				auto jc = jobs_.count();
				auto tc = task->count();
            }

			if(task->Complite(workers_.size())){
				qcount = TaskAlgo::count(&jobs_);

				TaskAlgo::unlink(task);
				kf.push_back(task);
				//std::cout << "REMOVE: " << task->marker_ << " sz " << qcount << std::endl;
			}
			else
				break;
        }

        lk.unlock();
		if(!kf.empty()){
			for(auto k : kf)
				delete k;

			kf.clear();
		}

        tmark_t now = HistoryTiker();
		if((now.first - measure_->impl_.tmark_) > MESH_INTERVAL){
			{
				std::stringstream ss;
				ss << "STAT ";
				ss << std::setw(4) << measure_->impl_.calls_ << ",";
				ss << std::setw(5) << measure_->impl_.counter_[Measure::JOB_QUEUE] << ",";
				ss << std::setw(5) << measure_->impl_.counter_[Measure::MSG_QUEUE] << ",";
				ss << std::setw(7) << measure_->impl_.elapsed_;

				std::cout << ss.str() << std::endl;
			}

			measure_ = measure_->next_;

			measure_->impl_.Reset();
			measure_->impl_.tmark_ = now.first;
			measure_->impl_.counter_[Measure::JOB_QUEUE] = qcount;
		}

		{
#if defined(USE_PRIORITY) // Todo: priority
			auto wk = workers_[gen(rng_)];
			if(!wk->empty()){
				measure_->impl_.elapsed_ += wk->On(mngr, now);
                measure_->impl_.calls_ += 1;
			}
#else
			for(auto wk : workers_){
                if(!wk->empty()){
                    measure_->impl_.elapsed_ += wk->On(mngr, now);
                    measure_->impl_.calls_ += 1;
                }
			}
#endif

			measure_->impl_.counter_[Measure::MSG_QUEUE] += 1;
		}
	}
}

uint16_t Driver::Overwatch(tmark_t tick, Manager *const mngr){
    reg()->Refresh(magic_, tick.first);

	{ // select leader process by PID
		uint32_t lpid = 0;
		if(leader_ != 0){
			auto r = reg()->at(leader_);
			if(r->ok() && !mngr->Guilty(r->magic_))
				lpid = r->pid_;
			else
				leader_ = 0;
		}

        for(auto r : reg()->world()){ 
			if(r->ok() && !mngr->Guilty(r->magic_)){
				if(lpid < r->pid_)
					leader_ = r->magic_;
			}
        }
	}

	return leader_;
}

std::string Driver::tag() const{
	return "M";
}

void Driver::statistica(std::stringstream& ss){
	Pipeline::statistica(ss);
	ss << " CMDs: " << TaskAlgo::count(&jobs_);
	ss << " JOBS: " << jobs_.count();
}

ctx::ptr_t Driver::Service(Manager *const mngr, ctx::ptr_t c, uint32_t tick){
	mngr->Service(this, tick);
	return ctx::ptr_t();
}

ctx::ptr_t Driver::Play(Manager *const mngr, ctx::ptr_t c, graph::ptr_t m){
	if(m->hdr()->type == graph::ML_COMMAND){
		switch(m->hdr()->group){
		case akva::CORE_MEASURE:{
			auto t = m->hdr()->tmark;
			auto c = m->hdr()->count;
			if(c == 0){
				std::list< akva::Message > msgs;
				MeasureX *mesh = measure_;
				for(auto i(0); i < 10; ++i){
					akva::Message msg = Manager::app()->MakeMessage(sizeof(Measure));
					if(msg.ok()){
						msg.Copy((uint8_t*)(&mesh->impl_), sizeof(Measure));
						//msg.set_flags(42);
						msgs.push_back(msg);
					}

					mesh = mesh->prev_;
				}

				Broadcast(mngr, graph::ML_COMMAND, akva::CORE_MEASURE, msgs);
			}
			break;
		} // case
		default:
			break;
		}; // switch
	}

    mngr->Play(this, m);
	return ctx::ptr_t();
}

