#include <string>
#include <iostream>
#include <cstdio>
#include <WinSock2.h>
#include <thread>
#include<vector>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable : 4996)

#include "protocol.h"

using namespace std;

    const char* asciiArt[] = {
        "   _____ _           _   _____                       ",
        "  / ____| |         | | |  __ \\                      ",
        " | |    | |__   __ _| |_| |__) |___   ___  _ __ ___  ",
        " | |    | '_ \\ / _` | __|  _  // _ \\ / _ \\| '_ ` _ \\ ",
        " | |____| | | | (_| | |_| | \\ \\ (_) | (_) | | | | | |",
        "  \\_____|_| |_|\\__,_|\\__|_|  \\_\\___/ \\___/|_| |_| |_|"
    };


ChatMessage message;
vector<ChatMessage> historyMessages;

void renderScreen(){
    system("cls");
    // 输出所有历史消息
    for (const char* line : asciiArt) {
        std::cout << line << std::endl;
    }
    cout<<"             roomID:"<<message.roomId<<endl;
    cout<<"================================="<<endl;

    for (const ChatMessage& msg : historyMessages){
        // 使用 std::string 来方便处理
        std::string timestamp(msg.timestamp);
        // 截取字符串，保留小时和分钟部分
        std::string formattedTimestamp = timestamp.substr(11, 5); 
        cout << msg.username<< (strcmp(msg.username,message.username)==0?("[me]"):"") <<"("<<formattedTimestamp <<"):"<< msg.message << endl;
    }
    // cout<<receivedMessage.username<<" "<<receivedMessage.message<<endl;
    for(int i=1;i<=15-historyMessages.size();i++){//如果消息比较少，用空行补齐
        cout<<std::endl;
    }
    cout<<"================================="<<endl;
    cout<<message.username<<" message>:";
}

void ReceiveMessages(SOCKET clientSocket){
    ChatMessage receivedMessage;
    int recvResult; // 长度

    while (true){
        recvResult = recv(clientSocket, (char *)&receivedMessage, sizeof(receivedMessage), 0); // 子线程等待接收消息，阻塞
        if (recvResult == sizeof(ChatMessage)){
            historyMessages.push_back(receivedMessage);//新增消息
            renderScreen();//渲染屏幕
            recvResult=0;//保证下一次收到信息之前，不会再次输出
        }
        else if (recvResult == 0){
            cout << "Disconnected from the server." << endl;
            break;
        }
        else{
            cerr << "Recv failed." << endl;
            break;
        }
    }
}

int main(){
    // 初始化 Windows 套接字库（Winsock）,即使后面没有使用，也要写
    WSADATA wsaData; // windows socket api data
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    { // Winsock Startup,需要版本号和data地址  0表示成功，非0表示失败
        // MAKEWORD(2, 2) 是一个宏，它将两个字节大小的参数合并成一个,这里表示用 Winsock 2.2 版本的库
        cerr << "WSAStartup failed." << endl;
        
    }

    // 输出 Winsock 版本信息,例如Winsock version: 514.514
    // std::cout << "Winsock version: " << wsaData.wVersion << "." << wsaData.wHighVersion << std::endl;

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0); // AF_INET 地址家族 表示ipv4，流式套接字，0表示协议自动选择
    // 对于TCP套接字（SOCK_STREAM），操作系统将自动选择TCP协议。对于UDP套接字（SOCK_DGRAM），操作系统将自动选择UDP协议。
    if (clientSocket == INVALID_SOCKET)
    { // INVALID_SOCKET表示一个数值，(SOCKET)(~0),也就是-1，如果是无效的
        cerr << "Failed to create socket." << endl;
        WSACleanup(); // windows socket api 清除Winsock库所占用的资源
        
    }

    for (const char* line : asciiArt) {
        std::cout << line << std::endl;
    }

    sockaddr_in serverAddr;                              // 接口地址 internet,对应着服务器地址
    serverAddr.sin_family = AF_INET;                     // socket info ipv4
    serverAddr.sin_port = htons(12345);                  // 服务器端口12345
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 设置服务器IP地址，由于服务器是本电脑，所以只能是127.0.0.1

    printTimeInfo();
    std::cout << "Welcome to the chat system!" << std::endl;

    // 用户输入用户名
    cout << "Please enter your username: ";
    cin.getline(message.username, sizeof(message.username));

    // 用户选择聊天室数字
    cout << "Choose a chat room (0~999, a new one will be created if it doesn't exist): ";

    cin >> message.roomId;
    cin.ignore(); // 忽略输入缓冲中的回车符

    cout<<"chat room loading"<<endl;
    
    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    { // 根据服务器地址，客户端接口，建立连接
        // 建立成功返回0，否则是一个数字对应error
        cerr << "Failed to connect to the server." << endl;
        closesocket(clientSocket); // 关闭失败的连接
        WSACleanup();
    }

    send(clientSocket, (const char*)&(message.roomId), sizeof(int), 0); // 发送房间号给服务器

    cout << "Connected to the server." << endl;

    std::this_thread::sleep_for(std::chrono::seconds(1));

    cout << "Enter a message to send to the server (or type 'quit' to exit): "<<endl;

    std::this_thread::sleep_for(std::chrono::seconds(1));

    thread t(ReceiveMessages,clientSocket);
    while (true)
    {   
        renderScreen();
        // 反复让用户输入信息，提示quit退出
        cin.getline(message.message, MAX_MESSAGE_LEN); // 读取一行,这次是到字符串里面
        
        //消息要最新的时间
        UpdateTimestamp(message);

        if (strcmp(message.message, "quit") == 0)
        { // 如果读取到的message是quit，退出
            break;
        }
        // 否则就是输入了要发送的信息，通过send可以发送给服务端
        int sendResult = send(clientSocket, (const char *)&message, sizeof(ChatMessage), 0); // 需要客户端接口，要发的信息，长度，额外操作
        historyMessages.push_back(message);//自己发的消息也计入
        if (sendResult == SOCKET_ERROR)
        { // 如果发送成功
            cerr << "Send failed." << endl;
            break;
        }
    }
    t.join(); // 等待接收消息的线程结束
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
