#pragma once
#include <cstdint>

namespace akva{

enum core_command_t{
    CORE_NONE = 0,
    CORE_TEST,
    CORE_MEASURE,
    CORE_TOKEN,
    CORE_ANNOUNCE,
    CORE_MAX_COMMANDS
};

enum group_spec_t{
    GR_ALL = 0,
    GR_CORE,
    GR_WEB,
    GR_MAX_GROUPS
};

#pragma pack(push, 4)
struct Measure{
    enum counter_t{
        JOB_QUEUE = 0,
        MSG_QUEUE,
        MAX_COUNTER_ITEMS
    };

	uint32_t tmark_;
	uint32_t elapsed_;
	uint32_t calls_;
	uint32_t counter_[MAX_COUNTER_ITEMS];

	inline void Reset(){
		memset(this, 0, sizeof(Measure));
	}
};
#pragma pack(pop)

} // akva
