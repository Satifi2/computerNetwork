#include<iostream>
#include<WinSock2.h>
#include<winsock.h>
#include<Windows.h>
#include<string>
#include<cstring>
#include<fstream>
#include<io.h>
#include<Mmsystem.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Winmm.lib")
using namespace std;

#define SERVER_PORT 61000
#define BUF_SIZE 8172

int main() {
    WSADATA wsa;
    SOCKET srvSock;
    struct sockaddr_in serverAddr, clientAddr;
    char buffer[BUF_SIZE];
    int clientAddrSize = sizeof(clientAddr);

    WSAStartup(MAKEWORD(2, 2), &wsa);
    srvSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;

    bind(srvSock, (sockaddr*)&serverAddr, sizeof(serverAddr));

    while (true) {
        string fileName = "received_file.txt"; // Assuming one file transfer per session
        ofstream file("./destination/" + fileName, ios::binary | ios::app);

        if (!file.is_open()) {
            cerr << "Could not open file: " << fileName << endl;
            continue;
        }

        int recvBytes;
        if ((recvBytes = recvfrom(srvSock, buffer, BUF_SIZE, 0, (sockaddr*)&clientAddr, &clientAddrSize)) > 0) {
            file.write(buffer, recvBytes);
            cout << "Packet received: " << recvBytes << " bytes" << endl;
        }

        file.close();
        cout << "File received and saved as: " << fileName << endl;
    }

    closesocket(srvSock);
    WSACleanup();
    return 0;
}
