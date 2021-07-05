#pragma once
#ifndef SPIN_GUARD_H_
#define SPIN_GUARD_H_

#include "horse_helper.hpp"
#include "spin_lock.hpp"

namespace kernel_ns{

template < class lockerT >
struct SpinGuard{
	typedef lockerT locker_t;

	explicit SpinGuard(lockerT& locker)
	: locker_(locker), TID_(worker_ns::fn_local_tid()){ //!!! изначально, должен быть проинициализирован
		locker_.Lock(TID_);
	}

	~SpinGuard(){
		locker_.Unlock(TID_);
	}

private:
	const uint16_t TID_;
	lockerT& locker_;
};

template < >
struct SpinGuard< SpinLock >{
	typedef SpinLock locker_t;

	explicit SpinGuard(SpinLock& locker)
	: locker_(locker){
		locker_.Lock();
	}

	~SpinGuard(){
		locker_.UnLock();
	}

private:
	SpinLock& locker_;
};

struct NoLock{ };

template < >
struct SpinGuard< NoLock >{
	typedef NoLock locker_t;

	explicit SpinGuard(NoLock& locker)
	: locker_(locker)
	{ }

	~SpinGuard()
	{ }

private:
	NoLock& locker_;
};

} // kernel_ns

#endif // SPIN_GUARD_H_