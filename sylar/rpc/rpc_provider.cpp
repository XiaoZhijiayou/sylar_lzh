#include "rpc_provider.h"
#include "rpcheader.pb.h"
#include <string>

#include <google/protobuf/descriptor.h>
#include "rpc_server.h"

namespace sylar {

static sylar::Logger::ptr g_rpclogger = SYLAR_LOG_NEAME("rpc");

namespace rpc {

using namespace google::protobuf;

RpcProvider::RpcProvider() {}

RpcProvider::~RpcProvider() {
    m_zkClient.close();
}

void RpcProvider::notifyService(Service* service) {
    // 获取服务对象的描述信息
    const ServiceDescriptor* serviceDesc = service->GetDescriptor();

    // 获取服务名称
    std::string serviceName = std::string(serviceDesc->name());
    SYLAR_LOG_INFO(g_rpclogger) << "方法名称 " << serviceName; 
    // 获取服务对象的方法数量
    int methondCnt = serviceDesc->method_count();
    SYLAR_LOG_INFO(g_rpclogger) << "方法数量 " << methondCnt; 
    ServiceInfo info;
    for (int i = 0; i < methondCnt; ++i) {
        // 获取服务对象指定下标的服务方法的描述
        const MethodDescriptor* methodDesc = serviceDesc->method(i);
        std::string             methodName = std::string(methodDesc->name());
        info.methodMap.insert({ methodName, methodDesc });
    }
    info.service = service;
    m_serviceMap.insert({ serviceName, info });

    SYLAR_LOG_INFO(g_rpclogger) << m_serviceMap.size() 
        << " " << m_serviceMap.find(serviceName)->second.methodMap.find(std::string(serviceDesc->method(0)->name()))->first;

    SYLAR_LOG_INFO(g_rpclogger) << "publish servic: [" << serviceName.c_str() << "][" << methondCnt << "][" << serviceDesc->method(0)->name();
}

void RpcProvider::run() {
    IPAddress::ptr addr = sylar::IPAddress::Create("127.0.0.1", 2181);
    
    RpcServer::ptr server(new RpcServer);
    server->bind(addr);

    // sleep(2);

    SYLAR_LOG_INFO(g_rpclogger) << "开始绑定回调";
    server->setConnectionCallback(std::bind(&RpcProvider::onConnection,
        this, std::placeholders::_1));

    server->setMessageCallback(std::bind(&RpcProvider::onMessage,
        this, std::placeholders::_1));
    
    SYLAR_LOG_INFO(g_rpclogger) << "绑定回调结束";

    // ZkClientRPC::ptr zkCli = std::make_shared<ZkClientRPC>();
    m_zkClient.start();
    SYLAR_LOG_INFO(g_rpclogger) << "zkclient 开始";
    // 将当前RPC节点上要发布的服务全部注册到zookeeper中 让RpcClient可以在zookeeper上发现服务
    std::string new_path;
    new_path.resize(128);
    for(auto& service : m_serviceMap) {
        std::string service_path = "/" + service.first;
        m_zkClient.create(service_path, "", 0);
        for(auto& method : service.second.methodMap) {
            std::string method_path = service_path + "/" + method.first;
            SYLAR_LOG_INFO(g_rpclogger) << "********************************";
            std::string method_data = "127.0.0.1:8000";

            m_zkClient.create(method_path.c_str(), method_data, ZOO_EPHEMERAL);
        }
    }

    SYLAR_LOG_INFO(g_rpclogger) << "RpcProvider start service at 127.0.0.1:8000";

    server->start();
}

void RpcProvider::onConnection(const Socket::ptr& conn) {
    if(!conn->isConnected()) {
        conn->close();
    }
}

void RpcProvider::onMessage(const Socket::ptr& conn) {
    SocketStream ss(conn);
    std::string message;
    message.resize(128);
    ss.read(&message[0], 128);

    SYLAR_LOG_INFO(g_rpclogger) << message;

    // 读取前4个字节的内容
    uint32_t headerSize = 0;
    message.copy(reinterpret_cast<char*>(&headerSize), 4, 0);

    // 根据header_size读取数据头的原始字符流
    std::string content = message.substr(4, headerSize);

    // 反序列化数据得到RPC请求的详细信息
    RpcHeader rpcHeader;
    std::string serviceName, methodName;
    uint32_t argsSize = 0;
    if(rpcHeader.ParseFromString(content)) {
        serviceName = rpcHeader.service_name();
        methodName = rpcHeader.method_name();
        argsSize = rpcHeader.args_size();
    } else {
        SYLAR_LOG_ERROR(g_rpclogger) << "failed to parse rpc header: " << content.c_str();
        return;
    }

    std::string argsStr = message.substr(4 + headerSize, argsSize);

    SYLAR_LOG_DEBUG(g_rpclogger) << "receive rpc header: "
                                << headerSize << " " 
                                << content.c_str() << " "
                                << serviceName.c_str() << " "
                                << methodName.c_str() << " "
                                << argsSize << " "
                                << argsStr.c_str();
    
    // 获取 service 对象和 method 对象
    SYLAR_LOG_ERROR(g_rpclogger) << m_serviceMap.size();
    auto iter = m_serviceMap.find(serviceName);
    if(iter == m_serviceMap.end()) {
        SYLAR_LOG_ERROR(g_rpclogger) << serviceName.c_str() << " is not exist";
        return;
    }

    auto mt_iter = iter->second.methodMap.find(methodName);
    if(mt_iter == iter->second.methodMap.end()) {
        SYLAR_LOG_ERROR(g_rpclogger) << serviceName.c_str()
                                << ":" << methodName.c_str()
                                << " is not exist";
        return;
    }

    Service* service = iter->second.service;
    const MethodDescriptor* methodDesc = mt_iter->second;

    // 生成RPC方法调用的请求和响应
    Message* request = service->GetRequestPrototype(methodDesc).New();
    if(!request->ParseFromString(argsStr)) {
        SYLAR_LOG_ERROR(g_rpclogger) << "request parse error: " << argsStr.c_str();
        return;
    }

    // 根据远端RPC请求 调用当前RPC节点上发布的方法
    Message* response = service->GetResponsePrototype(methodDesc).New();
    Closure* done = NewCallback<RpcProvider,
        const Socket::ptr&, Message*>(this, &RpcProvider::sendRpcResponse, conn, response);
    service->CallMethod(methodDesc, nullptr, request, response, done);
}

void RpcProvider::sendRpcResponse(const Socket::ptr& conn, Message* response) {
    SYLAR_LOG_INFO(g_rpclogger) << "send rpc response";

    std::string responseStr;
    if(response->SerializeToString(&responseStr)) {
        SYLAR_LOG_ERROR(g_rpclogger) << responseStr << " " << responseStr.size();
        // 通过网络将RPC方法执行的结果发送回RPC的调用方
        size_t responSize = responseStr.size();

        std::string packet;
        packet.reserve(4 + responSize);
        char sizeBuffer[4];
        sizeBuffer[0] = (responSize >> 24) & 0xFF;
        sizeBuffer[1] = (responSize >> 16) & 0xFF;
        sizeBuffer[2] = (responSize >>  8) & 0xFF;
        sizeBuffer[3] = responSize & 0xFF;

        packet.append(reinterpret_cast<char*>(&sizeBuffer), 4);
        packet.append(responseStr);
        conn->send(packet.c_str(), packet.size());
    } else {
        SYLAR_LOG_ERROR(g_rpclogger) << "failed to serial string";
    }
    conn->close();
}

}

}