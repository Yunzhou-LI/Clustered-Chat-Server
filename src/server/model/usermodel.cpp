#include "usermodel.hpp"
#include "db.h"
#include <iostream>
using namespace std;
// User表的增加方法
bool UserModel::insert(User &user)
{
    char sql[1024] = {0};
    // 由于用户id设置成自增的，所以这里只用插入其他三条数据
    // 这里get方法返回的是string类型，基于mysql的要求，需要转换为c风格字符串
    sprintf(sql, "insert into user(name,password,state) values('%s','%s','%s')",
            user.getname().c_str(), user.getpwd().c_str(), user.getstate().c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 获取插入成功的用户数据生成的主键id
            user.setid(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}
//根据用户id查询用户信息
User UserModel::query(int id)
{
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id=%d", id);
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                User user;
                user.setid(atoi(row[0]));
                user.setname(row[1]);
                user.setpwd(row[2]);
                user.setstate(row[3]);
                mysql_free_result(res);
                return user;
            }
        }
    }
    return User();
}
//更新用户的状态信息
bool UserModel::updateState(User &user)
{
    char sql[1024] = {0};
    sprintf(sql,"update user set state = '%s' where id = '%d'",user.getstate().c_str(),user.getid());
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}
//重置所有用户状态信息offline
void UserModel::resetState()
{
    char sql[1024] = "update user set state = 'offline'";
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}