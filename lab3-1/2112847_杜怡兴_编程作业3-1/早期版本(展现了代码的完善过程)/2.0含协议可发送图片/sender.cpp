#include "protocol.h"

using namespace std;

// Global variables
WSADATA wsa;
SOCKET clientSocket;
struct sockaddr_in serverAddr;
Packet sentPacket, receivedPacket;
int sentPacketCount = 0;
char fileName[256];
FILE* inFile;

void sendAndReceive(Packet packet, uint8_t firstExpectedFlags, bool waitForSecondPacket = false, uint8_t secondExpectedFlags = 0) {
    int serverAddrSize = sizeof(serverAddr);
    printPacket(packet, true, false);
    sendto(clientSocket, (char*)&packet, sizeof(Packet), 0, (struct sockaddr*)&serverAddr, serverAddrSize);

    // Receive first packet
    do {
        recvfrom(clientSocket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&serverAddr, &serverAddrSize);
        printPacket(receivedPacket, false, true);
    } while (!validateChecksum(&receivedPacket) || receivedPacket.flags != firstExpectedFlags);

    if (waitForSecondPacket) {
        // Receive second packet
        do {
            recvfrom(clientSocket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&serverAddr, &serverAddrSize);
            printPacket(receivedPacket, false, true);
        } while (!validateChecksum(&receivedPacket) || receivedPacket.flags != secondExpectedFlags);
    }
}

int main() {
    printSenderArt();
    // Initialize Winsock
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // Create socket
    clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // Setup address structure
    memset((char*)&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddr.sin_port = htons(CLIENT_PORT);
    while (true) {
        // User selects file
        while (true) {
            cout << "Enter filename: ";
            cin >> fileName;
            inFile = fopen(("./source/" + string(fileName)).c_str(), "rb");
            if (inFile != NULL) { break; }
            cout << "File not found, please try again." << endl;
        }

        // Three-way handshake
        // First two handshakes
        sentPacket = Packet(0, 0, 0, SYN, ++sentPacketCount, "");
        sendAndReceive(sentPacket, SYN | ACK);

        // Last two handshakes
        sentPacket = Packet(receivedPacket.ackNum, receivedPacket.seqNum + 1, strlen(fileName), ACK, ++sentPacketCount, fileName);
        sendAndReceive(sentPacket, ACK);

        // Start file transfer
        while (!feof(inFile)) {
            memset(sentPacket.message, 0, sizeof(sentPacket.message)); // 清空数组
            int bytesRead = fread(sentPacket.message, 1, sizeof(sentPacket.message), inFile);
            if (bytesRead <= 0) break;
            if (bytesRead > 0) {
                sentPacket = Packet(receivedPacket.ackNum, receivedPacket.seqNum + 1, bytesRead, ACK, ++sentPacketCount, sentPacket.message);
                sendAndReceive(sentPacket, ACK);
            }
        }

        // 发送 FIN | ACK，等待 ACK，然后等待 FIN | ACK
        sentPacket = Packet(receivedPacket.ackNum, receivedPacket.seqNum + 1, 0, FIN | ACK, ++sentPacketCount, "");
        sendAndReceive(sentPacket, ACK, true, FIN | ACK);

        // 发送 ACK
        sentPacket = Packet(receivedPacket.ackNum, receivedPacket.seqNum + 1, 0, ACK, ++sentPacketCount, "");
        printPacket(sentPacket, true, true);
        sendto(clientSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

        // Close file and socket
        fclose(inFile);
        cout << "File transfer completed successfully." << endl;
    }
    closesocket(clientSocket);
    WSACleanup();

    system("pause");
    return 0;
}
