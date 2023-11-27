#include "protocol.h"

WSADATA wsa;
SOCKET serverSocket;
struct sockaddr_in serverAddr, remoteAddr;
Packet sentPacket, receivedPacket;
int ACKNum = 0, remoteAddrSize = sizeof(remoteAddr);
char fileName[256];
FILE* outFile;

void init (){
    WSAStartup(MAKEWORD(2, 2), &wsa);
    serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);
    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
}

int main() {
    printReceiver(),init();
    cout<<"Enter filename: ";
    cin>>fileName;
    outFile = fopen(fileName, "wb");
    do {
        recvfrom(serverSocket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, &remoteAddrSize);
    } while (!validateChecksum(&receivedPacket) || receivedPacket.flags != SYN || receivedPacket.seqNum != 0);
    ACKNum += receivedPacket.dataLen;
    sentPacket = Packet(0, ACKNum, 1, SYN | ACK, ".");
    sendto(serverSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, remoteAddrSize);
    while (true) {
        recvfrom(serverSocket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, &remoteAddrSize);
        if (!validateChecksum(&receivedPacket) || receivedPacket.flags & ACK == 0 || ACKNum != receivedPacket.seqNum) {
            continue;
        }
        ACKNum += receivedPacket.dataLen;
        if (receivedPacket.flags == (FIN | ACK)) {
            sentPacket = Packet(receivedPacket.ackNum, ACKNum, 1, ACK, ".");
            sendto(serverSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, remoteAddrSize);
            break;
        }
        else {
            fwrite(receivedPacket.message, 1, receivedPacket.dataLen, outFile);
            sentPacket = Packet(receivedPacket.ackNum, ACKNum, 1, ACK, ".");
            sendto(serverSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, remoteAddrSize);
        }
    }
    cout << "File transfer completed." << endl;
    fclose(outFile),system("pause");
    return 0;
}