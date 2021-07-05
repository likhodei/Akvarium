#include "protocol.hpp"
#include "block.hpp"

namespace akva{
namespace engine{

// +++ AkvaReader +++

AkvaReader::AkvaReader(uint8_t *buf, uint16_t len, uint16_t qos)
: buf_(buf, len), writed_(0), counter_(0), lose_(false), qos_(qos << 10), b_(nullptr), e_(nullptr)
{ }

uint16_t AkvaReader::QoS(uint8_t *buf){
    BOOST_ASSERT(buf);
    AkvaHdr* h = (AkvaHdr*)buf;
    return (h->counter & 0xfc00) >> 10;
}

bool AkvaReader::Complited() const{
    uint16_t sz = DataFrame::ceil4(hdr_.len) << 2;
    return (writed_ == sz) ? true : false;
}

AkvaReader* AkvaReader::Head(uint32_t *b, uint32_t *e){
    b_ = b;
    e_ = e;

    if(b_ == e_)
        return nullptr;

    AkvaHdr* fh = (AkvaHdr*)b_++;
    if(((fh->counter - counter_) & COUNTER_MASK) > 1){
        writed_ = 0;
        lose_ = true;
    }
    else
        lose_ = false;

    counter_ = fh->counter;

    if(fh->pointer == 0xffff){
        uint16_t sz = (e_ - b_) << 2;
        memcpy(buf_.first + writed_, b_, sz);
        writed_ += sz;

        b_ = e_;
    }
    else if(fh->pointer > 0){
        uint32_t *c = b_ + DataFrame::ceil4(fh->pointer);

        if(c > e_) // data overhead
            c = e_;

        uint16_t sz = (c - b_) << 2;
        memcpy(buf_.first + writed_, b_, sz);
        writed_ += sz;

        b_ = c;
    }
    else{
        writed_ = 0;
        hdr_ = *((AkvaBody*)b_);

        if(hdr_.len > 0){
            b_++;
            uint32_t *c = b_ + DataFrame::ceil4(hdr_.len);

            if(c > e_) // data overhead
                c = e_;

            uint16_t sz = (c - b_) << 2;
            memcpy(buf_.first + writed_, b_, sz);
            writed_ += sz;

            b_ = c;
        }
        else
            return nullptr;
    }

    return this;
}

AkvaReader* AkvaReader::Next(){
    if(b_ == e_)
        return nullptr;

    writed_ = 0;
    hdr_ = *((AkvaBody*)b_);

    if(hdr_.len > 0){
        b_++;
        uint32_t *c = b_ + DataFrame::ceil4(hdr_.len);

        if(c > e_) // data overhead
            c = e_;

        uint16_t sz = (c - b_) << 2;
        memcpy(buf_.first + writed_, b_, sz);
        writed_ += sz;

        b_ = c;
    }
    else
        return nullptr;

    return this;
}

} // engine
} // akva
