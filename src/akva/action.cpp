#include "action.hpp"
#include "manager.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>

namespace akva{

// +++ Action +++

Action::Action()
: one_(false)
{ }

Action::~Action(){
//	std::cout << __FUNCTION__  << std::endl;
}

action_sh_t Action::Play(Manager *const mngr, Pipeline* line, mail::ptr_t m){
	std::cout << "F" << " | " << line->magic() << " -> " << __FUNCTION__ << std::endl;
    return action_sh_t();
}

action_sh_t Action::operator()(Manager *const mngr, Pipeline* line){
	std::cout << line->magic() << " -> " << __FUNCTION__ << std::endl;
    return action_sh_t();
}

void Action::Complite(Manager *const mngr, Pipeline* line, mail::ptr_t m){
	//std::cout << line->magic() << " -> " << __FUNCTION__ << std::endl;
}

uint16_t Action::priority() const
{ return Action::NORMAL; }

namespace cmd{

// +++ Test +++ 

Test::Test(const std::string& msg)
: msg_(msg), count_(0)
{ }

Test::~Test(){
	std::cout << msg_ << " -> " << __FUNCTION__  << std::endl;
}

void Test::Complite(Manager *const mngr, Pipeline* line, mail::ptr_t m){
	std::cout << "C" << " | " << line->magic() << " -> " << __FUNCTION__ << std::endl;
}

action_sh_t Test::Play(Manager *const mngr, Pipeline* line, mail::ptr_t mail){
	std::cout << "F" << " | " << line->magic() << " -> " << __FUNCTION__ << std::endl;
    return action_sh_t();
}

action_sh_t Test::operator()(Manager *const mngr, Pipeline* line){
    if(!one_){
        std::cout << akva::CORE_TEST << " | " << msg_ << " -> " << __FUNCTION__ << std::endl;
        line->Broadcast(mngr, mail::ML_COMMAND, akva::CORE_TEST, {}, shared_from_this());
        one_ = true;
    }
    return action_sh_t();
}

// +++ Info +++

action_sh_t Info::operator()(Manager *const mngr, Pipeline* line){
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
    return action_sh_t();
}

// +++ Service +++

Service::Service(tmark_t tmark)
: tmark_(tmark)
{ }

action_sh_t Service::Play(Manager *const mngr, Pipeline* line, mail::ptr_t m){
    return line->Service(mngr, shared_from_this(), m->hdr()->tmark);
}

void Service::Complite(Manager *const mngr, Pipeline* line, mail::ptr_t m){
    line->Service(mngr, shared_from_this(), m->hdr()->tmark);
}

uint16_t Service::priority() const{
    return Action::HIGH;
}

action_sh_t Service::operator()(Manager *const mngr, Pipeline* line){
    auto delta = (mngr->tiker() - tmark_.first);
    line->Service(mngr, shared_from_this(), tmark_.first);

    if(!one_ && (line->tag() != "P")){ // todo: fixme ... hardcode tag is P
        auto m = line->Broadcast(mngr, mail::ML_TIMESTAMP, 0, {});
        m->hdr()->tmark = tmark_.first;
        m->hdr()->number = tmark_.second;
        line->WaitAnnonce(m, shared_from_this());
        one_ = true;
    }

    return action_sh_t();
}

// +++ Transfer +++

Transfer::Transfer(bool local)
: local_(local)
{ }

uint16_t Transfer::priority() const{
    return Action::NORMAL;
}

action_sh_t Transfer::operator()(Manager *const mngr, Pipeline* line){
    return action_sh_t();
}

action_sh_t Transfer::Play(Manager *const mngr, Pipeline* line, mail::ptr_t m){
    if(local_){
        if((m->hdr()->magic != line->magic()) && line->filter(m))
            line->Play(mngr, shared_from_this(), m);
    }
    else{
        if(engine::Regedit::IxByMagic(m->hdr()->magic) == mngr->kseed())
            line->Complite(mngr, m);
        else{
            switch(m->hdr()->group){
            case mail::ML_METADATA:
            case mail::ML_DATA_ALL:
            case mail::ML_DATA_BEGIN:
            case mail::ML_DATA:
            case mail::ML_DATA_END:{
                if(line->filter(m))
                    line->Play(mngr, shared_from_this(), m);

                break;
            } // case
            case mail::ML_COMMAND:
                line->Play(mngr, shared_from_this(), m);
                break;
            case mail::ML_TIMESTAMP:{
                tmark_t t = std::make_pair(m->hdr()->tmark, m->hdr()->number);
                line->Service(mngr, shared_from_this(), t.first);
                break;
            } // case
            } // switch
        }
    }

    return action_sh_t();
}

// +++ Overwatch +++

Overwatch::Overwatch(tmark_t tmark)
: tmark_(tmark)
{ } 

action_sh_t Overwatch::operator()(Manager *const mngr, Pipeline* line){
    Manager::app()->Overwatch(tmark_, mngr);
    return action_sh_t();
}

} // namespace cmd
} // namespace akva
