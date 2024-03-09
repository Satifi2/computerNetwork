#include <string>
#include <iostream>
#include <cstdio>
#include <WinSock2.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable : 4996)

using namespace std;

int main(){
    WSADATA wsaData; // windows socket api data,2.2的库，这里一样,即使后面不用，必须要初始化
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0){
        cerr << "WSAStartup failed." << endl;
        system("pause");
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0); // 服务端接口，ipv4地址，流式套接字[已经实现],协议自动选择
    if (serverSocket == INVALID_SOCKET)
    { // 接口创建失败
        cerr << "Failed to create socket." << endl;
        WSACleanup();
        system("pause");
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
        system("pause");
    }

    if (listen(serverSocket, 5) == SOCKET_ERROR)
    { // 排队的客户端只能有5个
        cerr << "Listen failed." << endl;
        closesocket(serverSocket);
        WSACleanup();
        system("pause");
    }

    // 获取服务端的IP地址并打印,通常是0.0.0.0:12345,因为服务器绑定到了所有可用的网络接口上,端口是12345
    cout << "Server is listening on " << inet_ntoa(serverAddr.sin_addr) << ":" << ntohs(serverAddr.sin_port) << endl;

    SOCKET clientSocket;    // 客户端接口
    sockaddr_in clientAddr; // 客户端地址
    int clientAddrLen = sizeof(clientAddr);
    // accept 函数在服务器端一直监听客户端的连接请求,在此期间阻塞程序直到连接成功，当有客户端连接请求到达时，accept 函数会自动创建一个新的接口返回
    // 此时服务端和这个客户端建立了连接,可以直接通信了
    // 客户端连接的时候，不需要手动填写地址，接收连接请求的时候，客户端地址会被自动填充
    clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen); // 服务器接口和客户端地址
    if (clientSocket == INVALID_SOCKET)
    { // 创建失败处理
        cerr << "Accept failed." << endl;
        closesocket(serverSocket);
        WSACleanup();
        system("pause");
    }

    // 获取客户端的IP地址并打印,通常是127.0.0.1:53901这样
    cout << "Client connected from " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << endl;

    char buffer[1024]; // 缓冲区，接收客户端信息
    int recvResult;    // 收到的字符个数

    while (true){
        recvResult = recv(clientSocket, buffer, sizeof(buffer), 0); // 接收客户端接口的信息，结果写入buffer，无特殊标志
        if (recvResult > 0){
            buffer[recvResult] = '\0'; // 字符串最后一位的串尾符
            cout << "Client " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << " sent to server: " << buffer << endl;
        }
        else if (recvResult == 0)
        { // 没有信息，表示关闭
            //但是：TCP会周期性地检测连接是否仍然存在。如果客户端突然关闭了，服务器可能需要等待一段时间才能检测到连接的断开，
            cout << "Client disconnected." << endl;
            break;
        }
        else
        { // 其他是错误
            cerr << "Recv failed." << endl;
            // break;
        }
        cout << "wating for next message..." << endl;
    }

    closesocket(clientSocket); // 所有接口关闭，资源释放
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
