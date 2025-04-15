#include "rpc_server.h"
#include "../sylar.h"

namespace sylar {

static sylar::Logger::ptr g_rpclogger = SYLAR_LOG_NEAME("system");

namespace rpc {

void RpcServer::handleClient(Socket::ptr client) {

    SYLAR_LOG_INFO(g_rpclogger) << "RPC handleClient: " << *client;

    auto self = std::static_pointer_cast<RpcServer>(shared_from_this());

    m_ioWorker->addEvent(client->getSocket(), IOManager::READ, [&self, client]() {
        self->getMessageCallback()(client);
    });

    // m_ioWorker->addEvent(client->getSocket(), IOManager::WRITE, [&self, client]() {
    //     client->send("hello", 5);
    // });
}

}

}