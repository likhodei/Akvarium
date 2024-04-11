#include "context.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>

#include "manager.hpp"

namespace akva{

// +++ Context +++

Context::Context()
: one_(false)
{ }

Context::~Context(){
//	std::cout << __FUNCTION__  << std::endl;
}

ctx::ptr_t Context::Play(Manager *const mngr, Pipeline* line, graph::ptr_t g){
	std::cout << "F" << " | " << line->magic() << " -> " << __FUNCTION__ << std::endl;
    return ctx::ptr_t();
}

ctx::ptr_t Context::operator()(Manager *const mngr, Pipeline* line){
	std::cout << line->magic() << " -> " << __FUNCTION__ << std::endl;
    return ctx::ptr_t();
}

void Context::Complite(Manager *const mngr, Pipeline* line, graph::ptr_t g){
	//std::cout << line->magic() << " -> " << __FUNCTION__ << std::endl;
}

uint16_t Context::priority() const
{ return Context::NORMAL; }

namespace cmd{

// +++ Test +++ 

Test::Test(const std::string& msg)
: msg_(msg), count_(0)
{ }

Test::~Test(){
	std::cout << msg_ << " -> " << __FUNCTION__  << std::endl;
}

void Test::Complite(Manager *const mngr, Pipeline* line, graph::ptr_t g){
	std::cout << "C" << " | " << line->magic() << " -> " << __FUNCTION__ << std::endl;
}

ctx::ptr_t Test::Play(Manager *const mngr, Pipeline* line, graph::ptr_t g){
	std::cout << "F" << " | " << line->magic() << " -> " << __FUNCTION__ << std::endl;
    return ctx::ptr_t();
}

ctx::ptr_t Test::operator()(Manager *const mngr, Pipeline* line){
    if(!one_){
        std::cout << akva::CORE_TEST << " | " << msg_ << " -> " << __FUNCTION__ << std::endl;
        line->Broadcast(mngr, graph::ML_COMMAND, akva::CORE_TEST, {}, shared_from_this());
        one_ = true;
    }
    return ctx::ptr_t();
}

// +++ Info +++

ctx::ptr_t Info::operator()(Manager *const mngr, Pipeline* line){
    if(!one_){
        std::stringstream ss;
        ss << mngr->reg()->Info();
        ss << "-> leader[" << (Manager::app()->leader() ? "*" : " ") << "] line: ";
        Pipeline* l = line;
        do{
            ss << l->tag() << "->";
            l = l->next_;
        }
        while(line != l);
        ss << std::endl;

        ss << "STATISTICA" << std::endl;
        l = line;
        do{
            l->statistica(ss);
            ss << std::endl;
            l = l->next_;
        }
        while(line != l);

        std::cout << ss.str() << std::endl;
        one_ = true;
    }
    return ctx::ptr_t();
}

// +++ Service +++

Service::Service(tmark_t tmark)
: tmark_(tmark)
{ }

ctx::ptr_t Service::Play(Manager *const mngr, Pipeline* line, graph::ptr_t g){
    return line->Service(mngr, shared_from_this(), g->hdr()->tmark);
}

void Service::Complite(Manager *const mngr, Pipeline* line, graph::ptr_t g){
    line->Service(mngr, shared_from_this(), g->hdr()->tmark);
}

uint16_t Service::priority() const{
    return Context::HIGH;
}

ctx::ptr_t Service::operator()(Manager *const mngr, Pipeline* line){
    auto delta = (mngr->tiker() - tmark_.first);
    line->Service(mngr, shared_from_this(), tmark_.first);

    if(!one_ && (line->tag() != "P")){ // todo: fixme ... hardcode tag is P
        auto m = line->Broadcast(mngr, graph::ML_TIMESTAMP, 0, {});
        m->hdr()->tmark = tmark_.first;
        m->hdr()->number = tmark_.second;
        line->WaitAnnonce(m, shared_from_this());
        one_ = true;
    }

    return ctx::ptr_t();
}

// +++ Transfer +++

Transfer::Transfer(bool local)
: local_(local)
{ }

uint16_t Transfer::priority() const{
    return Context::NORMAL;
}

ctx::ptr_t Transfer::operator()(Manager *const mngr, Pipeline* line){
    return ctx::ptr_t();
}

ctx::ptr_t Transfer::Play(Manager *const mngr, Pipeline* line, graph::ptr_t m){
    if(local_){
        if((m->hdr()->magic != line->magic()) && line->filter(m))
            line->Play(mngr, shared_from_this(), m);
    }
    else{
        if(engine::Regedit::IxByMagic(m->hdr()->magic) == mngr->kseed())
            line->Complite(mngr, m);
        else{
            switch(m->hdr()->group){
            case graph::ML_METADATA:
            case graph::ML_DATA_ALL:
            case graph::ML_DATA_BEGIN:
            case graph::ML_DATA:
            case graph::ML_DATA_END:{
                if(line->filter(m))
                    line->Play(mngr, shared_from_this(), m);

                break;
            } // case
            case graph::ML_COMMAND:
                line->Play(mngr, shared_from_this(), m);
                break;
            case graph::ML_TIMESTAMP:{
                tmark_t t = std::make_pair(m->hdr()->tmark, m->hdr()->number);
                line->Service(mngr, shared_from_this(), t.first);
                break;
            } // case
            } // switch
        }
    }

    return ctx::ptr_t();
}

// +++ Overwatch +++

Overwatch::Overwatch(tmark_t tmark)
: tmark_(tmark)
{ } 

ctx::ptr_t Overwatch::operator()(Manager *const mngr, Pipeline* line){
    if(!one_){
        Manager::app()->Overwatch(tmark_, mngr);
        one_ = true;
    }
    return ctx::ptr_t();
}

} // namespace cmd
} // namespace akva
