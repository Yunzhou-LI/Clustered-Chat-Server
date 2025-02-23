#include "chatservice.hpp"
#include "public.hpp"

#include<string>
#include<vector>
using namespace std;

//获取单例对象的接口函数
ChatService* ChatService::instance()
{
    static ChatService service;//为什么静态的对象？？  单例对象
    return &service;
}
//注册消息以及对应的Handler回调操作
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG,std::bind(&ChatService::login,this,_1,_2,_3)});
    _msgHandlerMap.insert({LOGINOUT_MSG,bind(&ChatService::loginout,this,_1,_2,_3)});
    _msgHandlerMap.insert({REG_MSG,std::bind(&ChatService::reg,this,_1,_2,_3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG,std::bind(&ChatService::oneChat,this,_1,_2,_3)});
    _msgHandlerMap.insert({AD_FRIEND_MSG,std::bind(&ChatService::addFriend,this,_1,_2,_3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG,bind(&ChatService::createGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG,bind(&ChatService::addGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG,bind(&ChatService::groupChat,this,_1,_2,_3)});

    //连接redis服务器
    if(_redis.connect())
    {
        //设置上报消息的回调
        _redis.init_notify_handler(bind(&ChatService::HandleRedisSubscribeMessage,this,_1,_2));
    }
}
//获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgId)
{
    //记录错误日志，msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgId);
    if(it == _msgHandlerMap.end())
    {
        //调用muduo库自带的日志输出    自带换行
        //LOG_ERROR<<"msgid:"<<msgId<<"can not find a handler";
        //返回一个默认的处理器 空操作
        return [=](const TcpConnectionPtr &conn,json &js,Timestamp)
        {
            LOG_ERROR<<"msgid:"<<msgId<<" can not find a handler";
        };
    }
    else
    {
        return _msgHandlerMap[msgId];
    }
    
}
//处理登陆业务
/*
     ORM框架    object relation map 对象关系映射  
     解决了什么问题：方便业务层和数据层分离
     怎么解决：业务层操作的都是对象  数据层操作数据库 
*/ 
void ChatService::login(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];
    User user = _userModel.query(id);
    if (user.getid() == id && user.getpwd() == pwd)
    {
        if(user.getstate() == "online")
        {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;//登录响应
            response["errno"] = 2;//0表示登录成功
            response["errmsg"] = "this account is using ,login another";
            conn->send(response.dump());//将json字符串序列化后发送
        }
        else
        {
            {
                lock_guard<mutex> lck(_connMtx);
                //登陆成功，记录用户连接信息   需注意 该步骤需要时线程安全的
                _userConnMap.insert({id,conn});
            }
            //id用户登录成功后，向redis订阅channel(id)
            _redis.subscribe(id);

            //LOG_INFO<<"登录用户订阅成功！";
            //登陆成功  更新用户状态为online
            user.setstate("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;//登录响应
            response["errno"] = 0;//0表示登录成功
            response["id"] = user.getid();
            response["name"] = user.getname();
           
            //检查该用户是否有离线消息
            vector<string> vec = _offLineMsgModel.query(id);
            if(!vec.empty())
            {
                response["offlinemsg"] = vec;
                _offLineMsgModel.remove(id);
            }

            //查询用户好友列表
            vector<User> userVec = _friendModel.query(id);
            if(!userVec.empty())
            {
                vector<string> v2;
                for(User &user:userVec)
                {
                    json js;
                    js["id"] = user.getid();
                    js["name"] = user.getname();
                    js["state"] = user.getstate();
                    v2.push_back(js.dump());
                }
                response["friends"] = v2; 
            }
            //查询用户的群组信息
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if(!groupuserVec.empty())
            {
                vector<string> groupV;
                for(Group &group:groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getid();
                    grpjson["groupname"] = group.getname();
                    grpjson["groupdesc"] = group.getdesc();
                    vector<string> userV;
                    for(GroupUser &user:group.getUsers())
                    {
                        json js;
                        js["id"] = user.getid();
                        js["name"] = user.getname();
                        js["state"] = user.getstate();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }
                response["groups"] = groupV;
            }
            conn->send(response.dump());//将json字符串序列化后发送
        }
    }
    else
    {
        //登陆失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;//登录响应
        response["errno"] = 1;//0表示成功  1表示失败
        response["errmsg"] = "id or password is incorrect";
        conn->send(response.dump());//将json字符串序列化后发送
    }
}
//处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setname(name);
    user.setpwd(pwd);
    bool status = _userModel.insert(user);
    if(status)
    {   
        //注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;//注册响应
        response["errno"] = 0;//0表示注册成功
        response["id"] = user.getid();
        conn->send(response.dump());//将json字符串序列化后发送

    }
    else
    {
        //注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;//注册响应
        response["errno"] = 1;//0表示注册成功
        conn->send(response.dump());//将json字符串序列化后发送
    }
}
 //用户注销业务
 void ChatService::loginout(const TcpConnectionPtr &conn,json &js,Timestamp time)
 {
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lck(_connMtx);
        auto it = _userConnMap.find(userid);
        if(it!=_userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    //用户注销后，订阅在redis的channel(id)也应该一并取消
    _redis.unsubscribe(userid);

    //更新用户的状态信息
    User user(userid,"","","offline");
    _userModel.updateState(user);
 }
//处理客户端异常退出方法
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        //删除异常断开的连接
        lock_guard<mutex> lck(_connMtx);
        for(unordered_map<int,TcpConnectionPtr>::iterator it = _userConnMap.begin();it != _userConnMap.end();it++)
        {
            if(it->second == conn)
            {
                user.setid(it->first);
                _userConnMap.erase(it);
            }
        }
    }
   
    //更新用户的状态信息
    if(user.getid() != -1)
    {
        user.setstate("offline");
        _userModel.updateState(user);
    }
    //用户注销后，订阅在redis的channel(id)也应该一并取消
    _redis.subscribe(user.getid());
    
}
//一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int toid = js["to"].get<int>();
    {
        lock_guard<mutex> lck(_connMtx);
        auto it=_userConnMap.find(toid);
        if(it != _userConnMap.end())
        {
            //toid在线 且和发送方登录在同一台服务器上 转发消息   服务器主动推送消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }
    //查询toid状态是否在线  因为有可能在其他服务器上登录
    User user = _userModel.query(toid);
    if(user.getstate() == "online")
    {
        _redis.publish(toid,js.dump());
        return;
    }

    //toid不在线，存储到离线消息队列
    _offLineMsgModel.insert(toid,js.dump());
}
//添加好友业务   msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int id = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    _friendModel.insert(id,friendid);
}
//服务器异常退出后的重置方法 
void ChatService::reset()
{
    _userModel.resetState();
}
//创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();
    string groupname = js["groupname"];
    string groupdesc = js["groupdesc"];
    //存储新创建的群组信息
    Group group(-1,groupname,groupdesc);
    if(_groupModel.createGroup(group))
    {
        //存储群组创建人信息
        _groupModel.addGroup(userid,group.getid(),"creator");
    }
}
//添加群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid,groupid,"normal");
}
//群聊业务
void ChatService::groupChat(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid,groupid);
    lock_guard<mutex> lck(_connMtx);
    for(int id:useridVec)
    {
        auto it = _userConnMap.find(id);
        if(it!=_userConnMap.end())//发送方和接收方在同一台服务器登录
        {
            it->second->send(js.dump());
        }
        else
        {
            User user = _userModel.query(id);
            if(user.getstate() == "online")
            {
                _redis.publish(id,js.dump());
            }
            else{
                _offLineMsgModel.insert(id,js.dump()); 
            }
           
        }
    }
}
//从redis消息队列中获取订阅的消息
void ChatService::HandleRedisSubscribeMessage(int userid,string msg)
{
    //用户登录上服务器，服务器订阅用户的id到消息队列，按理说当有消息时，调用这个回调的时候，该用户一定在服务器上
    //因为用户只要在线，就会有一个线程去阻塞等待关于他的消息，但是！！！！ 多线程下考虑用户有可能在有消息来时，正好下线了
    //所以多考虑一步，如果他不在线的话，把消息放到离线消息表中
    lock_guard<mutex> lck(_connMtx);
    auto it = _userConnMap.find(userid);
    if(it!=_userConnMap.end())
    {
        it->second->send(msg);
        return;
    }
    //
    _offLineMsgModel.insert(userid,msg);
}