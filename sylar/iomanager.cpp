#include "iomanager.h"
#include "macro.h"
#include "log.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <unistd.h>

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NEAME("system");

IOManager::FdContext::EventContext&  IOManager::FdContext::getContext(IOManager::Event event){
  switch (event) {
    case IOManager::READ:
      return read;
    case IOManager::WRITE:
      return write;
    default:
      SYLAR_ASSERT2(false,"getContext");
  }
  throw std::invalid_argument("getContext invalid event");
}

void IOManager::FdContext::resetContext(EventContext& ctx){
  ctx.scheduler = nullptr;
  ctx.fiber.reset();
  ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event){
    SYLAR_ASSERT(events & event);

    events = (Event)(events & ~event);        ///取反
    EventContext& ctx = getContext(event);
    if(ctx.cb){
      ctx.scheduler->schedule(&ctx.cb);
    }else {
      ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
    return;
}

IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
  : Scheduler(threads,use_caller,name){
  m_epfd = epoll_create(5000);
  SYLAR_ASSERT(m_epfd > 0);

  int rt = pipe(m_trickleFds);
  SYLAR_ASSERT(rt);

  epoll_event event;
  memset(&event,0,sizeof(epoll_event));
  event.events = EPOLLIN | EPOLLET;
  event.data.fd = m_trickleFds[0];

  rt = fcntl(m_trickleFds[0],F_SETFL,O_NONBLOCK);
  SYLAR_ASSERT(rt);

  rt = epoll_ctl(m_epfd,EPOLL_CTL_ADD, m_trickleFds[0],&event);
  SYLAR_ASSERT(rt);
  contextResize(32);
  Scheduler::Start();
}

IOManager::~IOManager(){
  Stop();
  close(m_epfd);
  close(m_trickleFds[0]);
  close(m_trickleFds[1]);

  for(size_t i = 0; i < m_fdContexts.size(); i++){
    if(m_fdContexts[i]){
      delete m_fdContexts[i];
    }
  }
}

void IOManager::contextResize(size_t size){
  m_fdContexts.resize(size);
  for(size_t i = 0; i < m_fdContexts.size(); i++){
      if(!m_fdContexts[i]){
          m_fdContexts[i] = new FdContext;
          m_fdContexts[i]->fd = i;
      }
  }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb){
  FdContext* fd_ctx = nullptr;
  RWMutexType::ReadLock lock(m_mutex);
  if(m_fdContexts.size() > fd){
    fd_ctx = m_fdContexts[fd];
    lock.unlock();
  }else{
    lock.unlock();
    RWMutexType::WriteLock lock2(m_mutex);
    contextResize(fd * 1.5);
    fd_ctx = m_fdContexts[fd];
  }
  FdContext::MutexType::Lock lock2(fd_ctx->mutex);
  if(fd_ctx->events & event){
    SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                    << "event = " << event
                    << " fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->events;
    SYLAR_ASSERT(!(fd_ctx->events & event));
  }

  int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
  epoll_event epevent;
  epevent.events = EPOLLET | fd_ctx->events | event;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd,op,fd,&epevent);
  if(rt){
    SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << "):"
          << op << "," << fd << "," << epevent.events << "):"
          << rt << "(" << errno << ") (" << strerror(errno) << ")";
    return -1;
  }
  ++m_pendingEventCount;
  fd_ctx->events = (Event)(fd_ctx->events | event);
  FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
  SYLAR_ASSERT(!event_ctx.scheduler
               && !event_ctx.fiber
               && !event_ctx.cb);
  event_ctx.scheduler = Scheduler::GetThis();
  if(cb){
    event_ctx.cb.swap(cb);
  } else{
    event_ctx.fiber = Fiber::GetThis();
    SYLAR_ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
  }
  return 0;
}

bool IOManager::delEvent(int fd, Event event){
  RWMutexType::ReadLock lock(m_mutex);
  if(m_fdContexts.size() <= fd){
    return false;
  }
  FdContext* fd_ctx = m_fdContexts[fd];
  lock.unlock();
  FdContext::MutexType::Lock lock2(fd_ctx->mutex);
  if(!(fd_ctx->events & event)){
    return false;
  }
  Event new_events = (Event)(fd_ctx->events & ~event);
  int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events = EPOLLET | new_events;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd,op,fd,&epevent);
  if(rt){
    SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << "):"
                              << op << "," << fd << "," << epevent.events << "):"
                              << rt << "(" << errno << ") (" << strerror(errno) << ")";
    return false;
  }
  --m_pendingEventCount;
  fd_ctx->events = new_events;
  FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
  fd_ctx->resetContext(event_ctx);
  return true;
}

bool IOManager::cancelEvent(int fd, Event event){
  RWMutexType::ReadLock lock(m_mutex);
  if(m_fdContexts.size() <= fd){
    return false;
  }
  FdContext* fd_ctx = m_fdContexts[fd];
  lock.unlock();
  FdContext::MutexType::Lock lock2(fd_ctx->mutex);
  if(!(fd_ctx->events & event)){
    return false;
  }
  Event new_events = (Event)(fd_ctx->events & ~event);
  int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events = EPOLLET | new_events;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd,op,fd,&epevent);
  if(rt){
    SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << "):"
                              << op << "," << fd << "," << epevent.events << "):"
                              << rt << "(" << errno << ") (" << strerror(errno) << ")";
    return false;
  }
  fd_ctx->triggerEvent(event);
  --m_pendingEventCount;
  return true;
}

bool IOManager::cacelAll(int fd){
  RWMutexType::ReadLock lock(m_mutex);
  if((int)m_fdContexts.size() <= fd){
    return false;
  }
  FdContext* fd_ctx = m_fdContexts[fd];
  lock.unlock();
  FdContext::MutexType::Lock lock2(fd_ctx->mutex);
  if(fd_ctx->events){
    return false;
  }
  int op = EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events = 0;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd,op,fd,&epevent);
  if(rt){
    SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << "):"
                              << op << "," << fd << "," << epevent.events << "):"
                              << rt << "(" << errno << ") (" << strerror(errno) << ")";
    return false;
  }
  if(fd_ctx->events & READ){
    fd_ctx->triggerEvent(READ);
    --m_pendingEventCount;
  }
  if(fd_ctx->events & WRITE){
    fd_ctx->triggerEvent(WRITE);
    --m_pendingEventCount;
  }
  SYLAR_ASSERT(fd_ctx->events == 0);
  return true;
}


IOManager*  IOManager::GetThis(){
  return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

void IOManager::trickle(){
  if(!hasIdleThreads()){
    return;
  }
  int rt = write(m_trickleFds[1],"T",1);
  SYLAR_ASSERT(rt == 1);
}

bool IOManager::stopping(){

}
void IOManager::idle(){

}

}