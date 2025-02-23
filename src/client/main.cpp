#include "user.hpp"
#include "group.hpp"
#include "public.hpp"
#include "json.hpp"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>

#include <atomic>
#include <thread>
#include <iostream>
#include <chrono>
#include <ctime>

using namespace std;
using json = nlohmann::json;

// 记录当前登陆系统的用户信息
User g_currentUser;
// 记录当前登陆用户的好友列表信息
vector<User> g_currentUserFriendList;
// 记录当前登陆用户的群组列表信息
vector<Group> g_currentUserGroupList;
// 主菜单页面状态控制变量
bool isMainMenuRunning = false;
// 用于读写线程之间的通信
sem_t rwsem;
// 记录用户登陆状态
atomic_bool g_isLoginSuccess{false};

// 显示当前登陆成功的用户信息
void showCurrentUserData();
// 接收线程
void readTaskHandler(int clientfd);
// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime();
// 主聊天页面程序
void mainMenu(int clientfd);

// 聊天客户端实现
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

    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    // 填写client需要连接的server信息ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // client和server进行连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(0);
    }

    // 初始化用于读写线程通信用的信号量
    sem_init(&rwsem, 0, 0); // 第二个参数表示信号量用于进程还是线程，0表示线程；第三个参数是rwsem的初始值

    // 连接成功，启动一个子线程，专门用于接收数据
    std::thread readTask(readTaskHandler, clientfd);
    readTask.detach();

    // main线程用于接收用户的输入，负责发送数据
    for (;;)
    {
        // 显示首页面菜单，登录，注册，退出
        cout << "======================================================" << endl;
        cout << "1.login" << endl;
        cout << "2.register" << endl;
        cout << "3.exit" << endl;
        cout << "======================================================" << endl;
        cout << "choice:";
        int choice = 0;
        cin >> choice;
        cin.get(); // 读掉缓冲区残留的回车

        switch (choice)
        {
        case 1: // 登录
        {
            int id = 0;
            char password[50] = {0};
            cout << "userid:";
            cin >> id;
            cin.get(); // 读掉缓冲区残留的回车
            cout << "password:";
            cin.getline(password, 50);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = password;

            string request = js.dump();
            g_isLoginSuccess = false;
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (-1 == len)
            {
                cerr << "send login message error" << endl;
            }
            // 等待信号量，由接收线程处理完登录的响应消息后，通知这里
            sem_wait(&rwsem);
            if (g_isLoginSuccess)
            {
                // 进入聊天主界面
                isMainMenuRunning = true;
                mainMenu(clientfd);
            }
            break;
        }
        case 2: // 注册
        {
            char name[50] = {0};
            char password[50] = {0};
            cout << "username:";
            cin.getline(name, 50); // getline遇到回车才会退出   若这里使用cin的话，遇到空格就会退出，导致用户名中无法包含空格
            cout << "password:";
            cin.getline(password, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = password;
            string request = js.dump();
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send reg msg error:" << request << endl;
            }
            sem_wait(&rwsem);//等待信号量，子线程处理完会通知

            break;
        }
        case 3: // 退出
            close(clientfd);
            sem_destroy(&rwsem);
            exit(0);
        default:
            cerr << "invalid input" << endl;
            break;
        }
    }
}
// 系统支持的客户端命令列表
unordered_map<string, string> commandMap =
    {
        {"help", "显示所有支持的命令，格式help"},
        {"chat", "一对一聊天,格式chat:friendid:message"},
        {"addfriend", "添加好友，格式addfriend:friendid"},
        {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
        {"addgroup", "加入群组，格式addgroup:groupid"},
        {"groupchat", "群聊，格式groupchat:groupid:message"},
        {"loginout", "注销，格式loginout"}};
void help(int fd = 0, string str = "");
void chat(int, string);
void addfriend(int, string);
void creategroup(int, string);
void addgroup(int, string);
void groupchat(int, string);
void loginout(int, string);
// 注册系统支持的客户端命令处理
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};
// 主聊天页面程序
void mainMenu(int clientfd)
{
    help();
    char buffer[1024] = {0};
    while (isMainMenuRunning)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer); // 用buffer构造一个string 对象
        string command;            // 存储命令
        int idx = commandbuf.find(":");
        if (-1 == idx)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << "invalid input command" << endl;
            continue;
        }
        // 调用相应命令的事件回调处理，mainMenu对修改封闭，添加新功能不需要修改该函数
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));
    }
}
// 处理登录的响应逻辑
void doLoginResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>())
    {
        cerr << responsejs["errmsg"] << endl;
        g_isLoginSuccess = false;
        return;
    }

    // 登录成功
    // 记录当前用户的信息
    g_currentUser.setid(responsejs["id"].get<int>());
    g_currentUser.setname(responsejs["name"]);

    // 记录当前用户的好友列表信息
    if (responsejs.contains("friends"))
    {
        // 初始化
        g_currentUserFriendList.clear();

        vector<string> vec = responsejs["friends"];
        for (string &str : vec)
        {
            json js = json::parse(str);
            User user;
            user.setid(js["id"].get<int>());
            user.setname(js["name"]);
            user.setstate(js["state"]);
            g_currentUserFriendList.push_back(user);
        }
    }
    // 记录当前用户的群组列表信息
    if (responsejs.contains("groups"))
    {
        // 初始化
        g_currentUserGroupList.clear();

        vector<string> grpV = responsejs["groups"];
        for (string &group : grpV)
        {
            json js = json::parse(group);
            Group grp;
            grp.setid(js["id"].get<int>());
            grp.setname(js["groupname"]);
            grp.setdesc(js["groupdesc"]);
            vector<string> gVec = js["users"];
            for (string &user : gVec)
            {
                json userjs = json::parse(user);
                GroupUser u;
                u.setid(userjs["id"]);
                u.setname(userjs["name"]);
                u.setstate(userjs["state"]);
                u.setRole(userjs["role"]);
                grp.getUsers().push_back(u);
            }
            g_currentUserGroupList.push_back(grp);
        }
    }
    // 显示登陆用户的基本信息
    showCurrentUserData();

    // 显示当前用户的离线信息  （个人聊天信息或群组聊天信息）
    if (responsejs.contains("offlinemsg"))
    {
        vector<string> vec = responsejs["offlinemsg"];
        for (string &str : vec)
        {
            json js = json::parse(str);
            if (ONE_CHAT_MSG == js["msgid"])
            {
                cout << js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
            }
            else
            {
                cout << "来自[" << js["groupid"] << "]" << "群消息：" << js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
            }
        }
    }
    g_isLoginSuccess = true;
}
// 处理注册业务响应逻辑
void doRegResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>()) // 注册失败
    {
        cerr << " is already exist,regsiter error!" << endl;
    }
    else // 注册成功
    {
        cout <<"register success, userid is " << responsejs["id"].get<int>() << ", do not forget it!" << endl;
    }
}
// "help" command handler
void help(int, string)
{
    cout << "<<<   show command list   >>>" << endl;
    for (auto &cp : commandMap)
    {
        cout << cp.first << " : " << cp.second << endl;
    }
    cout << endl;
}
//"chat" command handler
void chat(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "chat command invalid" << endl;
        return;
    }
    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getid();
    js["name"] = g_currentUser.getname();
    js["to"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send chat message error" << buffer << endl;
    }
}
void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = AD_FRIEND_MSG;
    js["id"] = g_currentUser.getid();
    js["friendid"] = friendid;
    string buff = js.dump();

    int len = send(clientfd, buff.c_str(), strlen(buff.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addfriend message error" << buff << endl;
    }
}
void creategroup(int clientfd, string str) // groupname:groupdesc idx=9
{
    int idx = str.find(":");
    if (std::string::npos == idx)
    {
        cerr << "command creategroup invalid" << endl;
        return;
    }
    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);
    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getid();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send creategroup message error" << buffer << endl;
    }
}
void addgroup(int clientfd, string str) // groupid
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getid();
    js["groupid"] = groupid;

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addgroup message error" << buffer << endl;
    }
}

