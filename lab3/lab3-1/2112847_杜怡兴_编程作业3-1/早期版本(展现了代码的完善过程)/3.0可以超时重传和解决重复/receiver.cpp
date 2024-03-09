#include "protocol.h"

WSADATA wsa;
SOCKET serverSocket;
struct sockaddr_in serverAddr, remoteAddr;
int remoteAddrSize = sizeof(remoteAddr);
Packet sentPacket, receivedPacket;
char buffer[sizeof(Packet)];
int sentPacketCount = 0;
int timeout = 200;//（以毫秒为单位）
int notimeout= -1;
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
        } while (!validateChecksum(&receivedPacket) || (receivedPacket.flags & SYN) == 0);

        // Second handshake - Send SYN|ACK
        sentPacket = Packet(receivedPacket.ackNum, receivedPacket.seqNum + 1, 0, SYN | ACK, ++sentPacketCount, "");
        printPacket(sentPacket, true, true);
        sendto(serverSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, remoteAddrSize);

        // Third handshake - Receive ACK with filename
        do {
            recvfrom(serverSocket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, &remoteAddrSize);
            printPacket(receivedPacket, false, true);
        } while (!validateChecksum(&receivedPacket) || sentPacket.ackNum != receivedPacket.seqNum);

        // Extract filename
        receivedPacket.message[receivedPacket.dataLen] = '\0'; // 确保字符串正确终止
        strcpy(fileName, "./destination/");
        strcat(fileName, receivedPacket.message);
        cout << "received File name: " << fileName << endl;
        outFile = fopen(fileName, "wb");

        // Fourth handshake - Send ACK
        sentPacket = Packet(receivedPacket.ackNum, receivedPacket.seqNum + receivedPacket.dataLen, 0, ACK, ++sentPacketCount, "");
        printPacket(sentPacket, true, true);
        sendto(serverSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, remoteAddrSize);

        // File transfer
        while (true) {
            recvfrom(serverSocket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, &remoteAddrSize);
            printPacket(receivedPacket, false, false);

            if (!validateChecksum(&receivedPacket) || sentPacket.ackNum != receivedPacket.seqNum) {
                continue;
            }

            // Check for termination signal
            if (receivedPacket.flags & FIN) {
                // Termination - First part
                sentPacket = Packet(receivedPacket.ackNum, receivedPacket.seqNum + 1, 0, ACK, ++sentPacketCount, "");
                printPacket(sentPacket, true, true);
                sendto(serverSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, remoteAddrSize);

                // Termination - Second part
                sentPacket = Packet(receivedPacket.ackNum, receivedPacket.seqNum + 1, 0, FIN | ACK, ++sentPacketCount, "");
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
                sentPacket = Packet(receivedPacket.ackNum, receivedPacket.seqNum + receivedPacket.dataLen, 0, ACK, ++sentPacketCount, "");
                printPacket(sentPacket, true, true);
                sendto(serverSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, remoteAddrSize);
            }
        }
        cout << "File transfer completed." << endl;
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
