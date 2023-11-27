#pragma once
#include<iostream>
#include<WinSock2.h>
#include<winsock.h>
#include<Windows.h>
#include<string>
#include<cstring>
#include<io.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
#include <cstdint>
#include <cstdio>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 61000
#define CLIENT_PORT 60000

#pragma pack(push, 1)

struct Packet;
void setChecksum(Packet* packet);

struct Packet {
    uint16_t checksum = 0;
    uint32_t seqNum = 0;
    uint32_t ackNum = 0;
    uint16_t dataLen = 0;
    uint8_t  flags = 0;
    uint16_t packetNum = 0;
    char          reserved[5] = {0, 0, 0, 0, 0};
    char          message[8172];

    // ���캯��
    Packet(uint32_t seq = 0, uint32_t ack = 0, uint16_t len = 0, uint8_t flgs = 0, uint16_t pkt = 0, const char* msg = "") 
        : seqNum(seq), ackNum(ack), dataLen(len), flags(flgs), packetNum(pkt) {
        memcpy(this->message, msg, len);

        // �ڹ��캯���е���setChecksum������У���
        setChecksum(this);
    }

};
#pragma pack(pop)

enum Flag {
    SYN = 1,
    ACK = 2,
    FIN = 4,
    // �����־λ
};

// ����У���
uint16_t calculateChecksum(const Packet* packet) {
    uint32_t sum = 0;
    const uint16_t* data = reinterpret_cast<const uint16_t*>(packet);
    for (size_t i = 0; i < 20/2; ++i, ++data) {
        sum += *data;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return static_cast<uint16_t>(~sum);
}

// �������ݰ���У���
void setChecksum(Packet* packet) {
    packet->checksum = 0;
    packet->checksum = calculateChecksum(packet);
}

// ��֤У���
bool validateChecksum(const Packet* packet) {
    return calculateChecksum(packet) == 0;
}

// ��ӡ���ݰ�
void printPacket(const Packet& packet, bool isSent, bool showMessage) {
    cout << (isSent ? "[Sent" : "[Received") << " Packet]: "
              << "validateChecksum: " << (validateChecksum(&packet) ? "true" : "false") // ��� validateChecksum ����ֵ
              << ", SeqNum: " << packet.seqNum
              << ", AckNum: " << packet.ackNum
              << ", DataLen: " << packet.dataLen
              << ", Flags: [";
    
    // ��� flags �����ݣ�ʹ��λ�����鲢��ӡ��Ӧ�ı�־λ
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
    // ����и����־λ������������Ӧ�ļ��ʹ�ӡ

    cout << "]";

    if (showMessage) {
        cout << endl << "Message: " << packet.message << endl;
    } else {
        cout << endl;
    }
}

void printSenderArt() {
    cout << "  ___    ___   _ __     __| |   ___   _ __ \n";
    cout << " / __|  / _ \\ | '_ \\   / _` |  / _ \\ | '__|\n";
    cout << " \\__ \\ |  __/ | | | | | (_| | |  __/ | |   \n";
    cout << " |___/  \\___| |_| |_|  \\__,_|  \\___| |_|\n";
}

void printReceiver() {
    cout << "                              _                       " << endl;
    cout << "  _ __    ___    ___    ___  (_) __   __   ___   _ __ " << endl;
    cout << " | '__|  / _ \\  / __|  / _ \\ | | \\ \\ / /  / _ \\ | '__|" << endl;
    cout << " | |    |  __/ | (__  |  __/ | |  \\ V /  |  __/ | |   " << endl;
    cout << " |_|     \\___|  \\___|  \\___| |_|   \\_/    \\___| |_|   " << endl;
}