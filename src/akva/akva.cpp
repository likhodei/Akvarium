#include "akva.hpp"
#include "manager.hpp"

#include <boost/bind.hpp>

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace akva;

// +++ Pipeline +++

Pipeline::Pipeline()
: local_(10, 4096)
{ }

action_sh_t Pipeline::Service(Manager *const mngr, action_sh_t act, uint32_t tick){
	return action_sh_t();
}

action_sh_t Pipeline::Play(Manager *const mngr, action_sh_t act, mail::ptr_t m){
	return action_sh_t();
}

void Pipeline::Complite(Manager *const mngr, mail::ptr_t m){
    auto i = holds_.find(m->hash_value());
    if(i != holds_.end()){
        action_sh_t w = i->second;
        if(w)
            w->Complite(mngr, this, m);

        holds_.erase(i);
    }
}

void Pipeline::statistica(std::stringstream& ss){
	ss << std::setw(5) << tag() << " | ";// << std::setw(5) << mails_.size();
}

buffer_ptr_t Pipeline::MakeData(){
	return new Buffer(&local_);
}

bool Pipeline::filter(mail::ptr_t m){
    if(white_.empty() || (white_.find(m->hdr()->group) != white_.end())){
        if(black_.find(m->hdr()->magic) == black_.end()){
			return true;
        }
    }

	return false;
}

mail::ptr_t Pipeline::Broadcast(Manager *const mngr, mail::type_t type, uint16_t spec, const std::list< message_t >& msgs){
	mail::ptr_t mail = mngr->Pack(type, spec, msgs);
	mail->hdr()->magic = magic();
	mngr->Broadcast(mail, true); // !!!!!!
	return mail;
}

mail::ptr_t Pipeline::WaitAnnonce(mail::ptr_t m, action_sh_t act){
	if(act)
		holds_[m->hash_value()] = act;

	return m;
}

mail::ptr_t Pipeline::Broadcast(Manager *const mngr, mail::type_t type, uint16_t spec, const std::list< message_t >& msgs, action_sh_t act){
	mail::ptr_t m = Broadcast(mngr, type, spec, msgs);
	return WaitAnnonce(m, act);
}

Pipeline *const Pipeline::white(uint16_t group){
	white_.insert(group);
	return this;
}

Pipeline *const Pipeline::black(uint16_t magic){
	black_.insert(magic);
	return this;
}

// +++ Pipe +++

Pipe::Pipe(Manager *const gm, uint16_t key)
: hNp_(NULL), gm_(gm), ppid_(0), corresponding_(false), accepted_(false), id_(0), writer_(this, gm->kseed()){
	guard_.init();
	std::string name = std::string("\\\\.\\pipe\\akva-" + std::to_string(key));
	std::cout << "RUN - [" << name  << "]" << std::endl;

	hNp_ = CreateNamedPipe(name.c_str(),
						   PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
						   PIPE_READMODE_MESSAGE | PIPE_TYPE_MESSAGE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
						   1, 0, 0, NMPWAIT_USE_DEFAULT_WAIT, NULL);

	if(hNp_ == INVALID_HANDLE_VALUE){
		BOOST_ASSERT_MSG(hNp_ != NULL, std::to_string(GetLastError()).c_str()); // ToDo: ERROR_ACCESS_DENIED
	}
	else{
		boost::system::error_code ec;
		boost::asio::use_service< boost::asio::detail::io_context_impl >(gm_->io()).register_handle(hNp_, ec);
		if(ec){
			std::cout << " error : " << ec.message() << std::endl;
		}
	}

	Manager::app()->Attach(this);
}

Pipe::~Pipe(){
	Manager::app()->Detach(this);
	if(hNp_){
		FlushFileBuffers(hNp_);
		DisconnectNamedPipe(hNp_);
		CloseHandle(hNp_);
		hNp_ = NULL;
	}
}

uint16_t Pipe::magic() const
{ return 0; }

std::string Pipe::tag() const{
	return "P";
}

bool Pipe::accepted() const{
	return accepted_;
}

buffer_ptr_t Pipe::Aquare(){
	return MakeData();
}

void Pipe::Direct(buffer_ptr_t b){
    boost::lock_guard< engine::MicroSpinLock > lk(guard_);
    rbuf_.push_back(b);
}

