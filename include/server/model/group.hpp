#ifndef GROUP_H
#define GROUP_H

#include<string>
#include<vector>
#include "groupuser.hpp"
using namespace std;
// allgroup表的ORM类
class Group
{
public:
    Group(int id = -1,string name = "",string desc = "")
    {
        this->g_id = id;
        this->g_name=name;
        this->g_desc = desc;
    }
    void setid(int id){this->g_id = id;}
    void setname(string name){this->g_name = name;}
    void setdesc(string desc){this->g_desc=desc;}
    int getid(){return this->g_id;}
    string getname(){return this->g_name;}
    string getdesc(){return this->g_desc;}
    vector<GroupUser>& getUsers(){return this->users;}

private:
    int g_id;
    string g_name;
    string g_desc;
    vector<GroupUser> users;

};

#endif