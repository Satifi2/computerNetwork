#include "protocol.h"

WSADATA wsa;
SOCKET serverSocket;
struct sockaddr_in serverAddr, remoteAddr;
Packet sentPacket, receivedPacket;
int ack = -1, remoteAddrSize = sizeof(remoteAddr), ackLen = 0;
char fileName[256];
FILE* outFile;
set<Packet> packetsSet;
set<int> ackedPacketsSet;

mutex packetsSetMutex;
bool isFinished = false;

void send() {
    sendto(serverSocket, (char*)&sentPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, remoteAddrSize);
}

void receive() {
    while (true) {
        recvfrom(serverSocket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&remoteAddr, &remoteAddrSize);
        if (validateChecksum(&receivedPacket)) break;
    }
    printPacket(receivedPacket, 0);
}

void receivePacketsThread() {
    while (!isFinished) {
        receive();
        if (receivedPacket.flags == (FIN | ACK)) {
            isFinished = true;
            break;
        }
        if (ackedPacketsSet.find(receivedPacket.seqNum) != ackedPacketsSet.end()) {
            sentPacket = Packet(0, receivedPacket.seqNum, 0, ACK, "");
            send();
        }
        else if (receivedPacket.seqNum > ack && receivedPacket.seqNum <= ack + M) {
            lock_guard<mutex> lock(packetsSetMutex);
            packetsSet.insert(receivedPacket), ackedPacketsSet.insert(receivedPacket.seqNum);
            sentPacket = Packet(0, receivedPacket.seqNum, 0, ACK, "");
            send(), printWindow(packetsSet), cout << "ack: " << ack << endl;
        }
    }
}

void processPacketsThread() {
    while (!isFinished || !packetsSet.empty()) {
        if (!packetsSet.empty() && packetsSet.begin()->seqNum == ack + 1) {
            lock_guard<mutex> lock(packetsSetMutex);
            const Packet& packet = *packetsSet.begin();
            fwrite(packet.message, 1, packet.dataLen, outFile),ackLen += packet.dataLen;
            ack++;
            packetsSet.erase(packetsSet.begin());
        }
    }
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

    do {
        receive();
    } while (receivedPacket.flags != ACK || receivedPacket.seqNum != ack + 1);
    sentPacket = Packet(0, ++ack, 0, ACK, "");
    send();

    auto start_time = chrono::high_resolution_clock::now();

    thread receiver(receivePacketsThread);
    thread processor(processPacketsThread);

    receiver.join();
    processor.join();

    do {
        receive();
    } while (receivedPacket.flags != (FIN | ACK) || receivedPacket.seqNum != ack + 1);
    sentPacket = Packet(0, ++ack, 0, ACK, "");
    send();

    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::seconds>(end_time - start_time);
    cout << "time counter ended , File transfer duration:" << duration.count() << " s " << endl;
    cout << "file transfer size : " << ackLen << " B " << endl;
    if (duration.count() != 0)cout << "file transfer rate : " << (ackLen / 1024 / duration.count()) << " k/s " << endl;
    cout << "File transfer completed." << endl;

    fclose(outFile), system("pause");
    return 0;
}