#include "protocol.h"
#include <iostream>
using namespace std;

int main() {
    // ����һ�����ݰ�������������:seqNum,ackNum,dataLen(message�ĳ���),flags(SYN,ACK,FIN),packetId(ÿ��һ��packet��packetId+1),message
    Packet pkt(123, 456, 14, SYN | ACK, 1, "Hello, World!");
    // �������ݰ���У���
    setChecksum(&pkt);
    // ��ӡ���ݰ�������������:packet��isSent(��ӡ���յ������ݰ�ʱ����false)��showMessage(��ӡ��Ϣ)
    printPacket(pkt, true, true);
    // validChecksum:У����Ƿ���ȷ
    bool validChecksum = validateChecksum(&pkt);
    cout << "Checksum is " << (validChecksum ? "valid" : "invalid") << endl;

    return 0;
}
