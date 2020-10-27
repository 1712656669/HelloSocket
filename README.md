# C++百万并发网络通信引擎架构与实现
## Windows中编译的注意事项
VS2019更改调试属性  
输出目录：$(SolutionDir)../bin/$(Platform)/$(Configuration)\  
中间目录：$(SolutionDir)../temp/$(Platform)/$(Configuration)/$(ProjectName)\  
## Linux环境编译命令
服务器：  
g++ server.cpp -std=c++11 -pthread -o server  
./server  
客户端：  
g++ client.cpp -std=c++11 -pthread -o client  
./client  
