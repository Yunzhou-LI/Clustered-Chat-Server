#ifndef PUBLIC_H
#define PUBLIC_H

/*
service和client的公共文件
*/
enum  EnMsgType
{
    LOGIN_MSG = 1,  //登录msg
    LOGIN_MSG_ACK,  //登录响应
    LOGINOUT_MSG,   //用户注销
    REG_MSG, //注册msg
    REG_MSG_ACK, //注册响应
    ONE_CHAT_MSG,//一对一聊天
    AD_FRIEND_MSG,//添加好友

    CREATE_GROUP_MSG,//创建群组
    ADD_GROUP_MSG,//加入群组
    GROUP_CHAT_MSG,//群聊天
};

#endif