#include "graph.hpp"

#include <boost/functional/hash.hpp>

#include "regedit.hpp"

using namespace graph;

// +++ Graph +++

Graph::Graph(uint16_t group, type_t type)
 : h_(nullptr), d_(nullptr), words_(HDR_SIZE, 0){
	h_ = (Hdr*)words_.data();
	d_ = (uint64_t*)(words_.data() + HDR_SIZE);

	h_->group = group;
	h_->type = type;
}

Graph::Graph(uint32_t *const body, uint16_t words)
: h_(nullptr), d_(nullptr){
	words_.reserve(HDR_SIZE);
	Copy(body, words);
}

void Graph::Copy(uint32_t *const body, uint16_t words){
	std::copy(body, body + words, std::back_inserter(words_));
	if(words_.size() >= HDR_SIZE){
		h_ = (Hdr*)words_.data();
		d_ = (uint64_t*)(words_.data() + HDR_SIZE);
	}
}

Hdr *const Graph::hdr() const{
	BOOST_ASSERT(h_);
	return (Hdr *const)h_;
}

size_t Graph::hash_value() const{
	size_t seed = h_->magic;
	boost::hash_combine(seed, h_->type);
	boost::hash_combine(seed, h_->group);
	boost::hash_combine(seed, h_->magic);
	boost::hash_combine(seed, h_->number);

	for(uint64_t *b = d_, *e = d_ + h_->count; b != e ; ++b){
		boost::hash_combine(seed, *b);
	}

	return seed;
}

uint64_t Graph::Visit(uint16_t magic){
	h_->users |= ((uint64_t)1 << akva::engine::Regedit::IxByMagic(magic));
	return h_->users;
}

uint16_t Graph::users() const{
#if WIN64
	return __popcnt64(h_->users); // ToDo: win assert
#else
	return __popcnt((uint32_t)h_->users); // trim more 32 
#endif	
}

bool Graph::is_token() const{
	return (h_->count == 0) ? true : false;
}

bool Graph::ok() const{
	if(h_ && (h_->magic != 0)){
		return (words_.size() == (HDR_SIZE + 2 * h_->count)) ? true : false;
	}
	else
		return false;
}

uint8_t *const Graph::data() const{
	return (uint8_t *const)words_.data();
}

uint16_t Graph::bytes() const{
	return words_.size() << 2;
}
