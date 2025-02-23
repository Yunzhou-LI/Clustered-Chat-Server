#include "chatserver.hpp"
#include<iostream>
#include "chatservice.hpp"
#include<signal.h>
using namespace std;

//处理服务器被ctrl+c强制结束后，获取这个信号，来更改user表中的state状态，避免user的state永远为online，影响后续登录
void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid！ example:./chatserver 127.0.0.1 6000" << endl;
        exit(-1);
    }
    // 解析通过命令参数传递的ip和client
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    signal(SIGINT,resetHandler);
    EventLoop loop;
    InetAddress addr(ip,port);
    ChatServer server(&loop,addr,"chatserver");
    server.start();
    loop.loop();

    return 0;
}