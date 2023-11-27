#include "protocol.h"
WSADATA wsa;
SOCKET clientSocket;
struct sockaddr_in serverAddr;
Packet sentPacket, receivedPacket;
int timeout = 200, ACKNum = 0;
char fileName[256];
FILE* inFile;

void sendAndReceive(Packet packet, uint8_t firstExpectedFlags) {
    int serverAddrSize = sizeof(serverAddr), recvResult;
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    while (true) {
        sendto(clientSocket, (char*)&packet, sizeof(Packet), 0, (struct sockaddr*)&serverAddr, serverAddrSize);
        recvResult = recvfrom(clientSocket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&serverAddr, &serverAddrSize);
        if (recvResult < 0 || !validateChecksum(&receivedPacket) || receivedPacket.flags != firstExpectedFlags) {
            cout << "resending" << endl;
            continue;
        }
        ACKNum += receivedPacket.dataLen;
        break;
    }
}

void init() {
    WSAStartup(MAKEWORD(2, 2), &wsa);
    clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    memset((char*)&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddr.sin_port = htons(CLIENT_PORT);
}

int main() {
    printSenderArt(), init();
    cout << "Enter filename: ";
    cin >> fileName;
    inFile = fopen(("./source/" + string(fileName)).c_str(), "rb");
    sentPacket = Packet(0, 0, 1, SYN, ".");
    sendAndReceive(sentPacket, SYN | ACK);
    while (!feof(inFile)) {
        memset(sentPacket.message, 0, sizeof(sentPacket.message));
        int bytesRead = fread(sentPacket.message, 1, sizeof(sentPacket.message), inFile);
        if (bytesRead > 0) {
            sentPacket = Packet(receivedPacket.ackNum, ACKNum, bytesRead, ACK, sentPacket.message);
            sendAndReceive(sentPacket, ACK);
        }
    }
    sentPacket = Packet(receivedPacket.ackNum, ACKNum, 1, FIN | ACK, ".");
    sendAndReceive(sentPacket, ACK);
    cout << "File transfer completed successfully." << endl;
    fclose(inFile), system("pause");
    return 0;
}
