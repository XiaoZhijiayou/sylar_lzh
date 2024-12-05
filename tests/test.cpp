#include <iostream>
#include "../sylar/log.h"

int main(int argc, char* argv[]){
    sylar::Logger::ptr logger(new sylar::Logger);
    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender));
    sylar::LogEvent::ptr event(new sylar::LogEvent(__FILE__, __LINE__, 0,1,2,time(0)));
    event->getSS() << "hello sylar log ";
    logger->log(sylar::LogLevel::DEBUG, event);
    return 0;
}