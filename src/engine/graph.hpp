#pragma once
#include <cstdint>
#include <memory>

#include "frame.hpp"

namespace graph{

#pragma pack(push, 4)

struct Hdr{
	uint64_t users;
	uint32_t tmark; // time mark  Todo: need zip

	uint16_t number;
	uint16_t magic;
	uint16_t group; // protocol type
	uint16_t type;  // Graph::type_t

	int32_t MCp;
	uint16_t flags; // reserve, token, colour
	uint16_t count; // count items in mail
};

#pragma pack(pop)

enum type_t{
    ML_METADATA = 0,
    ML_DATA_ALL,
    ML_DATA_BEGIN,
    ML_DATA,
    ML_DATA_END,
    ML_COMMAND,
    ML_TIMESTAMP
};

struct Graph: public std::enable_shared_from_this< Graph >{
	enum{
		HDR_SIZE = sizeof(Hdr) >> 2 // count words
	};

	Graph(uint32_t *const body, uint16_t words);
	Graph(uint16_t group, type_t type = ML_DATA_ALL);

	void Copy(uint32_t *const body, uint16_t words);

	template < typename T >
	void Append(const T& frames){
        words_.resize(words_.size() + frames.size() * 2); // in words
        h_ = (Hdr*)words_.data();
        d_ = (uint64_t*)(words_.data() + HDR_SIZE);

        for(const auto& frame : frames)
            d_[h_->count++] = frame->key();
	}

	uint64_t Visit(uint16_t magic);
	uint16_t users() const;

	size_t hash_value() const;

	bool is_token() const;
	bool ok() const;

	Hdr *const hdr() const;

	uint8_t *const data() const;
	uint16_t bytes() const;

	uint64_t* d_;
	Hdr* h_;
	std::vector< uint32_t > words_;
};

typedef std::shared_ptr< Graph > ptr_t;

inline bool operator < (ptr_t a, ptr_t b){
	return a->hash_value() < b->hash_value();
}

} //graph
