#include "sylar/http/http_server.h"
#include "sylar/log.h"


static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();



void run(){
  sylar::http::HttpServer::ptr  server(new sylar::http::HttpServer);
  sylar::Address::ptr addr = sylar::Address::LookupAnyIPAddress("192.168.137.192:8020");
  while(!server->bind(addr)){
    sleep(2);
  }
  server->start();
}



int main(int argc,char** argv){
  sylar::IOManager iom(2);
  iom.schedule(run);
  return 0;
}