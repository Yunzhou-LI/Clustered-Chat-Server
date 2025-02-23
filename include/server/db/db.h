#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <string>
using namespace std;

// 数据库操作类
class MySQL
{
public:
    // 初始化数据库连接
    MySQL();
    // 释放数据库连接资源这里用UserModel示例，通过UserModel如何对业务层封装底层数据库的操作。代码示例如下：
    ~MySQL();
    // 连接数据库
    bool connect();
    // 更新操作
    bool update(string sql);
    // 查询操作
    MYSQL_RES *query(string sql);
    //获取mysql连接
    MYSQL* getConnection();
private:
    MYSQL* _conn;
};
 // class UserModel
// {
// public:
//     // 重写add接口方法，实现增加用户操作
//     bool add(UserDO &user)
//     {
//         // 组织sql语句
//         char sql[1024] = {0};
//         sprintf(sql, "insert into user(name,password,state) values('%s', '%s', '%s')", user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());

//         MySQL mysql;
//          if (mysql.connect())
//         {
//            if (mysql.update(sql))
//             {
//                 LOG_INFO << "add User success => sql:" << sql;
//                 return true;
//             }
//         }
//         LOG_INFO << "add User error => sql:" << sql;
//         return false;
//     };
// };

#endif