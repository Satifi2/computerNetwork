#include "protocol.h"
WSADATA wsa;
SOCKET clientSocket;
struct sockaddr_in serverAddr;
Packet sentPacket, receivedPacket;
int timeout = 200, seq = 0, serverAddrSize = sizeof(serverAddr), recvResult;
char fileName[256];
FILE* inFile;
vector<Packet>window;

void send() {
    sendto(clientSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&serverAddr, serverAddrSize);
}

int receive() {
    return recvfrom(clientSocket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&serverAddr, &serverAddrSize);
}

void sendAndReceive(uint8_t ExpectedFlags) {
    while (true) {
        send(), recvResult = receive();
        if (recvResult < 0 || receivedPacket.flags != ExpectedFlags) {
            cout << "resending" << endl;
            continue;
        }
        break;
    }
}

void init() {
    WSAStartup(MAKEWORD(2, 2), &wsa);
    clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddr.sin_port = htons(CLIENT_PORT);
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
}

int main() {
    printSenderArt(), init();
    cout << "Enter filename: ", cin >> fileName;
    inFile = fopen(("./source/" + string(fileName)).c_str(), "rb");
    sentPacket = Packet(seq++, 0, 0, SYN, "");
    sendAndReceive(SYN | ACK);
    while (true) {
        while (!feof(inFile) && window.size() <= N) {
            int bytesRead = fread(sentPacket.message, 1, sizeof(sentPacket.message), inFile);
            if (bytesRead > 0) {
                sentPacket = Packet(seq++, 0, bytesRead, ACK, sentPacket.message);
                send(), window.push_back(sentPacket);
            }
            printWindow(window);
        }
        if (feof(inFile) && window.size() == 0) break;
        recvResult = receive();
        if (recvResult < 0) {
            cout << "resending all" << endl;
            for (auto& packet : window)sentPacket = packet, send();
            continue;
        }
        if (receivedPacket.flags == ACK && receivedPacket.ackNum >= window[0].seqNum && receivedPacket.ackNum <= window.back().seqNum) {
            while (window.size() && window[0].seqNum <= receivedPacket.ackNum)window.erase(window.begin());
        }
    }
    sentPacket = Packet(seq++, 0, 0, FIN | ACK, "");
    sendAndReceive(ACK);
    cout << "File transfer completed successfully." << endl;
    fclose(inFile), system("pause");
    return 0;
}
