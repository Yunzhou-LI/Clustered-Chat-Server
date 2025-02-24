# Clustered-Chat-Server
## 1.项目介绍：  
1.基于muduo网络库实现的可以工作在nginx tcp负载均衡环境中的集群聊天服务器。  
2.使用基于订阅-发布的redis中间件  
3.使用json来打包数据  
4.使用nginx实现tcp长连接的负载均衡，可在nginx.conf文件中更改tcp负载均衡配置  
## 2.项目运行方式  
1.克隆到本地后，根据requirements.txt首先完成项目所需依赖配置  
2.运行自动编译脚本autobuild.sh，编译好的文件放在build文件夹下  
3.切换到bin目录，运行服务端可执行文件chatserver 默认需要两个参数：ip和port  
4.运行客户端可执行文件chatclient 默认需要两个参数：ip和port  
### 运行示例：  
``` c++ 
./autobuild.sh
```
![](/img/img1.png)

进入到bin目录，运行服务端可执行文件。

``` c++ 
cd bin
./chatserver 127.0.0.1 6000
```

![](/img/img2.png)  

新开一个Linux终端，进入到项目的bin目录下，运行聊天客户端可执行文件。  
``` c++ 
./chatclient 127.0.0.1 8000
```
![](/img/img3.png) 
