#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"

// 群主用户，因为多了一个grouprole字段，所以不直接使用user，而是继承user类封装成groupuser
class GroupUser : public User
{
public:
    void setRole(string role){this->role = role;}
    string getRole(){return this->role;}
private:
    string role;
};
#endif