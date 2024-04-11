#pragma once
#include <memory>
#include <boost/predef.h>

#if defined(BOOST_OS_WINDOWS)
#include <Windows.h>
#endif

#include "allocator.hpp"
#include "memory.hpp"

namespace akva{
namespace mem{

struct SpaceCtrl{
	uint16_t magic_;
	uint32_t pid_;
};

struct SpaceBlock{
	enum{ BUFFER_SIZE = 4 * 1024 }; // 16 * 1024

	inline uint32_t capacity() const{
		return BUFFER_SIZE;
	}

	uint8_t body_[BUFFER_SIZE]; // Warning: it's first payload !
	uint64_t log_;
	uint64_t writeset_;
};

struct SharedSpace{
	enum{
		MAGIC_SHIFT_MASK = 0x3f,
		MAX_NODES_SIZE = 0xffff
	};

public:
	SharedSpace(uint16_t magic, const std::string& space_name, uint32_t basket_size);
	virtual ~SharedSpace();

	// operation
	uint16_t Magic();
	uint32_t Capacity(void* item) const;
	SpaceBlock* const at(uint32_t pos);

	std::pair< SpaceBlock*, void* > get(uint32_t index);

	uint16_t SHIFT;
	uint64_t bytes() const;

	const uint32_t BASKET_SIZE;
	void* users_[MAX_NODES_SIZE];

protected:
	SpaceCtrl* ctrl_;

	std::unique_ptr< boost::interprocess::mapped_region > region_;
	std::unique_ptr< boost::interprocess::windows_shared_memory > shm_;
	std::string name_;
};

class SpaceMaster: public SharedSpace{
public:
	SpaceMaster(uint16_t magic, uint32_t basket_size);
};

class SpaceSlave: public SharedSpace{
public:
	SpaceSlave(uint16_t shift, uint32_t basket_size);
};

class RegularRegistr: protected CoreSpace< RegularRegistr >{
	enum{
		MAX_SPACES = 64
	};

public:
	RegularRegistr(uint16_t magic, uint32_t basket_size = 0xffff);
	~RegularRegistr();

	std::pair< void*, uint64_t > Write(uint64_t marker = 0);
	std::pair< void*, uint64_t > Read(uint64_t marker);
	bool Free(uint64_t marker); // COMPLITE

	void* at(uint64_t marker); // pointer
	bool is_owner(uint64_t marker) const;

	bool Ok(uint64_t marker);

	void Update(uint64_t magics);
	uint32_t Capacity();
	uint64_t bytes() const;

	Node* MakeNode(uint16_t level, uint64_t key);
	void FreeNode(Node* node);
	Node* Head();

	void Print() const;
	void Statistica();

	bool empty() const;

protected:
	const uint32_t BASKET_SIZE;

private:
	uint64_t MASTER_MASK;
	uint16_t master_; // shift
	SharedSpace* table_[MAX_SPACES];
	uint64_t correct_;

	Node* head_;
	uint32_t bias_;

#if defined(BOOST_OS_WINDOWS)
	HANDLE hp_;
#endif
};

} // mem
} // akva

