#include <string>
#include <iostream>
#include <cstdio>
#include <WinSock2.h>
#include <thread>
#include <vector>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable : 4996)
#include <mutex>
using namespace std;
#include "protocol.h"
#include <unordered_map>

vector<SOCKET> clientSockets; // 存储所有客户端套接字
// 创建一个映射关系，将客户端套接字与房间号关联起来
std::unordered_map<SOCKET, int> clientRoomMap;
mutex mtx; // 用于保护客户端套接字列表的互斥锁

    const char* asciiArt[] = {
        "   _____ _           _    _____                          ",
        "  / ____| |         | |  / ____|                         ",
        " | |    | |__   __ _| |_| (___   ___ _ ____   _____ _ __ ",
        " | |    | '_ \\ / _` | __|\\___ \\ / _ \\ '__\\ \\ / / _ \\ '__|",
        " | |____| | | | (_| | |_ ____) |  __/ |   \\ V /  __/ |   ",
        "  \\_____|_| |_|\\__,_|\\__|_____/ \\___|_|    \\_/ \\___|_|   "
    };

void BroadcastMessage(const ChatMessage& message, SOCKET senderSocket ) {//将message发给除了发送者之外的接口
    lock_guard<mutex> lock(mtx);
    
    for (SOCKET clientSocket : clientSockets) {//遍历所有客户端接口
        if (clientSocket != senderSocket && message.roomId == clientRoomMap[clientSocket]) {//不要发给发送者自己，不要发给
            int sendResult = send(clientSocket, (const char*)&message, sizeof(message), 0);//将message发送给这个接口，flags=0没有额外操作
            if (sendResult == SOCKET_ERROR) {//一般不报错
                cerr << "Send failed." << endl;
            }
        }
    }
}

void HandleClient(SOCKET clientSocket,sockaddr_in clientAddr) {
    ChatMessage message;
    int recvResult;//长度
    while (true) {
        recvResult = recv(clientSocket,(char*)&message, sizeof(message), 0);//子线程内阻塞，等待客户端消息，收到后写入buffer，flags=0表示没有额外操作
        if (recvResult == sizeof(ChatMessage)) {//收到消息
            //输出客户端的ip和信息
            cout << "Client " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << " sent to server: " << message.username<<message.message << endl;
            PrintChatMessage(message);
            cout<<std::endl;
            //广播这个消息到其他所有用户
            BroadcastMessage(message, clientSocket);
        } else if (recvResult == 0) {
            // lock_guard<mutex> lock(mtx);
            // clientSockets.erase(remove(clientSockets.begin(), clientSockets.end(), clientSocket), clientSockets.end());
            cout << "Client disconnected." << endl;
            break;
        } else {
            cerr << "Recv failed." << endl;
            break;
        }
    }

    closesocket(clientSocket);
}

int main(){
    WSADATA wsaData; // windows socket api data,2.2的库，这里一样,即使后面不用，必须要初始化
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0){
        cerr << "WSAStartup failed." << endl;
        
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0); // 服务端接口，ipv4地址，流式套接字[已经实现],协议自动选择
    if (serverSocket == INVALID_SOCKET)
    { // 接口创建失败
        cerr << "Failed to create socket." << endl;
        WSACleanup();
        
    }

    sockaddr_in serverAddr;                  // 服务器地址
    serverAddr.sin_family = AF_INET;         // socket info ipv4
    serverAddr.sin_port = htons(12345);      // 服务器端口12345
    serverAddr.sin_addr.s_addr = INADDR_ANY; // 设置服务器IP地址，由于服务器是本电脑，所以只能是127.0.0.1
    // 服务器接口要和地址绑定
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR){
        cerr << "Bind failed." << endl; // 如果失败
        closesocket(serverSocket);      // 关闭接口，清理接口占用
        WSACleanup();
        
    }

    if (listen(serverSocket, 100) == SOCKET_ERROR)
    { // 排队的客户端只能有5个
        cerr << "Listen failed." << endl;
        closesocket(serverSocket);
        WSACleanup();
        
    }


    for (const char* line : asciiArt) {
        std::cout << line << std::endl;
    }

    printTimeInfo();
    // 获取服务端的IP地址并打印,通常是0.0.0.0:12345,因为服务器绑定到了所有可用的网络接口上,端口是12345
    cout << "Server is listening on " << inet_ntoa(serverAddr.sin_addr) << ":" << ntohs(serverAddr.sin_port) << endl;
    
    while (true) {
        SOCKET clientSocket;//客户端接口
        sockaddr_in clientAddr;//客户端地址
        int clientAddrLen = sizeof(clientAddr);//地址长度

        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);//中断，直到一个客户端连接上，之后自动填写客户端地址，返回客户端接口
        int roomNum =0;//读取房间号
        recv(clientSocket,(char*)&(roomNum), sizeof(roomNum), 0);
        // 在接收到客户端房间号后，将客户端套接字和房间号关联
        clientRoomMap[clientSocket] = roomNum;
        cout<<"roomID:"<<clientRoomMap[clientSocket]<<" ";

        if (clientSocket == INVALID_SOCKET) {//连接失败
            cerr << "An accept failed." << endl;
        } else {//否则就是连接上了，可以输出连接到的客户端的ip和端口，注意网络序转小端序
            cout << "Client connected from " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << endl;
            {//由于在使用 std::lock_guard 时，不需要显式地解锁，因为 std::lock_guard 会在离开其作用域时自动解锁互斥量,所以这里加上一个括号
            lock_guard<mutex> lock(mtx);
            clientSockets.push_back(clientSocket);//所有客户端接口需要保留，后面还有用
            }
            //线程创立之后立即执行，但不会阻塞，也就是执行的同时后面的代码继续执行，thread第一个参数是执行的函数名，第二个是函数参数
            thread t(HandleClient, clientSocket,clientAddr);//一旦连接上，开一个线程处理客户端的消息
            t.detach(); // 分离线程，不等待线程结束
        }
    }


    closesocket(serverSocket);//关闭接口，清理占用
    WSACleanup();
    return 0;
}
