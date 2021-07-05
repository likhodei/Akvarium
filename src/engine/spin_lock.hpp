#pragma once
#ifndef ENGINE_SPIN_LOCK_H_
#define ENGINE_SPIN_LOCK_H_

#include "engine/detail/sleeper.hpp"

#include <boost/assert.hpp>
#include <boost/atomic.hpp>

namespace akva{
namespace engine{

class SpinLock{
	enum lock_state_t{
		Locked,
		Unlocked
	};

public:
	SpinLock();

	void Lock();
	void UnLock();
	bool TryLock();

private:
	boost::atomic< lock_state_t > state_;
};

namespace lock{
enum lock_guard_policy_t{
	USE_LOCK = 1,
	TRY_LOCK = 2
};
} // lock

template< int specT >
struct LockGuardEx{
	template < int vT >
	struct Int2Type{
		enum{ value = vT };
	};

	LockGuardEx(SpinLock& spin_lock)
		: spin_lock_(spin_lock), locked_(false){
		DoLock(typename Int2Type< specT  >());
	}

	~LockGuardEx(){
		if(locked_) spin_lock_.UnLock();
	}

	bool is_locked() const{
		return locked_;
	}

protected:
	void DoLock(Int2Type< lock::TRY_LOCK > /*tag*/){
		locked_ = spin_lock_.TryLock();
	}

	void DoLock(Int2Type< lock::USE_LOCK > /*tag*/){
		locked_ = true; spin_lock_.Lock();
	}

private:
	SpinLock& spin_lock_;
	volatile bool locked_;
};

typedef LockGuardEx< lock::USE_LOCK > LockGuard;
typedef LockGuardEx< lock::TRY_LOCK > TryLockGuard;

class SpinLockII{
public:
	SpinLockII();

	int Lock();
	void UnLock();

private:
	boost::atomic< int > refcount_;
};

struct LockGuardII{
	LockGuardII(SpinLockII& /*spin*/);
	~LockGuardII();

	bool is_locked() const;

private:
	SpinLockII& spin_;
	volatile bool locked_;
};

struct MicroSpinLock{
	enum{ FREE = 0, LOCKED = 1 };
	uint8_t lock_; // can't be boost::atomic<> to preserve POD-ness.

	// Initialize this MSL.  It is unnecessary to call this if you
	// zero-initialize the MicroSpinLock.
	void init();
	bool try_lock();

	void lock();
	void unlock();

private:
	boost::atomic< uint8_t >* payload();
	bool cas(uint8_t compare, uint8_t newVal);
};

} // engine
} // akva
#endif // ENGINE_SPIN_LOCK_H_
