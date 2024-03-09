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

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 61000
#define CLIENT_PORT 60000
#define BUF_SIZE 8172

int main() {
    WSADATA wsa;
    SOCKET cliSock;
    struct sockaddr_in serverAddr;
    char buffer[BUF_SIZE];

    WSAStartup(MAKEWORD(2, 2), &wsa);
    cliSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.S_un.S_addr = inet_addr(SERVER_IP);

    while (true) {
        string fileName;
        cout << "Enter file name: ";
        cin >> fileName;
        ifstream file("./source/" + fileName, ios::binary);

        if (!file.is_open()) {
            cerr << "Could not open file: " << fileName << endl;
            continue;
        }

        while (!file.eof()) {
            file.read(buffer, BUF_SIZE);
            int readBytes = file.gcount();
            sendto(cliSock, buffer, readBytes, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
            cout << "Packet sent: " << readBytes << " bytes" << endl;
        }

        file.close();
        cout << "File transfer completed for: " << fileName << endl;
    }

    closesocket(cliSock);
    WSACleanup();
    return 0;
}
