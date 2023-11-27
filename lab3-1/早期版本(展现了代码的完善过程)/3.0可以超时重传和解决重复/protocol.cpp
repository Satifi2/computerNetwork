#include "protocol.h"
#include <iostream>
using namespace std;

int main() {
    // 创建一个数据包，参数依次是:seqNum,ackNum,dataLen(message的长度),flags(SYN,ACK,FIN),packetId(每发一个packet，packetId+1),message
    Packet pkt(123, 456, 14, SYN | ACK, 1, "Hello, World!");
    // 设置数据包的校验和
    setChecksum(&pkt);
    // 打印数据包，参数依次是:packet，isSent(打印接收到的数据包时设置false)，showMessage(打印信息)
    printPacket(pkt, true, true);
    // validChecksum:校验和是否正确
    bool validChecksum = validateChecksum(&pkt);
    cout << "Checksum is " << (validChecksum ? "valid" : "invalid") << endl;

    return 0;
}
