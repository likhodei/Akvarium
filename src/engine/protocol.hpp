#pragma once
#ifndef ENGINE_PROTOCOL_H_
#define ENGINE_PROTOCOL_H_

#include "block.hpp"
#include "support/simple_block.hpp"

#include <cstdint>
#include <deque>

namespace akva{
#pragma pack(push, 4)
struct AkvaHdr{
	uint16_t counter;
	uint16_t pointer;
};

struct AkvaBody{
	uint16_t len;
	uint16_t spec;
};
#pragma pack(pop)

namespace engine{

// +++ AkvaWriter +++

template< typename allocatorT >
class AkvaWriter{
public:
    enum{
        MAX_COUNTER = 1024,
        COUNTER_MASK = (MAX_COUNTER - 1)
    };

    typedef typename allocatorT::block_ptr_t block_ptr_t;

    AkvaWriter(allocatorT *const allocator, uint16_t qos = 1);
    void Write(uint16_t type, uint8_t* data, uint16_t len);

    std::deque< block_ptr_t > abuf_;

private:
    uint16_t counter_;
    allocatorT *const mem_;
    const uint16_t qos_;
};

// +++ AkvaReader +++

class AkvaReader{
public:
    enum{
        MAX_COUNTER = 1024,
        COUNTER_MASK = (MAX_COUNTER - 1)
    };

    typedef std::pair< uint8_t*, uint16_t > buffer_t;

    AkvaReader(uint8_t *buf, uint16_t len, uint16_t qos = 1);
    bool Complited() const;

    static uint16_t QoS(uint8_t *buf);

    inline uint8_t *const buf(){
        return buf_.first;
    }

    inline uint16_t counter() const{
        return counter_;
    }

    AkvaReader* Head(uint32_t *b, uint32_t *e);
    AkvaReader* Next();

    bool lose_;
    AkvaBody hdr_;

protected:
    buffer_t buf_;
    uint16_t writed_;
    uint16_t counter_;
    uint32_t *b_, *e_;
    const uint16_t qos_;
};

template< class bufferT >
AkvaReader* Head(AkvaReader* r, bufferT df){
    auto o = df->length() % 4;
    if(o > 0)
        return nullptr;

    return r->Head((uint32_t*)df->rd_ptr(), (uint32_t*)(df->rd_ptr() + df->length()));
}

template< class bufferT >
AkvaReader* Next(AkvaReader* r, bufferT df){
    return r->Next();
}

// +++ AkvaWriter +++

template< typename allocatorT >
AkvaWriter< allocatorT >::AkvaWriter(allocatorT *const allocator, uint16_t qos)
: mem_(allocator), counter_(0), qos_(qos << 10)
{ }

template< typename allocatorT >
void AkvaWriter< allocatorT >::Write(uint16_t type, uint8_t* data, uint16_t len){
    AkvaHdr *fh = nullptr;
    if(abuf_.empty() || (abuf_.back()->space() < sizeof(AkvaBody))){
        block_ptr_t f = mem_->Aquare(); abuf_.push_back(f);
        memset(f->wr_ptr(), 0, f->space());

        fh = (AkvaHdr*)f->wr_ptr();
        fh->pointer = 0;
        fh->counter = ++counter_ & COUNTER_MASK;
        fh->counter |= qos_;
        f->set_offset_wr_ptr(sizeof(AkvaHdr));
    }
    else{
        block_ptr_t f = abuf_.back();
        fh = (AkvaHdr*)f->base();
    }

    block_ptr_t f = abuf_.back();
    AkvaBody *fb = (AkvaBody*)f->wr_ptr();
    fb->len = len;
    fb->spec = type;
    f->set_offset_wr_ptr(sizeof(AkvaBody));

    uint16_t bytes = DataFrame::ceil4(len) << 2; // normalize
    uint16_t writed(0);
    while(writed < bytes){
        bool alloc = (f->space() == 0) ? true : false;
        if(alloc){
            f = mem_->Aquare(); abuf_.push_back(f);
            memset(f->wr_ptr(), 0, f->space());

            fh = (AkvaHdr*)f->wr_ptr();
            fh->pointer = 0;
            fh->counter = ++counter_ & COUNTER_MASK;
            fh->counter |= qos_;
            f->set_offset_wr_ptr(sizeof(AkvaHdr));
        }

        uint32_t *b = (uint32_t*)f->wr_ptr();
        uint32_t *e = b + (f->space() >> 2); // move by words

        uint32_t* p = b + ((bytes - writed) >> 2);
        if(p < e){ // end
            if(alloc && (writed > 0)){
                fh->pointer = (bytes - writed);
            }
        }
        else{ // more
            if(alloc && (writed > 0)){
                fh->pointer = 0xffff;
            }
            p = e;
        }

        uint16_t sz = (p - b) << 2;
        f->Copy(data + writed, sz);
        writed += sz;
    }
}

} // engine
} // akva
#endif // ENGINE_PROTOCOL_H_ 

