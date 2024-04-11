#include "spin_lock.hpp"

#include <boost/assert.hpp>

namespace akva{
namespace engine{
    
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
    payload()->store(FREE, std::memory_order_release);
}

std::atomic< uint8_t >* MicroSpinLock::payload(){
    return reinterpret_cast<std::atomic< uint8_t > *>(&this->lock_);
}

bool MicroSpinLock::cas(uint8_t compare, uint8_t newVal){
    return payload()->compare_exchange_strong(compare, newVal, std::memory_order_acquire, std::memory_order_relaxed);
}

} // engine
} // akva