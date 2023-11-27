#include "protocol.h"

WSADATA wsa;
SOCKET serverSocket;
struct sockaddr_in serverAddr, remoteAddr;
Packet sentPacket, receivedPacket;
int sentPacketCount = 0, ACKNum = 0,timeout = 200, notimeout = -1,remoteAddrSize = sizeof(remoteAddr);
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

    while (true) {
        cout << "receiver is listening..." << endl;
        do {
            recvfrom(serverSocket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, &remoteAddrSize);
            printPacket(receivedPacket, false, true);
        } while (!validateChecksum(&receivedPacket) || receivedPacket.flags != SYN || receivedPacket.seqNum != 0);
        ACKNum += receivedPacket.dataLen;
        cout << "receiver confirmed to " << ACKNum << endl;
        
        sentPacket = Packet(0,ACKNum , 1, SYN | ACK, ++sentPacketCount, ".");
        printPacket(sentPacket, true, true);
        sendto(serverSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, remoteAddrSize);

        do {
            recvfrom(serverSocket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, &remoteAddrSize);
            printPacket(receivedPacket, false, true);
        } while (!validateChecksum(&receivedPacket) || receivedPacket.flags != ACK || receivedPacket.seqNum != ACKNum);
        ACKNum += receivedPacket.dataLen;
        cout << "receiver confirmed to " << ACKNum << endl;
        
        receivedPacket.message[receivedPacket.dataLen] = '\0'; 
        strcpy(fileName, "./destination/");
        strcat(fileName, receivedPacket.message);
        cout << "received File name: " << fileName << endl;
        outFile = fopen(fileName, "wb");

        sentPacket = Packet(receivedPacket.ackNum,ACKNum, 1, ACK, ++sentPacketCount, ".");
        printPacket(sentPacket, true, true);
        sendto(serverSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, remoteAddrSize);

        auto start_time = chrono::high_resolution_clock::now();
        cout<<"start time counter"<<endl;

        while (true) {
            recvfrom(serverSocket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, &remoteAddrSize);
            printPacket(receivedPacket, false, false);
            if (!validateChecksum(&receivedPacket) || receivedPacket.flags & ACK == 0 || ACKNum != receivedPacket.seqNum) {
                continue;
            }
            ACKNum += receivedPacket.dataLen;
            cout << "receiver confirmed to " << ACKNum << endl;

            if (receivedPacket.flags == (FIN | ACK)) {
                sentPacket = Packet(receivedPacket.ackNum,ACKNum, 1, ACK, ++sentPacketCount, ".");
                printPacket(sentPacket, true, true);
                sendto(serverSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, remoteAddrSize);
                sentPacket = Packet(receivedPacket.ackNum,ACKNum, 1, FIN | ACK, ++sentPacketCount, ".");
                printPacket(sentPacket, true, true);
                sendto(serverSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, remoteAddrSize);
                setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
                recvfrom(serverSocket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, &remoteAddrSize);
                printPacket(receivedPacket, false, true);
                setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&notimeout, sizeof(notimeout));
                fclose(outFile);
                break;
            }
            else {
                fwrite(receivedPacket.message, 1, receivedPacket.dataLen, outFile);
                fflush(outFile);
                sentPacket = Packet(receivedPacket.ackNum,ACKNum, 1, ACK, ++sentPacketCount, ".");
                printPacket(sentPacket, true, true);
                sendto(serverSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, remoteAddrSize);
            }
        }

        auto end_time = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::seconds>(end_time - start_time);
        cout<<"time counter ended , File transfer duration:"<< duration.count() <<" s "<<endl;
        cout<<"file transfer size : "<<ACKNum<<" B "<<endl;
        if(duration.count()!=0)cout<<"file transfer rate : "<<(ACKNum/1024/duration.count())<<" k/s "<<endl;
        cout << "File transfer completed." << endl;

        ACKNum=0;
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}