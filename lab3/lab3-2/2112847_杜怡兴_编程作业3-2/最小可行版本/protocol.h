#include<iostream>
#include<WinSock2.h>
#include<Windows.h>
#include<cstring>
#include <chrono>
#include <cstdio>
#include<vector>
#include <thread>
#include <mutex>
#include <condition_variable>
using namespace std;
#pragma comment(lib,"ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 61000
#define CLIENT_PORT 60000
#define N 32

#pragma pack(push, 1)

struct Packet;
void setChecksum(Packet* packet);
void printPacket(const Packet& packet);
struct Packet {
    uint16_t checksum = 0;
    uint32_t seqNum = 0;
    uint32_t ackNum = 0;
    uint16_t dataLen = 0;
    uint8_t  flags = 0;
    uint16_t packetNum = 0;
    char          reserved[5] = { 0, 0, 0, 0, 0 };
    char          message[8172];

    Packet(uint32_t seq = 0, uint32_t ack = 0, uint16_t len = 0, uint8_t flgs = 0, const char* msg = "")
        : seqNum(seq), ackNum(ack), dataLen(len), flags(flgs) {
        memcpy(this->message, msg, len);
        setChecksum(this);
        printPacket(*this);
    }
};
#pragma pack(pop)

enum Flag {
    SYN = 1,
    ACK = 2,
    FIN = 4,
};

uint16_t calculateChecksum(const Packet* packet) {
    uint32_t sum = 0;
    const uint16_t* data = reinterpret_cast<const uint16_t*>(packet);
    for (size_t i = 0; i < 20 / 2; ++i, ++data) {
        sum += *data;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return static_cast<uint16_t>(~sum);
}

void setChecksum(Packet* packet) {
    packet->checksum = 0;
    packet->checksum = calculateChecksum(packet);
}

bool validateChecksum(const Packet* packet) {
    return calculateChecksum(packet) == 0;
}

void printPacket(const Packet& packet) {
    cout << " [package]: "
        << "validateChecksum: " << (validateChecksum(&packet) ? "true" : "false")
        << ", SeqNum: " << packet.seqNum
        << ", AckNum: " << packet.ackNum
        << ", DataLen: " << packet.dataLen
        << ", Flags: [";
    bool firstFlag = true;
    if (packet.flags & SYN) {
        cout << (firstFlag ? "" : ",") << "SYN";
        firstFlag = false;
    }
    if (packet.flags & ACK) {
        cout << (firstFlag ? "" : ",") << "ACK";
        firstFlag = false;
    }
    if (packet.flags & FIN) {
        cout << (firstFlag ? "" : ",") << "FIN";
        firstFlag = false;
    }
    cout << "]" << endl;
}

void printSenderArt() {
    system("cls");
    cout << "CLIENT_PORT 60000" << endl << "SERVER_PORT 61000" << endl;
    cout << "  ___    ___   _ __     __| |   ___   _ __ \n";
    cout << " / __|  / _ \\ | '_ \\   / _` |  / _ \\ | '__|\n";
    cout << " \\__ \\ |  __/ | | | | | (_| | |  __/ | |   \n";
    cout << " |___/  \\___| |_| |_|  \\__,_|  \\___| |_|\n";
}

void printReceiver() {
    system("cls");
    cout << "CLIENT_PORT 60000" << endl << "SERVER_PORT 61000" << endl;
    cout << "                              _                       " << endl;
    cout << "  _ __    ___    ___    ___  (_) __   __   ___   _ __ " << endl;
    cout << " | '__|  / _ \\  / __|  / _ \\ | | \\ \\ / /  / _ \\ | '__|" << endl;
    cout << " | |    |  __/ | (__  |  __/ | |  \\ V /  |  __/ | |   " << endl;
    cout << " |_|     \\___|  \\___|  \\___| |_|   \\_/    \\___| |_|   " << endl;
}

void printWindow(vector<Packet>& window) {
    cout << "window[";
    for (int i = 0; i < N; i++) {
        if (i < window.size())cout << window[i].seqNum << " ";
        else cout << "- ";
    }
    cout << "]" << endl;
}