#include "protocol.h"

WSADATA wsa;
SOCKET serverSocket;
struct sockaddr_in serverAddr, remoteAddr;
Packet sentPacket, receivedPacket;
int ack = -1, remoteAddrSize = sizeof(remoteAddr);
char fileName[256];
FILE* outFile;

void send() {
    sendto(serverSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, remoteAddrSize);
}

void receive() {
    recvfrom(serverSocket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, &remoteAddrSize);
    // printPacket(receivedPacket, 0);
}

void init() {
    WSAStartup(MAKEWORD(2, 2), &wsa);
    serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);
    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
}

int main() {
    printReceiver(), init();
    cout << "Enter filename: ", cin >> fileName;
    outFile = fopen(fileName, "wb");
    do {
        receive();
    } while (receivedPacket.flags != SYN || receivedPacket.seqNum != ack + 1);
    sentPacket = Packet(0, ++ack, 0, SYN | ACK, "");
    send();
    while (true) {
        receive();
        if (receivedPacket.flags == (FIN | ACK)) {
            sentPacket = Packet(0, ++ack, 0, ACK, "");
            send();
            break;
        }
        if (receivedPacket.seqNum == ack + 1) {
            fwrite(receivedPacket.message, 1, receivedPacket.dataLen, outFile);
            sentPacket = Packet(0, ++ack, 0, ACK, "");
            send();
        }
        else {
            sentPacket = Packet(0, ack, 0, ACK, "");
            send();
        }
    }
    cout << "File transfer completed." << endl;
    fclose(outFile), system("pause");
    return 0;
}