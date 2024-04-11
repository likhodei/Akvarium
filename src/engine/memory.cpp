#include "memory.hpp"

#include <memory>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <malloc.h>

#include <boost/intrusive/circular_list_algorithms.hpp>

namespace akva{
namespace mem{

struct NodeListTraits{
	typedef Node node;
	typedef Node* node_ptr;
	typedef const Node* const_node_ptr;

	static node_ptr get_next(const_node_ptr n){ return n->next_; }
	static void set_next(node_ptr n, node_ptr next){ n->next_ = next; }
	static node* get_previous(const_node_ptr n){ return n->prev_; }
	static void set_previous(node_ptr n, node_ptr prev){ n->prev_ = prev; }
};

typedef boost::intrusive::circular_list_algorithms< NodeListTraits > AlgoN;

// +++ EasySpace +++

EasySpace::EasySpace(size_t blocks, size_t size)
: CoreSpace(this), CAPACITY(size), seq_(1){
	hp_ = HeapCreate(HEAP_NO_SERIALIZE, 0x01000, 0);

	memset(&head_, 0, sizeof(head_));
	head_.height_ = Node::MAX_LEVEL;
	AlgoN::init_header(&head_);

	blocks_.resize(blocks);

	UniMessage um;
	um.log = seq_;
	um.shift = 0;
	for(um.pos = 0; um.pos < blocks; ++um.pos){
		blocks_[um.pos] = MakeNode(0, um.Pack());
		AlgoN::link_after(&head_, blocks_[um.pos]);
	}
}

EasySpace::~EasySpace(){
	for(Node* n : blocks_)
		FreeNode(n);

	blocks_.clear();
	BOOL ok = HeapDestroy(hp_);
	BOOST_ASSERT(ok != FALSE);
}

Node* EasySpace::Head(){
	return &head_;
}

Node* EasySpace::MakeNode(uint16_t level, uint64_t key){
	Node* node = (Node*)HeapAlloc(hp_, HEAP_NO_SERIALIZE, sizeof(Node) + CAPACITY);
	memset(node, 0, sizeof(Node));
	node->height_ = level;
	node->key_ = key;
	node->user_ = ((uint8_t*)node + sizeof(Node));
	return node;
}

void EasySpace::FreeNode(Node* n){
	BOOL ok = HeapFree(hp_, HEAP_NO_SERIALIZE, n);
	BOOST_ASSERT(ok != FALSE);
}

std::pair< void*, uint64_t > EasySpace::Write(uint64_t marker){
	UniMessage um;
	Node* y = Contains(marker);
	if(y && um.UnPack(marker)){ // update
		um.log = ++seq_;
		Move(y, um.Pack());
	}
	else{
		um.shift = 0;
		if(AlgoN::unique(&head_)){ // append
			if(blocks_.size() < MAX_BLOCK_COUNT){
				um.log = ++seq_;
				um.pos = blocks_.size();

				y = Insert(um.Pack());
				blocks_.push_back(y);
			}
			else
				return std::make_pair(nullptr, 0);
		}
		else{
			y = head_.next_;
			AlgoN::unlink(y);

			um.UnPack(y->key_);
			um.log = ++seq_;

			y = Insert(um.Pack(), y);
		}

		y->writeset2_ = 1;
		memset(y->user_, 0, CAPACITY);
	}

	return std::make_pair(y->user_, y->key_);
}

std::pair< void*, uint64_t > EasySpace::Read(uint64_t marker){
	UniMessage um;
	Node* x = Contains(marker);
	if(x && um.UnPack(marker)){
		BOOST_ASSERT(blocks_[um.pos]);
		while(++um.shift < 64){
			uint64_t mask((uint64_t)1 << um.shift);
			if((x->writeset2_ & mask) != mask){
				x->writeset2_ |= mask;
				break;
			}
		}

		return std::make_pair(x->user_, um.Pack());
	}
	else
		return std::make_pair(nullptr, 0);
}

void EasySpace::Free(uint64_t marker){
	UniMessage um;
	if(um.UnPack(marker)){
		Node* x = blocks_.at(um.pos);
		if(x){
			uint64_t mask((uint64_t)1 << um.shift);
			if((x->writeset2_ & mask) == mask){
				x->writeset2_ ^= mask;

				if(x->writeset2_ == 0){
					AlgoN::link_before(&head_, x);
					Delete(x->key_, false);
				}
			}
		}
	}
}

void* EasySpace::at(uint64_t marker){
	UniMessage uni;
	if(uni.UnPack(marker)){
		return (blocks_[uni.pos]) ? blocks_[uni.pos]->user_ : nullptr;
	}
	else
		return nullptr;
}

bool EasySpace::is_owner(uint64_t marker) const{
	UniMessage uni;
	if(uni.UnPack(marker))
		return (uni.shift == 0) ? true : false;
	else
		return false;
}

uint32_t EasySpace::Capacity(){
	return CAPACITY;
}

uint32_t EasySpace::max_block_count(){
	return MAX_BLOCK_COUNT;
}

} // mem
} // akva
