#pragma once
#ifndef ENGINE_DETAIL_SLEEPER_H_
#define ENGINE_DETAIL_SLEEPER_H_

#include <cstdint>

void asm_volatile_pause();
int nanosleep(time_t tv_sec, long tv_nsec);

namespace akva{
namespace engine{

class Sleeper{
public:
    Sleeper();
    void wait();

private:
    static const uint32_t kMaxActiveSpin = 4000;
    uint32_t spin_count_;
};

} // engine
} // akva

#endif ENGINE_DETAIL_SLEEPER_H_