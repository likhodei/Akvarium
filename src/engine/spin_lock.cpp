#include "spin_lock.hpp"

namespace akva{
namespace engine{

// +++ SpinLock +++

SpinLock::SpinLock()
    : state_(Unlocked){}

//#define SPIN_ATOMIC_THREAD_FENCE

void SpinLock::Lock(){
#if defined(SPIN_ATOMIC_THREAD_FENCE)	
    while(state_.exchange(Locked, boost::memory_order_relaxed) == Locked);
    boost::atomic_thread_fence(boost::memory_order_acquire);
#else
    while(state_.exchange(Locked, boost::memory_order_acquire) == Locked){ /*boost::this_thread::yield();  <- bad idea */
    }
#endif
}

void SpinLock::UnLock(){
    state_.store(Unlocked, boost::memory_order_release);
}

bool SpinLock::TryLock(){
    return (state_.exchange(Locked, boost::memory_order_acquire) == Locked) ? false : true;
}

// +++ SpinLockII +++

SpinLockII::SpinLockII()
    : refcount_(0){}

int SpinLockII::Lock(){
    return refcount_.fetch_add(1, boost::memory_order_relaxed);
}

void SpinLockII::UnLock(){
    if(refcount_.fetch_sub(1, boost::memory_order_release) == 1){
        boost::atomic_thread_fence(boost::memory_order_acquire);
    }
}

// +++ LockGuardII +++

LockGuardII::LockGuardII(SpinLockII& spin)
    : spin_(spin), locked_((spin.Lock() == 0) ? true : false){}

LockGuardII::~LockGuardII(){
    spin_.UnLock();
}

bool LockGuardII::is_locked() const{
    return locked_;
}


// +++ MicroSpinLock +++

void MicroSpinLock::init(){
    payload()->store(FREE);
}

bool MicroSpinLock::try_lock(){
    return cas(FREE, LOCKED);
}

void MicroSpinLock::lock(){
    Sleeper sleeper;
    do{
        while(payload()->load() != FREE){
            sleeper.wait();
        }
    }
    while(!try_lock());
    BOOST_ASSERT(payload()->load() == LOCKED);
}

void MicroSpinLock::unlock(){
    BOOST_ASSERT(payload()->load() == LOCKED);
    payload()->store(FREE, boost::memory_order_release);
}

boost::atomic< uint8_t >* MicroSpinLock::payload(){
    return reinterpret_cast<boost::atomic< uint8_t > *>(&this->lock_);
}

bool MicroSpinLock::cas(uint8_t compare, uint8_t newVal){
    return payload()->compare_exchange_strong(compare, newVal, boost::memory_order_acquire, boost::memory_order_relaxed);
}

} // engine
} // akva