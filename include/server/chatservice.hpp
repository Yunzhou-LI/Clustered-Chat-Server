#ifndef CHATSERVICE_H
#define CHATSERVICE_H
#include<muduo/net/TcpConnection.h>
#include<muduo/base/Logging.h>
#include<unordered_map>
#include<functional>
#include<mutex>
#include "json.hpp"
#include "usermodel.hpp"
#include "groupmodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "redis.hpp"
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace std::placeholders;
using json = nlohmann::json;



//单例模式

//表示处理消息的事件的回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr &conn,json &js,Timestamp time)>;

//聊天服务器业务类
class ChatService
{
public:
    //获取单例对象的接口函数
    static ChatService* instance();
    //处理登陆业务
    void login(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //处理注册业务
    void reg(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //添加好友业务
    void addFriend(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //创建群组业务
    void createGroup(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //添加群组业务
    void addGroup(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //群聊业务
    void groupChat(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //用户注销业务
    void loginout(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //获取回调函数
    MsgHandler getHandler(int msgId);
    //处理客户端异常退出方法
    void clientCloseException(const TcpConnectionPtr &conn);

    //从redis消息队列中获取订阅的消息
    void HandleRedisSubscribeMessage(int userid,string msg);
    
    //服务器异常退出后的重置方法
    void reset();
private:
    ChatService();//单例模式的构造函数私有化    
    
    //存储消息id和其对应的业务处理方法
    unordered_map<int,MsgHandler> _msgHandlerMap;

    //存储在线用户的通信连接     <id,conn>
    unordered_map<int,TcpConnectionPtr> _userConnMap;

    //数据操作类对象
    UserModel _userModel;

    //离线消息对象
    OfflineMsgModel _offLineMsgModel;

    //用户好友对象
    FriendModel _friendModel;

    //群组对象
    GroupModel _groupModel;

    //redis操作对象
    Redis _redis;

    //定义互斥锁 保证对_userConnMap的访问是互斥的
    mutex _connMtx;
};

#endif