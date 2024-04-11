#pragma once
#include <random>
#include <vector>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/predef.h>

#if defined(BOOST_OS_WINDOWS)
#include <Windows.h>
#endif

#include "allocator.hpp"

namespace akva{
namespace mem{

struct Node{
	enum{
		MAX_LEVEL = 16
	};

	Node* forward_[MAX_LEVEL + 1];
	Node *next_, *prev_;

	uint16_t height_;
	uint64_t key_;

	uint64_t writeset2_; // remove ... !!!!
	void* user_;
};

template< typename baseT >
class CoreSpace{
public:
	CoreSpace(baseT *const base);
	~CoreSpace();

	uint16_t RandomLevel();
	Node* Search(Node* x, uint64_t key);
	Node* Contains(uint64_t key);
	Node* Insert(uint64_t key, Node* y = nullptr);
	Node* Delete(uint64_t key, bool destroy = true);
	Node* Move(Node* x, uint64_t key);

	Node* Interval(uint64_t key, uint64_t mask);

protected:
	const float P_;
	baseT *const base_;
	std::mt19937 rng_;
	Node* trace_[Node::MAX_LEVEL + 1];
	uint16_t level_;
};

class EasySpace: protected CoreSpace< EasySpace >{
	enum{
		MAX_BLOCK_COUNT = 0xffff
	};

public:
	EasySpace(size_t blocks, size_t size);
	~EasySpace();

	std::pair< void*, uint64_t > Write(uint64_t marker = 0);
	std::pair< void*, uint64_t > Read(uint64_t marker);
	void Free(uint64_t marker);

	void* at(uint64_t marker);
	bool is_owner(uint64_t marker) const;

	uint32_t Capacity();
	static uint32_t max_block_count();

	Node* MakeNode(uint16_t level, uint64_t key);
	void FreeNode(Node* node);
	Node* Head();

private:
	const uint32_t CAPACITY;
	uint64_t seq_;
#if defined(BOOST_OS_WINDOWS)
	HANDLE hp_;
#endif

	Node head_;
	std::vector< Node* > blocks_;
};

// +++ CoreSpace +++

template< typename baseT >
CoreSpace< baseT >::CoreSpace(baseT *const base)
: P_(0.5), level_(0), base_(base)
{ }

template< typename baseT >
CoreSpace< baseT >::~CoreSpace(){}

template< typename baseT >
uint16_t CoreSpace< baseT >::RandomLevel(){
	uint16_t lvl = 0;
	std::uniform_real_distribution< > gen(0.0, 1.0);
	while((gen(rng_) < P_) && (lvl < Node::MAX_LEVEL)){
		lvl++;
	}

	return lvl;
}

template< typename baseT >
Node* CoreSpace< baseT >::Search(Node* x, uint64_t key){
	memset(trace_, 0, sizeof(trace_));
	for(int i = level_; i >= 0; i--){
		while((x->forward_[i] != nullptr) && (x->forward_[i]->key_ < key)){
			x = x->forward_[i];
		}

		trace_[i] = x;
	}

	return x->forward_[0];
}

template< typename baseT >
Node* CoreSpace< baseT >::Insert(uint64_t key, Node* y){
	Node* x = Search(base_->Head(), key);
	if(x == nullptr || x->key_ != key){
		uint16_t lvl = RandomLevel();
		if(lvl > level_){
			for(auto i = level_ + 1; i <= lvl; i++){
				trace_[i] = base_->Head();
			}
			level_ = lvl;
		}

		if(y){
			x = y;
			x->height_ = lvl;
			x->key_ = key;
		}
		else
			x = base_->MakeNode(lvl, key);

		for(auto i = 0; i <= lvl; i++){
			x->forward_[i] = trace_[i]->forward_[i];
			trace_[i]->forward_[i] = x;
		}
	}

	return x;
}

template< typename baseT >
Node* CoreSpace< baseT >::Delete(uint64_t key, bool destroy){
	Node* x = Search(base_->Head(), key);
	if(x){
		for(auto i = 0; i <= level_; i++){
			if(trace_[i]->forward_[i] != x)
				break;

			trace_[i]->forward_[i] = x->forward_[i];
		}

		while((level_ > 0) && (base_->Head()->forward_[level_] == nullptr)){
			level_--;
		}

		if(!destroy)
			return x;
		else
			base_->FreeNode(x);
	}

	return nullptr;
}

template< typename baseT >
Node* CoreSpace< baseT >::Interval(uint64_t b, uint64_t e){ // [...)
	Node* x = Search(base_->Head(), b);
	if(x && ((b <= x->key_) && (x->key_ < e)))
		return x;
	else
		return nullptr;
}

template< typename baseT >
Node* CoreSpace< baseT >::Contains(uint64_t key){
	Node* x = Search(base_->Head(), key);
	if(x && x->key_ == key){
		return x;
	}

	return nullptr;
}

template< typename baseT >
Node* CoreSpace< baseT >::Move(Node* x, uint64_t key){
	Node* y = Search(base_->Head(), key);
	if(y && (y->key_ == key))
		return nullptr;
	else{
		Node* z = Delete(x->key_, false);
		return Insert(key, z);
	}
}

} // mem
} // akva

