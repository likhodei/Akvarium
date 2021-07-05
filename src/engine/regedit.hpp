#pragma once
#ifndef ENGINE_REGEDIT_H_
#define ENGINE_REGEDIT_H_

#include "spin_lock.hpp"

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>

#include <boost/random.hpp>

#include <set>
#include <map>
#include <cstdint>
#include <list>
#include <string>
#include <boost/array.hpp>

namespace akva{

struct Register{
	uint32_t tmark_;
	uint16_t type_; // seed
	uint16_t magic_;
	uint32_t pid_; // pid
	uint64_t log_;
	uint64_t see_;

	bool ok() const;
};

namespace engine{

namespace ipc = boost::interprocess;

enum register_t{
	REG_NO = 0,
	REG_P2P = 1,
	REG_PUB = 2
};

struct SharedLock{
	SharedLock(ipc::file_handle_t pfile);
	~SharedLock();

	bool locked() const;

private:
	bool locked_;
	ipc::file_handle_t pfile_;
};

struct ScopedLock{
	ScopedLock(ipc::file_handle_t pfile);
	~ScopedLock();

private:
	ipc::file_handle_t pfile_;
};

class RegTable;

class Regedit{ // Todo: put, get, del
public:
	enum{
		MAGIC_SHIFT_MASK = 0x3f,
		MAX_RECORD_COUNT = MAGIC_SHIFT_MASK + 1
	};

	typedef std::pair< uint64_t, uint64_t > log_record_t;

	Regedit(const std::string& environment, uint32_t pid);
	~Regedit();

	const uint32_t PID;

	uint16_t GenMagic(uint16_t seed = 0xffff);
	bool Changed();

	void Refresh(uint16_t magic, uint32_t tmark);

	static uint16_t IxByMagic(uint16_t magic);
	std::set< uint16_t > MagicsByPID(uint32_t pid);

	Register* AddNode(uint16_t magic, uint16_t type = 0);
	Register* DelNode(uint16_t magic);

	Register* AddEdge(uint16_t magic, uint16_t ix);
	Register* DelEdge(uint16_t magic, uint16_t ix);

	std::vector< Register* > world();
	Register* at(uint16_t magic);

	log_record_t Log() const;
	std::string Info();

	void Update(const Register& r);

private:
	log_record_t rlog_;
	Register w2_[MAX_RECORD_COUNT];

	boost::random::mt19937 rng_;
	mutable engine::MicroSpinLock guard_;
	std::unique_ptr< RegTable > impl_; // todo: view ...
};

class RegTable{
public:
	RegTable(Regedit *const reg);
	~RegTable();

	void Init(const std::string& environment);
	void Merge(uint16_t magic, uint32_t tmark);

	Register* Commit(Register* r);
	Register* Load(uint16_t index);

	uint64_t ReCalcLog(uint64_t log);

protected:
	Register* W2(ipc::file_handle_t& pfile);

private:
	Regedit *const reg_;
	Register w2_[Regedit::MAX_RECORD_COUNT];
	ipc::file_handle_t pfile_;

	std::list< Register* > commits_;
	mutable engine::MicroSpinLock guard_;
};

} // engine;
} // akva
#endif // ENGINE_REGEDIT_H_

