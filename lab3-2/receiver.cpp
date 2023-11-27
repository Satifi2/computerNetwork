#include "protocol.h"

WSADATA wsa;
SOCKET serverSocket;
struct sockaddr_in serverAddr, remoteAddr;
Packet sentPacket, receivedPacket;
int sentPacketCount = 0, ACKNum = 0, timeout = 200, notimeout = -1, remoteAddrSize = sizeof(remoteAddr);
char fileName[256];
FILE* outFile;

int main() {
    printReceiver();
    WSAStartup(MAKEWORD(2, 2), &wsa);
    serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);
    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    cout<<"Enter filename: ";
    cin>>fileName;
    outFile = fopen(fileName, "wb");
    do {
        recvfrom(serverSocket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, &remoteAddrSize);
        printPacket(receivedPacket, false, true);
    } while (!validateChecksum(&receivedPacket) || receivedPacket.flags != SYN || receivedPacket.seqNum != 0);
    ACKNum += receivedPacket.dataLen;

    sentPacket = Packet(0, ACKNum, 1, SYN | ACK, ++sentPacketCount, ".");
    printPacket(sentPacket, true, true);
    sendto(serverSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, remoteAddrSize);

    while (true) {
        recvfrom(serverSocket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, &remoteAddrSize);
        printPacket(receivedPacket, false, false);
        if (!validateChecksum(&receivedPacket) || receivedPacket.flags & ACK == 0 || ACKNum != receivedPacket.seqNum) {
            continue;
        }
        ACKNum += receivedPacket.dataLen;

        if (receivedPacket.flags == (FIN | ACK)) {
            sentPacket = Packet(receivedPacket.ackNum, ACKNum, 1, ACK, ++sentPacketCount, ".");
            printPacket(sentPacket, true, true);
            sendto(serverSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, remoteAddrSize);
            break;
        }
        else {
            fwrite(receivedPacket.message, 1, receivedPacket.dataLen, outFile);
            sentPacket = Packet(receivedPacket.ackNum, ACKNum, 1, ACK, ++sentPacketCount, ".");
            printPacket(sentPacket, true, true);
            sendto(serverSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, remoteAddrSize);
        }
    }
    
    cout << "File transfer completed." << endl;
    fclose(outFile),system("pause");
    return 0;
}