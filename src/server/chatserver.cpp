#include "chatserver.hpp"
#include "chatservice.hpp"

using namespace std;
using namespace placeholders;

// 初始化聊天服务器对象
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : _loop(loop), _server(loop, listenAddr, nameArg)
{   
    //注册连接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));
    //注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));
    //设置线程数量
    _server.setThreadNum(4);

}
// 启动服务
void ChatServer::start()
{
    _server.start();
}

// 上报连接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    //客户端连接断开
    if(!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
        
    }
}
// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buff,
                           Timestamp time)
{
    string buf = buff->retrieveAllAsString();
    //数据的反序列化
    json js = json::parse(buf);
    //为了将网络模块和业务代码完全解耦
    //通过js["msg_id"] 获取一个业务handler(回调函数)
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    //回调消息绑定好的处理器，来执行相应的业务处理
    msgHandler(conn,js,time);
}