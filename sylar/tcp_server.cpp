#include "tcp_server.h"
#include "config.h"
#include "log.h"

namespace sylar {

static sylar::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
    sylar::Config::Lookup("tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2),
                          "tcp server read timeout");

static sylar::Logger::ptr g_logger = SYLAR_LOG_NEAME("system");

TcpServer::TcpServer(sylar::IOManager* worker, sylar::IOManager* accept_worker)
    : m_worker(worker),
      m_acceptWorker(accept_worker),
      m_recvTimeout(g_tcp_server_read_timeout->getValue()),
      m_name("sylar/1.0.0"),
      m_type("tcp"),
      m_isStop(true) {
  std::cout << "------------------- TcpServer()----------------------------\n";
}

TcpServer::~TcpServer() {
  std::cout << "------------------- "
               "TcpServer::~TcpServer()----------------------------\n";
  for (auto& i : m_socks) {
    i->close();
  }
  m_socks.clear();
}

bool TcpServer::bind(sylar::Address::ptr addr) {
  std::vector<Address::ptr> addrs;
  std::vector<Address::ptr> fails;
  addrs.push_back(addr);
  return bind(addrs, fails);
}

bool TcpServer::bind(const std::vector<Address::ptr>& addrs,
                     std::vector<Address::ptr>& fails) {
  for (auto& addr : addrs) {
    Socket::ptr sock = Socket::CreateTCP(addr);
    if (!sock->bind(addr)) {
      SYLAR_LOG_ERROR(g_logger)
          << "bind fail errno = " << errno << "errstr = " << strerror(errno)
          << "addr=[" << addr->toString() << "]";
      fails.push_back(addr);
      continue;
    }
    if (!sock->listen()) {
      SYLAR_LOG_ERROR(g_logger)
          << "listen fail errno = " << errno << "errstr = " << strerror(errno)
          << "addr=[" << addr->toString() << "]";
      fails.push_back(addr);
      continue;
    }
    m_socks.push_back(sock);
  }
  if (!fails.empty()) {
    m_socks.clear();
    return false;
  }
  for (auto& i : m_socks) {
    SYLAR_LOG_INFO(g_logger) << "type = " << m_type << " name = " << m_name
                             << " server bind success : " << *i;
  }
  return true;
}

bool TcpServer::start() {
  if (!m_isStop) {
    return true;
  }
  m_isStop = false;
  for (auto& sock : m_socks) {
    m_acceptWorker->schedule(
        std::bind(&TcpServer::startAccept, shared_from_this(), sock));
  }
  return true;
}

/// 这里面使用this和self一起用就是捕获this不足以保护TcpServer对象不被销毁，this可能成为一个空悬指针
/// 捕获this和self能更好的保护对象在函数执行期间不被销毁
void TcpServer::stop() {
  m_isStop = true;
  auto self = shared_from_this();
  m_acceptWorker->schedule([this, self]() {
    for (auto& sock : m_socks) {
      sock->cancelAll();
      sock->close();
    }
    m_socks.clear();
  });
}

void TcpServer::handleClient(Socket::ptr client) {
  SYLAR_LOG_INFO(g_logger) << "handleClient: " << *client;
}

void TcpServer::startAccept(Socket::ptr sock) {
  while (!m_isStop) {
    Socket::ptr Client = sock->accept();
    if (Client) {
      Client->setRecvTimeout(m_recvTimeout);
      m_worker->schedule(
          std::bind(&TcpServer::handleClient, shared_from_this(), Client));
    } else {
      SYLAR_LOG_ERROR(g_logger)
          << " accept errno" << errno << " errstr = " << strerror(errno);
    }
  }
}

std::string TcpServer::tostring(const std::string& prefix) {
  std::stringstream ss;
  ss << prefix << "[type=" << m_type << " name = " << m_name
     << " worker = " << (m_worker ? m_worker->getName() : "")
     << " accept= " << (m_acceptWorker ? m_acceptWorker->getName() : "")
     << " recv_timeout = " << m_recvTimeout << "]" << std::endl;
  std::string pfx = prefix.empty() ? "    " : prefix;
  for (auto& i : m_socks) {
    ss << pfx << pfx << *i << std::endl;
  }
  return ss.str();
}

}  // namespace sylar