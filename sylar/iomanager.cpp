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
    SYLAR_LOG_ERROR(g_logger) <<
  }
}

bool IOManager::delEvent(int fd, Event event){

}

bool IOManager::cancelEvent(int fd, Event event){

}

bool IOManager::cacelAll(int fd){

}

IOManager*  IOManager::GetThis(){

}

void IOManager::trickle(){

}
bool IOManager::stopping(){

}
void IOManager::idle(){

}

}