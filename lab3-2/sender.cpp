#include "protocol.h"

WSADATA wsa;
SOCKET clientSocket;
struct sockaddr_in serverAddr;
Packet sentPacket, receivedPacket;
int sentPacketCount = 0;
int timeout = 200,maxTimeout = 2000 , minTimeout = 100;
char fileName[256];
FILE* inFile;
int ACKNum = 0;

void sendAndReceive(Packet packet, uint8_t firstExpectedFlags, bool waitForSecondPacket = false, uint8_t secondExpectedFlags = 0) {
    int serverAddrSize = sizeof(serverAddr);
    int recvResult;
    bool shouldResend = true;  
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    while (true) {
        if (shouldResend) {
            printPacket(packet, true, false);
            sendto(clientSocket, (char*)&packet, sizeof(Packet), 0, (struct sockaddr*)&serverAddr, serverAddrSize);
            shouldResend = false;
        }
        recvResult = recvfrom(clientSocket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&serverAddr, &serverAddrSize);
        if(recvResult>=0)printPacket(receivedPacket, false, true);
        if (recvResult < 0 || !validateChecksum(&receivedPacket) || receivedPacket.flags != firstExpectedFlags) {
            if (recvResult < 0){
                shouldResend = true;
                cout<<"timeout resending... RTO:"<< timeout  <<"  increased "<<endl;
                if(timeout<maxTimeout)timeout+=100;
            }
            continue;
        }
        if (waitForSecondPacket) {
            recvResult = recvfrom(clientSocket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&serverAddr, &serverAddrSize);
            if(recvResult>=0)printPacket(receivedPacket, false, true);
            if (recvResult < 0 || !validateChecksum(&receivedPacket) || receivedPacket.flags != secondExpectedFlags) {
                if (recvResult < 0){
                    shouldResend = true;
                    cout<<"timeout resending..."<<endl;
                }
                continue;
            }
        }
        if(timeout>minTimeout)timeout-=100;
        ACKNum += receivedPacket.dataLen;
        cout << "sender confirmed to " << ACKNum << endl;
        break;
    }
}

int main() {
    printSenderArt();
    WSAStartup(MAKEWORD(2, 2), &wsa);
    clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    memset((char*)&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddr.sin_port = htons(CLIENT_PORT);
    while (true) {
        while (true) {
            cout << "Enter filename: ";
            cin >> fileName;
            inFile = fopen(("./source/" + string(fileName)).c_str(), "rb");
            if (inFile != NULL) { break; }
            cout << "File not found, please try again." << endl;
        }
        sentPacket = Packet(0, 0, 1, SYN, ++sentPacketCount, ".");
        sendAndReceive(sentPacket, SYN | ACK);
        sentPacket = Packet(receivedPacket.ackNum, ACKNum, strlen(fileName), ACK, ++sentPacketCount, fileName);
        sendAndReceive(sentPacket, ACK);
        while (!feof(inFile)) {
            memset(sentPacket.message, 0, sizeof(sentPacket.message)); 
            int bytesRead = fread(sentPacket.message, 1, sizeof(sentPacket.message), inFile);
            if (bytesRead <= 0) break;
            if (bytesRead > 0) {
                sentPacket = Packet(receivedPacket.ackNum, ACKNum, bytesRead, ACK, ++sentPacketCount, sentPacket.message);
                sendAndReceive(sentPacket, ACK);
            }
        }
        sentPacket = Packet(receivedPacket.ackNum, ACKNum, 1, FIN | ACK, ++sentPacketCount, ".");
        sendAndReceive(sentPacket, ACK, true, FIN | ACK);
        sentPacket = Packet(receivedPacket.ackNum, ACKNum, 1, ACK, ++sentPacketCount, ".");
        printPacket(sentPacket, true, true);
        sendto(clientSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        fclose(inFile);
        cout << "File transfer completed successfully." << endl;
    }
    closesocket(clientSocket);
    WSACleanup();
    system("pause");
    return 0;
}
