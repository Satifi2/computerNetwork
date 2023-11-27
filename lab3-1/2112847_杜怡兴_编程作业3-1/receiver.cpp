#include "protocol.h"

WSADATA wsa;
SOCKET serverSocket;
struct sockaddr_in serverAddr, remoteAddr;
int remoteAddrSize = sizeof(remoteAddr);
Packet sentPacket, receivedPacket;
int sentPacketCount = 0, ACKNum = 0;
int timeout = 200, notimeout = -1;//（以毫秒为单位）

char fileName[256];
FILE* outFile;

int main() {
    printReceiver();

    // Initialize Winsock
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // Create a socket
    serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // Prepare the sockaddr_in structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);

    // Bind
    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    while (true) {
        cout << "receiver is listening..." << endl;
        // First handshake - Receive SYN
        do {
            recvfrom(serverSocket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, &remoteAddrSize);
            printPacket(receivedPacket, false, true);
        } while (!validateChecksum(&receivedPacket) || receivedPacket.flags != SYN || receivedPacket.seqNum != 0);
        ACKNum += receivedPacket.dataLen;//表示确认接收了第一个数据包(它的SYN必须为SYN，并且是最初数据包)
        cout << "receiver confirmed to " << ACKNum << endl;

        // Second handshake - Send SYN|ACK
        sentPacket = Packet(0,ACKNum , 1, SYN | ACK, ++sentPacketCount, ".");
        printPacket(sentPacket, true, true);
        sendto(serverSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, remoteAddrSize);

        // Third handshake - Receive ACK with filename
        do {
            recvfrom(serverSocket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, &remoteAddrSize);
            printPacket(receivedPacket, false, true);
        } while (!validateChecksum(&receivedPacket) || receivedPacket.flags != ACK || receivedPacket.seqNum != ACKNum);
        ACKNum += receivedPacket.dataLen;//表示确认接收了第二个数据包(它的必须为ACK，并且是第二个数据包)
        cout << "receiver confirmed to " << ACKNum << endl;

        // Extract filename
        receivedPacket.message[receivedPacket.dataLen] = '\0'; // 确保字符串正确终止
        strcpy(fileName, "./destination/");
        strcat(fileName, receivedPacket.message);
        cout << "received File name: " << fileName << endl;
        outFile = fopen(fileName, "wb");

        // Fourth handshake - Send ACK
        sentPacket = Packet(receivedPacket.ackNum,ACKNum, 1, ACK, ++sentPacketCount, ".");
        printPacket(sentPacket, true, true);
        sendto(serverSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, remoteAddrSize);

        auto start_time = chrono::high_resolution_clock::now();
        cout<<"start time counter"<<endl;

        // File transfer
        while (true) {
            recvfrom(serverSocket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, &remoteAddrSize);
            printPacket(receivedPacket, false, false);

            if (!validateChecksum(&receivedPacket) || receivedPacket.flags & ACK == 0 || ACKNum != receivedPacket.seqNum) {
                continue;
            }
            ACKNum += receivedPacket.dataLen;
            cout << "receiver confirmed to " << ACKNum << endl;

            // Check for termination signal
            if (receivedPacket.flags == (FIN | ACK)) {
                // Termination - First part
                sentPacket = Packet(receivedPacket.ackNum,ACKNum, 1, ACK, ++sentPacketCount, ".");
                printPacket(sentPacket, true, true);
                sendto(serverSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, remoteAddrSize);

                // Termination - Second part
                sentPacket = Packet(receivedPacket.ackNum,ACKNum, 1, FIN | ACK, ++sentPacketCount, ".");
                printPacket(sentPacket, true, true);
                sendto(serverSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, remoteAddrSize);

                setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
                // Wait for final ACK
                // do {
                recvfrom(serverSocket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, &remoteAddrSize);
                printPacket(receivedPacket, false, true);
                // } while (!validateChecksum(&receivedPacket) || (receivedPacket.flags & ACK) == 0);
                setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&notimeout, sizeof(notimeout));
                fclose(outFile);

                break;
            }
            else {
                // Write data to file
                fwrite(receivedPacket.message, 1, receivedPacket.dataLen, outFile);
                fflush(outFile);

                // Send ACK back
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

        ACKNum=0;//累计确认必须清零 ，否则后面不会确认
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
