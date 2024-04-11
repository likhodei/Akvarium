#pragma once
#include <atomic>

#include "detail/sleeper.hpp"

namespace akva{
namespace engine{

struct MicroSpinLock{
	enum{ FREE = 0, LOCKED = 1 };
	uint8_t lock_; // can't be std::atomic<> to preserve POD-ness.

	// Initialize this MSL.  It is unnecessary to call this if you
	// zero-initialize the MicroSpinLock.
	void init();
	bool try_lock();

	void lock();
	void unlock();

private:
	std::atomic< uint8_t >* payload();
	bool cas(uint8_t compare, uint8_t newVal);
};

} // engine
} // akva