void Pipe::Accept(){
	accepted_ = true;
	boost::asio::windows::overlapped_ptr overlapped{gm_->io(), boost::bind(&Pipe::Handle, this, boost::asio::placeholders::error)};

	auto connected = ConnectNamedPipe(hNp_, overlapped.get());
	if(connected){
		boost::system::error_code ec(GetLastError(), boost::asio::error::get_system_category());
		std::cout << __FUNCTION__ << " con failed  " << ec.message() << std::endl;
	}
	else{
		auto rv = GetLastError();
		switch(rv){
		case ERROR_IO_PENDING: // The overlapped connection in progress.
			overlapped.release();
			break;
		case ERROR_PIPE_CONNECTED:{ // Client is already connected
			boost::system::error_code ec(rv, boost::asio::error::get_system_category());
			overlapped.complete(ec, 0);
			std::cout << __FUNCTION__ << " pipe-con " << ec.message() << std::endl;
			break;
		} // case
		default: { // Fast-fail (synchronous).
			boost::system::error_code ec(rv, boost::asio::error::get_system_category());
			std::cout << __FUNCTION__ << " fast-fail " << ec.message() << std::endl;
			accepted_ = false;
		} // default
		} // switch
	}
}

void Pipe::Handle(const boost::system::error_code& ec){
	ULONG ppid = 0;
    if(ec && (ec.value() != ERROR_PIPE_CONNECTED)){
		std::cout << __FUNCTION__ << " ec : " << ec.message() << std::endl;
		accepted_ = false;
    }
	else if(GetNamedPipeClientProcessId(hNp_, &ppid) == TRUE){
		ppid_ = ppid;
        id_ = 0;

		auto magics = gm_->reg()->MagicsByPID(ppid);
		for(auto magic : magics){
			id_ = magic;
			gm_->Edge(id_, true, true);
		}
	}
}

bool Pipe::connected() const{
	return ((ppid_ > 0) && (id_ != 0)) ? true : false;
}

void Pipe::Reject(const std::string& reason){
	ppid_ = 0;
	id_ = 0;

	accepted_ = false;
	FlushFileBuffers(hNp_);
	DisconnectNamedPipe(hNp_);
}

void Pipe::AsyncWrite(){
	buffer_ptr_t rb;
	{ // safe
		boost::lock_guard< engine::MicroSpinLock > lk(guard_);
		if(corresponding_ || rbuf_.empty())
			return;
		else{
			rb = rbuf_.front(); rbuf_.pop_front();
			corresponding_ = true;
		}
	}

	boost::asio::windows::overlapped_ptr overlapped{ gm_->io(), boost::bind(&Pipe::HandleWrite, this, boost::asio::placeholders::error, rb) };

	BOOL ok = WriteFile(hNp_, rb->rd_ptr(), rb->length(), NULL, overlapped.get());
	auto lerr = GetLastError();

	if(!ok && lerr != ERROR_IO_PENDING){
	    // The operation completed immediately, so a completion notification needs
	    // to be posted. When complete() is called, ownership of the OVERLAPPED-
	    // derived object passes to the io_context.

	    boost::system::error_code ec(lerr, boost::asio::error::get_system_category());
		overlapped.complete(ec, 0);

		{ // safe
            boost::lock_guard< engine::MicroSpinLock > lk(guard_);
			corresponding_ = false;
		}

		std::cout << __FUNCTION__ << " error " << ec.message() << std::endl;
	}
	else{
	    // The operation was successfully initiated, so ownership of the
	    // OVERLAPPED-derived object has passed to the io_context.
		overlapped.release();
	}
}

void Pipe::HandleWrite(const boost::system::error_code& ec, buffer_ptr_t rb){
    { // safe
        boost::lock_guard< engine::MicroSpinLock > lk(guard_);
        corresponding_ = false;
    }

	if(ec){
		Reject(ec.message());
	}
	else if(connected()){
		AsyncWrite();
	}
}

action_sh_t Pipe::Play(Manager *const mngr, action_sh_t act, mail::ptr_t m){
    if(m && m->ok()){
        if(engine::Regedit::IxByMagic(m->hdr()->magic) == gm_->kseed()){ //!!! We posting only self mails.
            writer_.Write(42, m->data(), m->bytes());
        }
    }

    if(gm_->connected() && !writer_.abuf_.empty()){
        boost::lock_guard< engine::MicroSpinLock > lk(guard_);
		while(rbuf_.empty() || (writer_.abuf_.size() > 1)){
            rbuf_.push_back(writer_.abuf_.front());
            writer_.abuf_.pop_front();
        }
    }

    AsyncWrite();
	return action_sh_t();
}

// +++ Tube +++

