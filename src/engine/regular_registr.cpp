#include "regular_registr.hpp"

#include <malloc.h>

#include <boost/make_shared.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/windows_shared_memory.hpp>
#include <boost/random/uniform_real_distribution.hpp>

#include <boost/intrusive/circular_list_algorithms.hpp>

#include <iostream>

#include <memory>
#include <stdexcept>
#include <algorithm>

#include <sstream>
#include <iomanip>

namespace ipc = boost::interprocess;

namespace akva{
namespace mem{

const int BIT_INDEX_MAGIC[64] = {
  63,  0, 58,  1, 59, 47, 53,  2,
  60, 39, 48, 27, 54, 33, 42,  3,
  61, 51, 37, 40, 49, 18, 28, 20,
  55, 30, 34, 11, 43, 14, 22,  4,
  62, 57, 46, 52, 38, 26, 32, 41,
  50, 36, 17, 19, 29, 10, 13, 21,
  56, 45, 25, 31, 35, 16,  9, 12,
  44, 24, 15,  8, 23,  7,  6,  5
};

#if defined(_MSC_VER)
# pragma warning(push) // Save warning settings.
# pragma warning(disable : 4146)
#endif

static int fn_first_bit_index(uint64_t* value){
	int result = BIT_INDEX_MAGIC[((*value & -*value) * 0x07EDD5E59A4E28C2ull) >> 58];
	*value &= *value - 1;
	return result;
}

#if defined(_MSC_VER)
# pragma warning(pop) // Restore warnings to previous state.
#endif


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

// +++ SharedSpace +++

SharedSpace::SharedSpace(uint16_t magic, const std::string& space_name, uint32_t basket_size)
	: name_(space_name), SHIFT(magic & MAGIC_SHIFT_MASK), ctrl_(nullptr), BASKET_SIZE(basket_size){
	memset(users_, 0, sizeof(users_));
	name_.append("-" + std::to_string(SHIFT));
	//Create a shared memory object.
	shm_.reset(new ipc::windows_shared_memory(ipc::open_or_create, name_.c_str(), ipc::read_write, bytes()));
}

SharedSpace::~SharedSpace(){
	region_.reset();
	shm_.reset();

	if(!name_.empty()){
		ipc::shared_memory_object::remove(name_.c_str());
	}
}

uint64_t SharedSpace::bytes() const{
	return sizeof(SpaceCtrl) + BASKET_SIZE * sizeof(SpaceBlock);
}

uint16_t SharedSpace::Magic(){
	BOOST_ASSERT(ctrl_);
	return ctrl_->magic_;
}

uint32_t SharedSpace::Capacity(void* ptr) const{
	return reinterpret_cast<SpaceBlock* const>(ptr)->capacity();
}

SpaceBlock* const SharedSpace::at(uint32_t pos){
	ptrdiff_t offset = sizeof(SpaceCtrl) + pos * sizeof(SpaceBlock);
	return (SpaceBlock*)((uint8_t*)ctrl_ + offset);
}

std::pair< SpaceBlock*, void* > SharedSpace::get(uint32_t index){
	return std::make_pair(at(index), users_[index]);
}

// +++ SpaceMaster +++

SpaceMaster::SpaceMaster(uint16_t magic, uint32_t basket_size)
	: SharedSpace(magic, "space", basket_size){
	try{
		region_.reset(new ipc::mapped_region(*(shm_.get()), ipc::read_write));
		//get the address of the mapped region
		void* addr = region_->get_address();

		std::memset(addr, 0, sizeof(SpaceCtrl));
		ctrl_ = new (addr)SpaceCtrl;
		ctrl_->magic_ = magic;
		//ipc::scoped_lock<ipc::interprocess_mutex> lock(ctrl_->mutex_);

		ptrdiff_t offset = sizeof(SpaceCtrl);
		ptrdiff_t total = bytes();
		do{
			SpaceBlock *block = new ((uint8_t*)addr + offset)SpaceBlock;
			offset += sizeof(SpaceBlock);
		}
		while(offset < total);
	}
	catch(boost::interprocess::interprocess_exception& ex){
		ptrdiff_t total = bytes();
		std::cerr << "MEM ERROR [" << ex.get_error_code() << "](" << ex.what() << ")" << std::endl;
		throw;
	}
}

// +++ SpaceSlave +++

SpaceSlave::SpaceSlave(uint16_t shift, uint32_t basket_size)
	: SharedSpace(shift, "space", basket_size){
	try{
		auto pg = ipc::mapped_region::get_page_size();
		auto tb = bytes();

		region_.reset(new ipc::mapped_region(*(shm_.get()), ipc::read_write));
		//get the address of the mapped region
		void* addr = region_->get_address();

		ctrl_ = (SpaceCtrl*)(addr);
		//ipc::scoped_lock<ipc::interprocess_mutex> lock(ctrl_->mutex_);

		ptrdiff_t offset = sizeof(SpaceCtrl);
		ptrdiff_t total = bytes();
		do{
			SpaceBlock *block = (SpaceBlock*)((uint8_t*)addr + offset);
			offset += sizeof(SpaceBlock);
		}
		while(offset < total);
	}
	catch(boost::interprocess::interprocess_exception& ex){
		ptrdiff_t total = bytes();
		std::cerr << "MEM ERROR [" << ex.get_error_code() << "](" << ex.what() << ")" << std::endl;
		throw;
	}
}

// +++ RegularRegistr +++

RegularRegistr::RegularRegistr(uint16_t magic, uint32_t basket_size)
	: CoreSpace(this), master_(magic & SharedSpace::MAGIC_SHIFT_MASK), BASKET_SIZE(basket_size), correct_(0), hp_(nullptr), bias_(0){
	MASTER_MASK = ((uint64_t)1 << master_);

	hp_ = HeapCreate(HEAP_NO_SERIALIZE, 0x01000, 0);
	head_ = MakeNode(Node::MAX_LEVEL, 0);
	AlgoN::init_header(head_);

	memset(table_, 0, sizeof(table_));
	table_[master_] = new SpaceMaster(magic, BASKET_SIZE);
	correct_ |= ((uint64_t)1 << master_);
}

RegularRegistr::~RegularRegistr(){
	while(!AlgoN::unique(head_)){
		Node* x = head_->next_;
		AlgoN::unlink(x);
		Delete(x->key_);
	}

	for(SharedSpace* s : table_){
		if(s)
			delete s;
	}

	BOOL ok = HeapDestroy(hp_);
	BOOST_ASSERT(ok != FALSE);
}

bool RegularRegistr::empty() const{
	return AlgoN::unique(head_);
}

void RegularRegistr::Update(uint64_t magics){ // ToDo: ... remove ...
	if(correct_ == magics)
		return;

	while(magics != 0){
		int iv = fn_first_bit_index(&magics);
		if(table_[iv] == nullptr){
			table_[iv] = new SpaceSlave(iv, BASKET_SIZE);;
			correct_ |= ((uint64_t)1 << iv);
		}
	}
}

Node* RegularRegistr::MakeNode(uint16_t level, uint64_t key){
	Node* x = (Node*)HeapAlloc(hp_, HEAP_NO_SERIALIZE, sizeof(Node));
	memset(x, 0, sizeof(Node));
	x->height_ = level;
	x->key_ = key;
	return x;
}

void RegularRegistr::FreeNode(Node* x){
	BOOL ok = HeapFree(hp_, HEAP_NO_SERIALIZE, x);
	BOOST_ASSERT(ok != FALSE);
}

Node* RegularRegistr::Head(){
	return head_;
}

std::pair< void*, uint64_t > RegularRegistr::Write(uint64_t marker){
	auto space = table_[master_];

	uint64_t MASK = ((uint64_t)1 << master_);

	Node *x = nullptr;
	UniMessage um;
	if(um.UnPack(marker)){
		if(x = Contains(marker)){ // select
			SpaceBlock* sb = (SpaceBlock*)x->user_;
			BOOST_ASSERT((sb->writeset_ & MASK) == MASK);

			um.log = ++(sb->log_);
			um.shift = master_;
			Move(x, um.Pack());
		}
		else if(x = (Node*)space->users_[um.pos]){ // by address
			SpaceBlock* sb = (SpaceBlock*)x->user_;
			BOOST_ASSERT(um.shift == master_);
			//			BOOST_ASSERT((sb->writeset_ & MASK) == MASK);

			um.log = ++(sb->log_);
			um.shift = master_;
			Move(x, um.Pack());
		}
		else{
			SpaceBlock *sb = space->at(um.pos);

			um.log = ++(sb->log_);
			um.shift = master_;
			x = Insert(um.Pack());
			x->user_ = sb;

			space->users_[um.pos] = x;
			AlgoN::link_before(head_, x);
		}
	}
	else{ // prev ... r3, r2, r1 | w1, w2, w3, ... next
		um.shift = master_;
		for(uint32_t b = bias_, e = bias_ + BASKET_SIZE; b < e; ++b){
			uint32_t i = b % BASKET_SIZE;
			if(space->users_[i] == nullptr){
				SpaceBlock *sb = space->at(i);

				um.pos = i;
				um.log = sb->log_ + 1;

				if(Ok(um.Pack())){
					sb->log_ = um.log;
					sb->writeset_ |= MASK;

					uint64_t nx = um.Pack();
					x = Insert(nx);
					x->user_ = sb;

					space->users_[i] = x;
					AlgoN::link_before(head_, x);
					bias_ = i + 1;

					break;
				}
			}
		}

		if(x == nullptr){
			x = head_;
			auto total = AlgoN::count(x);
			do{
				if(um.UnPack(x->key_) && (um.shift == master_)){
					SpaceBlock* sb = (SpaceBlock*)x->user_;

					BOOST_ASSERT((sb->writeset_ & MASK) == MASK);

					um.log = ++(sb->log_);
					Move(x, um.Pack());
					break;
				}
				else
					x = head_->next_;
			}
			while(x != head_);
		}

		memset(x->user_, 0, SpaceBlock::BUFFER_SIZE);
	}

	AlgoN::unlink(x);
	AlgoN::link_before(head_, x);

	return std::make_pair(x->user_, x->key_);
}

std::pair< void*, uint64_t > RegularRegistr::Read(uint64_t marker){
	UniMessage um;
	if(um.UnPack(marker)){
		auto space = table_[um.shift];
		Node* y = (Node*)(space->users_[um.pos]);
		if(y == nullptr){
			y = Insert(um.Pack());
			y->user_ = space->at(um.pos);
			space->users_[um.pos] = y;

			AlgoN::link_before(head_, y);
		}
		else if(y->key_ < marker){
			Move(y, marker);
		}

		uint64_t MASK = ((uint64_t)1 << um.shift);
		SpaceBlock *sb = table_[master_]->at(um.pos);
		sb->writeset_ |= MASK;
		return std::make_pair(y->user_, y->key_);
	}
	else
		return std::make_pair(nullptr, marker);
}

void* RegularRegistr::at(uint64_t marker){
	Node* x = Contains(marker);
	return (x) ? x->user_ : nullptr;
}

bool RegularRegistr::is_owner(uint64_t marker) const{
	UniMessage uni;
	if(uni.UnPack(marker))
		return (uni.shift == master_) ? true : false;
	else
		return false;
}

bool RegularRegistr::Free(uint64_t marker){
	Node* x = Contains(marker);
	if(x){
		UniMessage um;
		if(um.UnPack(marker)){
			uint64_t MASK = ((uint64_t)1 << um.shift);
			auto block = table_[master_]->at(um.pos);
			if((block->writeset_ & MASK) == MASK)
				block->writeset_ ^= MASK;

			BOOST_ASSERT(x == table_[um.shift]->users_[um.pos]);
			table_[um.shift]->users_[um.pos] = nullptr;
		}

		AlgoN::unlink(x);
		Delete(marker);

		return true;
	}
	else
		return false;
}

uint32_t RegularRegistr::Capacity(){
	return SpaceBlock::BUFFER_SIZE;
}

uint64_t RegularRegistr::bytes() const{
	return table_[master_]->bytes();
}

bool RegularRegistr::Ok(uint64_t marker){
	UniMessage um;
	if(um.UnPack(marker)){
		if(um.shift == master_){ // write
			uint64_t writeset = 0;
			for(SharedSpace* space : table_){ // FixMe: performance ...
				if(space){
					SpaceBlock* block = space->at(um.pos);
					if((block->writeset_ & MASTER_MASK) != MASTER_MASK){
						writeset |= ((uint64_t)1 << space->SHIFT);
					}
				}
			}

			if((correct_ & writeset) == correct_)
				return true;
		}
		else{ // read
			Node* x = Contains(marker);
			if(x && (((SpaceBlock*)x->user_)->log_ == um.log))
				return true;
		}
	}

	return false;
}

void RegularRegistr::Print() const{
	std::cout << "Master(" << std::setw(3) << master_ << ")" << std::endl;

	Node* n = head_->forward_[0];
	UniMessage uni;
	while(n){
		uni.UnPack(n->key_);
		std::stringstream ss;

		ss << "N[" << std::hex << std::setw(10) << n->key_ << std::dec << "]";
		ss << std::setw(3) << uni.shift;
		ss << std::setw(3) << uni.pos;
		ss << " lg " << std::setw(10) << uni.log;
		ss << " | " << std::setw(10) << std::hex << n->writeset2_ << std::dec;

		if(n->user_){
			SpaceBlock* block = (SpaceBlock*)(n->user_);
			ss << " || " << std::setw(10) << std::hex << block->writeset_ << std::dec;
		}

		std::cout << ss.str() << std::endl;

		n = n->forward_[0];
	}
}

void RegularRegistr::Statistica(){
	//std::cout << "(" << std::setw(3) << vx_count_ << ")" << std::endl;
}

} // mem
} // akva
