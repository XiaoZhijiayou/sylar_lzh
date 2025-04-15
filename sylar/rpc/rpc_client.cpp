#include "rpc_client.h"
#include "sylar/log.h"

namespace sylar {

static sylar::Logger::ptr g_rpclogger = SYLAR_LOG_NEAME("system");

namespace rpc {

RpcClient::RpcClient(const IPAddress::ptr& serverAddr, const std::string& name)
    : m_name(name)
    , m_connect(true)          // 先初始化 m_connect
    , m_retry(false)           // 再初始化 m_retry
    , m_serverAddr(serverAddr) // 然后是 m_serverAddr
    , m_connectionCallback()   // 再初始化 m_connectionCallback
    , m_messageCallback()      // 最后初始化 m_messageCallback
{
    m_sock = Socket::CreateTCPSocket();
}


RpcClient::~RpcClient() {
    if(m_sock) {
        // m_sock->cancelAll();
        m_sock->close();
    }
}

void RpcClient::connect() {
    SYLAR_LOG_INFO(g_rpclogger) << "RpcClient[" << m_name.c_str() << "] - connecting to " << m_serverAddr->getAddr()->sa_data;
    if(m_sock->connect(m_serverAddr)) {
        m_connect = true;
        SYLAR_LOG_INFO(g_rpclogger) << "RpcClient connect succ";
    } else {
        SYLAR_LOG_ERROR(g_rpclogger) << "RpcClient connect failed";
        m_connect = false;
        // getCond().notify_one();
        return;
    }

    auto self = shared_from_this();
    // auto sock = getMySock();

    IOManager::GetThis()->addEvent(m_sock->getSocket(), IOManager::READ, [self]() {
        // std::cout << "hahahahahahahahahaha";
        self->getMessageCallback()(self->getMySock());
        // std::cout << "hahahahahahahahahaha";
        self->getCond().notify_one();
    });

    m_connectionCallback(m_sock);
}

void RpcClient::close() {
    // m_sock->cancelAll();
    m_sock->close();
}

}

}