Tube::Tube(Manager *const manager)
: gm_(manager), hNp_(NULL), id_(0){
	Manager::app()->Attach(this);

    buf_.resize(MAX_FRAME_SIZE * MAX_LINK_COUNT);
	for(auto i(0); i < MAX_LINK_COUNT; ++i){
		physical_[i] = new engine::AkvaReader(&buf_[i * MAX_FRAME_SIZE], MAX_FRAME_SIZE);
	}
}

Tube::~Tube(){
	for(auto& link : physical_){
		if(link){
			delete link;
			link = nullptr;
		}
	}

	Manager::app()->Detach(this);
	if(hNp_){
		FlushFileBuffers(hNp_);
		DisconnectNamedPipe(hNp_);
		CloseHandle(hNp_);
		hNp_ = NULL;
	}
}

buffer_ptr_t Tube::Aquare(){
	return MakeData();
}

uint16_t Tube::magic() const
{ return 0; }

std::string Tube::tag() const{
	return "T";
}

bool Tube::Run(uint16_t id){
    std::string name = std::string("\\\\.\\pipe\\akva-" + std::to_string(id));
    if(WaitNamedPipe(name.c_str(), 50) == TRUE){
        hNp_ = CreateFile(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

        if(hNp_ != INVALID_HANDLE_VALUE){
            boost::system::error_code ec;
            boost::asio::use_service< boost::asio::detail::io_context_impl >(gm_->io()).register_handle(hNp_, ec);

            if(!ec){
                id_ = id;
                gm_->Edge(id, true, false);

                AsyncRead();
            }
            else{
                std::cout << __FUNCTION__ << " [" << name << "] ec : " << ec.message() << std::endl;
            }
        }
    }
    else{
        gm_->Edge(id, false, false);
        std::cout << "INVALID addr[" << name << "]." << std::endl;
    }

    return connected();
}

bool Tube::connected() const{
	return (id_ != 0) ? true : false;
}

void Tube::AsyncRead(){
	buffer_ptr_t wb = Aquare();

	boost::asio::windows::overlapped_ptr overlapped{ gm_->io(), boost::bind(&Tube::HandleRead, shared_from_this(),
		boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, wb) };

	BOOL ok = ReadFile(hNp_, wb->wr_ptr(), wb->space(), NULL, overlapped.get());
	auto lerr = GetLastError();

	if(!ok && lerr != ERROR_IO_PENDING){
	    // The operation completed immediately, so a completion notification needs
	    // to be posted. When complete() is called, ownership of the OVERLAPPED-
	    // derived object passes to the io_context.
	    boost::system::error_code ec(lerr, boost::asio::error::get_system_category());
		overlapped.complete(ec, 0);

		gm_->Edge(id_, false, false, ec.message());
		id_ = 0;
	}
	else{
	    // The operation was successfully initiated, so ownership of the
	    // OVERLAPPED-derived object has passed to the io_context.
		overlapped.release();
	}
}

void Tube::HandleRead(const boost::system::error_code& ec, size_t transferred, buffer_ptr_t wb){
	if(ec){
        gm_->Edge(id_, false, false, ec.message());
		id_ = 0;
	}
	else{
        wb->set_offset_wr_ptr(transferred);

		if(transferred > 0){
            auto qos = engine::AkvaReader::QoS(wb->rd_ptr());
            if(qos != gm_->kseed()){
                gm_->Direct(wb);
            }

            for(auto s = Head(physical_[qos], wb); s != nullptr; s = Next(physical_[qos], wb)){
                if(s->lose_){
					std::cerr << "Frame LOST [" << qos << "] seq: " << s->counter() << std::endl;
                }

                if(s->Complited()){
                    mail::ptr_t m = boost::make_shared< mail::Mail >((uint32_t *const)physical_[qos]->buf(), s->hdr_.len >> 2);
                    if(m->ok()){
                        uint64_t users = m->Visit(gm_->MAGIC);
                        gm_->Refresh(users);
						gm_->Broadcast(m);
                    }
                }
            }
		}

		AsyncRead();
/*
		if(Available(d.point_.src) && Available(d.point_.dst)){
            rbuf_.push_back(wbuf_);
            wbuf_ = impl_->Aquare();
		}
*/
/*
		{
			std::stringstream ss;
			ss << " + " << std::hex << MAGIC << " + ";
			ss << " <>> " << d.point_.src << " -> " << d.point_.dst  << std::dec << " <<> | ";
			ss << len << (used ? " + ok" : "");
            std::cout << ss.str() << std::endl;
		}
//*/
	}
}
