#include "NetServerInterface.h"


namespace ygo
{



CMNetServerInterface::CMNetServerInterface(RoomManager* roomManager):roomManager(roomManager),last_sent(0)
{

}

void CMNetServerInterface::SendMessageToPlayer(DuelPlayer*dp, char*msg)
{
    STOC_Chat scc;
    scc.player = dp->type;
    int msglen = BufferIO::CopyWStr(msg, scc.msg, 256);
    SendBufferToPlayer(dp, STOC_CHAT, &scc, 4 + msglen * 2);
}


void CMNetServerInterface::SendPacketToPlayer(DuelPlayer* dp, unsigned char proto)
{
    char* p = net_server_write;
    BufferIO::WriteInt16(p, 1);
    BufferIO::WriteInt8(p, proto);
    last_sent = 3;
    if(!dp)
        return;
    bufferevent_write(dp->bev, net_server_write, last_sent);
}



void CMNetServerInterface::SendBufferToPlayer(DuelPlayer* dp, unsigned char proto, void* buffer, size_t len)
{
    char* p = net_server_write;
    BufferIO::WriteInt16(p, 1 + len);
    BufferIO::WriteInt8(p, proto);
    memcpy(p, buffer, len);
    last_sent = len + 3;
    if(dp)
        bufferevent_write(dp->bev, net_server_write, last_sent);
}
void CMNetServerInterface::ReSendToPlayer(DuelPlayer* dp)
{
    if(dp)
        bufferevent_write(dp->bev, net_server_write, last_sent);
}

}
