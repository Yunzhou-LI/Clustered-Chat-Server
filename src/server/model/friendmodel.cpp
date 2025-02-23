#include "friendmodel.hpp"
#include "db.h"

// 添加好友关系
void FriendModel::insert(int userid, int friendid)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values(%d,%d)", userid, friendid);
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
    return;
}

// 返回用户好友列表
vector<User> FriendModel::query(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.name,a.state from user a inner join friend b on b.friend_id = a.id where b.user_id=%d", userid);
    MySQL mysql;
    vector<User> vec;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            // 把userid的所有离线消息放入vec中
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
               User user;
               user.setid(atoi(row[0]));
               user.setname(row[1]);
               user.setstate(row[2]);
               vec.push_back(user);
            }
        }
        // 释放资源res
        mysql_free_result(res);
    }
    return vec;
}
