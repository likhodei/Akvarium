#pragma once
#include <ctime>
#include <cstdint>

void asm_volatile_pause();
int nanosleep(std::time_t tv_sec, long tv_nsec);

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
