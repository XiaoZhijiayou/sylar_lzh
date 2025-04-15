#include "../user.pb.h"
#include "../../../sylar/rpc/rpc_channelimpl.h"
#include "../../../sylar/rpc/rpc_controllerimpl.h"
#include "../../../sylar/sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run() {
    fixbug::UserServiceRpc_Stub stub(new sylar::rpc::RpcChannelImpl);
    fixbug::LoginRequest request;
    request.set_name("li");
    request.set_pwd("1");
    fixbug::LoginResponse response;
    // RPC 同步调用
    sylar::rpc::RpcControllerImpl controller;
    for(int i = 0; i < 1000; ++i) {
        stub.Login(&controller, &request, &response, nullptr);

        if(controller.Failed()) {
            SYLAR_LOG_ERROR(g_logger) << controller.ErrorText();
        } else {
            // RPC 调用完成 读取结果
            if(response.result().errcode() == 0) {
                SYLAR_LOG_INFO(g_logger) << "rpc login response: " << response.success();
                std::string test_name_4 = response.test_name_4();
                std::string test_name_1 = response.test_name_1();
                SYLAR_LOG_INFO(g_logger) << "rpc test_name: " << test_name_1 << " " << test_name_1.size(); 
                SYLAR_LOG_INFO(g_logger) << "rpc test_name: " << test_name_4 << " " << test_name_4.size(); 
            } else {
                SYLAR_LOG_ERROR(g_logger) << "failed to rpc login: " << response.result().errmsg();
            }
        }
    }
}

int main() {
    sylar::IOManager iom(3);
    iom.schedule(run);
    // iom.stop();
    // run();
    return 0;
}