void groupchat(int clientfd, string str) // groupid:message
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "command groupchat invalid" << endl;
        return;
    }
    int groupid = atoi(str.substr(0, idx).c_str());
    string msg = str.substr(idx + 1, str.size() - idx);
    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getid();
    js["name"] = g_currentUser.getname();
    js["groupid"] = groupid;
    js["msg"] = msg;
    js["time"] = getCurrentTime();

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send groupchat message error" << buffer << endl;
    }
}
void loginout(int clientfd, string)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getid();
    string buff = js.dump();
    int len = send(clientfd, buff.c_str(), strlen(buff.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send loginout message error" << buff << endl;
    }
    else
    {
        isMainMenuRunning = false;
    }
}
// 接收线程
void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if (-1 == len || len == 0)
        {
            close(clientfd);
            exit(-1);
        }

        json js = json::parse(buffer);
        int msgType = js["msgid"].get<int>();
        if (ONE_CHAT_MSG == msgType)
        {
            cout << js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        else if (GROUP_CHAT_MSG == msgType)
        {
            cout << "来自[" << js["groupid"] << "]" << "群消息：" << js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        if (msgType == LOGIN_MSG_ACK)
        {
            // 处理登录响应业务的逻辑
            doLoginResponse(js);
            sem_post(&rwsem); // 通知主线程，登陆结果处理完成
            continue;
        }
        if (msgType == REG_MSG_ACK)
        {
            // 处理注册响应业务的逻辑
            doRegResponse(js);
            sem_post(&rwsem);
            continue;
        }
    }
}
// 显示当前登陆成功的用户信息
void showCurrentUserData()
{
    cout << "======================login user======================" << endl;
    cout << "current login user => id:" << g_currentUser.getid() << " name:" << g_currentUser.getname() << endl;
    cout << "----------------------friend list---------------------" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << "  " << user.getid() << "  " << user.getname() << "  " << user.getstate() << endl;
        }
    }
    cout << "----------------------group  list---------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << "  " << group.getid() << "  " << group.getname() << "  " << group.getdesc() << endl;
            for (GroupUser &g_user : group.getUsers())
            {
                cout << "  " << g_user.getid() << "  " << g_user.getname() << "  " << g_user.getstate() << "  " << g_user.getRole() << endl;
            }
        }
    }
    cout << "======================================================" << endl;
}
// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}