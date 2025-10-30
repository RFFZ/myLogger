#include <iostream>
#include "logger.h"

int main(int argc, char *argv[])
{
    local_time_init();

    Logger::Instance().add_channel(std::shared_ptr<LogChannel>(new LogFileChannel()));
    Logger::Instance().add_channel(std::shared_ptr<LogChannel>(new LogConsoleChannel()));
    Logger::Instance().set_writer(std::shared_ptr<LogWriter>(new LogAsyncWriter()));

    DebugL << "Hello World!";
    InfoL << "Hello World!";
    WarnL << "Hello World!";
    ErrorL << "Hello World!";

    getchar();
    return 0;
}