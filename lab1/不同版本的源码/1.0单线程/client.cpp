#include <string>
#include <iostream>
#include <cstdio>
#include <WinSock2.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable : 4996)

using namespace std;

int main()
{
    // 初始化 Windows 套接字库（Winsock）,即使后面没有使用，也要写
    WSADATA wsaData; // windows socket api data
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    { // Winsock Startup,需要版本号和data地址  0表示成功，非0表示失败
        // MAKEWORD(2, 2) 是一个宏，它将两个字节大小的参数合并成一个,这里表示用 Winsock 2.2 版本的库
        cerr << "WSAStartup failed." << endl;
        system("pause");
    }

    // 输出 Winsock 版本信息,例如Winsock version: 514.514
    // std::cout << "Winsock version: " << wsaData.wVersion << "." << wsaData.wHighVersion << std::endl;

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0); // AF_INET 地址家族 表示ipv4，流式套接字，0表示协议自动选择
    //对于TCP套接字（SOCK_STREAM），操作系统将自动选择TCP协议。对于UDP套接字（SOCK_DGRAM），操作系统将自动选择UDP协议。
    if (clientSocket == INVALID_SOCKET)
    { // INVALID_SOCKET表示一个数值，(SOCKET)(~0),也就是-1，如果是无效的
        cerr << "Failed to create socket." << endl;
        WSACleanup(); // windows socket api 清除Winsock库所占用的资源
        system("pause");
    }

    sockaddr_in serverAddr;                              // 接口地址 internet,对应着服务器地址
    serverAddr.sin_family = AF_INET;                     // socket info ipv4
    serverAddr.sin_port = htons(12345);                  // 服务器端口12345
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 设置服务器IP地址，由于服务器是本电脑，所以只能是127.0.0.1

    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    { // 根据服务器地址，客户端接口，建立连接
        // 建立成功返回0，否则是一个数字对应error
        cerr << "Failed to connect to the server." << endl;
        closesocket(clientSocket); // 关闭失败的连接
        WSACleanup();
        system("pause");
    }

    cout << "Connected to the server." << endl;

    char message[1024]; // 用户可以发的信息是有限的

    while (true)
    { // 反复让用户输入信息，提示quit退出
        cout << "Enter a message to send to the server (or type 'quit' to exit): ";
        cin.getline(message, sizeof(message)); // 读取一行

        if (strcmp(message, "quit") == 0)
        { // 如果读取到的message是quit，退出
            break;
        }
        // 否则就是输入了要发送的信息，通过send可以发送给服务端
        int sendResult = send(clientSocket, message, strlen(message), 0); // 需要客户端接口，要发的信息，长度，额外操作
        if (sendResult == SOCKET_ERROR)
        { // 如果发送成功
            cerr << "Send failed." << endl;
            break;
        }
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
