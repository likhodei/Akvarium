#include "regedit.hpp"

#include <boost/thread.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/detail/os_thread_functions.hpp>

#include <boost/filesystem.hpp>
#include <sstream>
#include <iomanip>

namespace akva{

// +++ Register +++

bool Register::ok() const{
	return ((pid_ > 0) && (pid_ != ~0ul)) ? true : false;
}

namespace engine{

namespace fs = boost::filesystem;

// +++ SharedLock +++

SharedLock::SharedLock(ipc::file_handle_t pfile)
	: locked_(false), pfile_(pfile){
	locked_ = ipc::ipcdetail::acquire_file_lock_sharable(pfile_);
}

SharedLock::~SharedLock(){
	if(locked_) ipc::ipcdetail::release_file_lock_sharable(pfile_);
}

bool SharedLock::locked() const{
	return locked_;
}

// +++ ScopedLock +++

ScopedLock::ScopedLock(ipc::file_handle_t pfile)
	: pfile_(pfile){
	while(!ipc::ipcdetail::acquire_file_lock(pfile_)){
		boost::this_thread::yield();
	}
}

ScopedLock::~ScopedLock(){
	ipc::ipcdetail::release_file_lock(pfile_);
}

// +++ Regedit +++

Regedit::Regedit(const std::string& environment, uint32_t pid)
	: impl_(new RegTable(this)), rlog_(0, 0), PID(pid), rng_(pid){
	guard_.init();
	memset(w2_, 0, sizeof(w2_));
	impl_->Init(environment);
	rlog_ = Log();
}

Regedit::~Regedit(){
	impl_.reset();
}

uint16_t Regedit::GenMagic(uint16_t seed){
	boost::random::uniform_int_distribution< > gen(0, 0xffff);
	if(seed > MAGIC_SHIFT_MASK){ // fix: data race
		do{
			seed = IxByMagic(gen(rng_));
		}
		while(impl_->Load(seed)->ok());
	}

	uint16_t magic(0);
	do{ // gen magic
		magic = gen(rng_) << 6; // Fixme: MAGIC NUMBER
		magic |= seed;
	}
	while(impl_->Load(seed)->magic_ == magic);
	return magic;
}

uint16_t Regedit::IxByMagic(uint16_t magic){
	return (magic & MAGIC_SHIFT_MASK);
}

std::set< uint16_t > Regedit::MagicsByPID(uint32_t pid){
	std::set< uint16_t > ixs;
	for(const auto& w : w2_){
		boost::lock_guard< MicroSpinLock > lk(guard_);

		if(w.ok() && (w.pid_ == pid)){
			ixs.insert(w.magic_);
		}
	}
	return ixs;
}

Regedit::log_record_t Regedit::Log() const{
	log_record_t lg(0, 0);
	for(const auto& w : w2_){
		boost::lock_guard< MicroSpinLock > lk(guard_);

		if(w.ok()){
			lg.first += w.log_;   // log
			lg.second |= w.see_;  // see
		}
	}

	return lg;
}

Register* Regedit::at(uint16_t magic){
	return (w2_ + IxByMagic(magic));
}

Register* Regedit::AddNode(uint16_t magic, uint16_t type){
	Register* r = at(magic);
	r->magic_ = magic;
	r->type_ = type;
	r->pid_ = PID;
	r->see_ = 0;
	r->log_ = impl_->ReCalcLog(r->log_);

	return impl_->Commit(r);
}

Register* Regedit::DelNode(uint16_t magic){
	Register* r = at(magic);
	if(r->ok()){
		r->pid_ = ~0ul;
		r->log_ = impl_->ReCalcLog(r->log_);
		r->see_ = 0;
	}

	return impl_->Commit(r);
}

Register* Regedit::AddEdge(uint16_t magic, uint16_t pos){
	Register* r = at(magic);
	if(r->ok()){
		r->see_ |= (1ull << pos);
		r->log_ = impl_->ReCalcLog(r->log_);
	}

	return impl_->Commit(r);
}

Register* Regedit::DelEdge(uint16_t magic, uint16_t pos){
	Register* r = at(magic);
	if(r->ok()){
		uint64_t mask = 1ull << pos;
		if((r->see_ & mask) == mask)
			r->see_ ^= mask;

		r->log_ = impl_->ReCalcLog(r->log_);
	}

	return impl_->Commit(r);
}

void Regedit::Refresh(uint16_t magic, uint32_t tmark){
	impl_->Merge(magic, tmark);
}

std::vector< Register* > Regedit::world(){
	std::vector< Register* > world;
	for(Register *b = w2_, *e = w2_ + MAX_RECORD_COUNT; b < e; ++b){
		boost::lock_guard< MicroSpinLock > lk(guard_);
		if(b->ok())
			world.push_back(b);
	}

	return world;
}

bool Regedit::Changed(){
	log_record_t log = Log();
	if(log.first != rlog_.first){
		rlog_ = log;
		return true;
	}
	else
		return false;
}

std::string Regedit::Info(){
	std::stringstream ss;
	for(uint16_t i(0); i < MAX_RECORD_COUNT; ++i){
		Register r = w2_[i];
		if(r.ok()){
			ss << "M(" << std::setw(2) << i << ", " << std::hex << std::setw(6) << r.magic_ << std::dec << ")";
			ss << "[" << std::setw(8) << r.tmark_ << "]";
			ss << " PID" << std::setw(6) << r.pid_;
			ss << " L " << std::setw(3) << r.log_;

			ss << " S[";
			uint64_t edges = r.see_;
			for(auto j(0); j < 64; ++j){
				char symbol = '.';
				uint64_t maska = (1ull << j);
				if((edges & maska) == maska){
					symbol = 'x';
				}

				ss << symbol;
				//ss << std::setw(2) << symbol;
			}
			ss << "]";
			ss << std::endl;
		}
	}
	return ss.str();
}

void Regedit::Update(const Register& r){
	auto R = at(r.magic_);
	if((R->log_ < r.log_) || (R->tmark_ < r.tmark_)){
		memcpy(R, &r, sizeof(Register));
	}
}

// +++ RegTable +++

RegTable::RegTable(Regedit *const reg)
	: reg_(reg), pfile_(ipc::ipcdetail::invalid_file()){
	guard_.init();
	memset(w2_, 0, sizeof(w2_));
}

RegTable::~RegTable(){
	if(pfile_ != ipc::ipcdetail::invalid_file()){
		ipc::ipcdetail::close_file(pfile_);
	}
}

void RegTable::Init(const std::string& environment){
	memset(w2_, 0, sizeof(w2_));

	fs::path cfg(ipc::ipcdetail::get_temporary_path());
	cfg /= (environment + ".cfg");
	cfg = fs::system_complete(cfg);

	while(!fs::exists(cfg)){
		pfile_ = ipc::ipcdetail::create_new_file(cfg.string().c_str(), ipc::read_write);
		if((pfile_ == 0) || (pfile_ == ipc::ipcdetail::invalid_file()))
			continue;

		engine::ScopedLock guard(pfile_);
		for(uint16_t i(0); i < Regedit::MAX_RECORD_COUNT; ++i){
			ipc::ipcdetail::write_file(pfile_, (const void*)&w2_[i], sizeof(Register));
		}
	}

	if((pfile_ == 0) || (pfile_ == ipc::ipcdetail::invalid_file())){
		pfile_ = ipc::ipcdetail::open_existing_file(cfg.string().c_str(), ipc::read_write);
	}

	{
		engine::SharedLock guard(pfile_);
		W2(pfile_);
	}
}

Register* RegTable::W2(ipc::file_handle_t& pfile){
	memset(w2_, 0, sizeof(w2_));
	ipc::ipcdetail::set_file_pointer(pfile, 0, ipc::file_begin);

	Register r;
	unsigned long  readed(0);
	while(ipc::winapi::read_file(pfile, &r, sizeof(r), &readed, nullptr)){
		if(readed == sizeof(r)){
			if(r.ok()){
				w2_[Regedit::IxByMagic(r.magic_)] = r;
			}
		}
		else
			break;
	}

	return w2_;
}

void RegTable::Merge(uint16_t magic, uint32_t tmark){
	{ // read
		engine::SharedLock guard(pfile_);
		W2(pfile_);
	}
	{ // save
		boost::lock_guard< MicroSpinLock > lk(guard_);
		for(auto r : commits_){
			auto i = Regedit::IxByMagic(r->magic_);
			if(w2_[i].log_ < r->log_){
				w2_[i] = *r;

				ipc::offset_t offset = i * sizeof(Register);

				engine::ScopedLock guard(pfile_);
				ipc::ipcdetail::set_file_pointer(pfile_, offset, ipc::file_begin);
				ipc::ipcdetail::write_file(pfile_, (const void*)r, sizeof(Register));
				ipc::winapi::flush_file_buffers(pfile_);
			}
		}
		commits_.clear();
	}
	{ // update
		for(const auto& r : w2_){
			if(r.ok())
				reg_->Update(r);
		}
	}
	{ // mark
		auto i = Regedit::IxByMagic(magic);
		ipc::offset_t offset = i * sizeof(Register);

		w2_[i].tmark_ = tmark;

		engine::ScopedLock guard(pfile_);
		ipc::ipcdetail::set_file_pointer(pfile_, offset, ipc::file_begin);
		ipc::ipcdetail::write_file(pfile_, (const void*)&tmark, sizeof(tmark));
		ipc::winapi::flush_file_buffers(pfile_);
	}
}

Register* RegTable::Load(uint16_t index){
	ipc::offset_t offset = index * sizeof(Register);

	engine::SharedLock guard(pfile_);
	ipc::ipcdetail::set_file_pointer(pfile_, offset, ipc::file_begin);

	Register r;
	unsigned long  readed(0);
	ipc::winapi::read_file(pfile_, &r, sizeof(r), &readed, nullptr);
	if(readed == sizeof(Register)){
		w2_[index] = r;
	}

	return w2_ + index;
}

Register* RegTable::Commit(Register* r){
	{ // safe
		boost::lock_guard< MicroSpinLock > lk(guard_);
		commits_.push_back(r);
	}
	return r;
}

uint64_t RegTable::ReCalcLog(uint64_t log){
	for(const auto& w : w2_){
		if(w.ok() && (log < w.log_)){
			log = w.log_;
		}
	}

	log += 1;
	return log;
}

} // engine
} // akva
