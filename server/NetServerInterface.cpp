#include "NetServerInterface.h"
#include "debug.h"

namespace ygo
{



CMNetServerInterface::CMNetServerInterface(RoomManager* roomManager,GameServer*gameServer):
roomManager(roomManager),gameServer(gameServer),last_sent(0)
{

}

void CMNetServerInterface::SendMessageToPlayer(DuelPlayer*dp, char*msg)
{
    STOC_Chat scc;
    scc.player = dp->type;
    int msglen = BufferIO::CopyWStr(msg, scc.msg, 256);
    SendBufferToPlayer(dp, STOC_CHAT, &scc, 4 + msglen * 2);
}
int CMNetServerInterface::getNumPlayers()
{
    return players.size();
}
void CMNetServerInterface::SendPacketToPlayer(DuelPlayer* dp, unsigned char proto)
{
    if(players.find(dp) == players.end())
    {
        log(INFO,"sendpacket ignorato\n");
        return;
    }
    char* p = net_server_write;
    BufferIO::WriteInt16(p, 1);
    BufferIO::WriteInt8(p, proto);
    last_sent = 3;
    if(!dp)
        return;
    bufferevent_write(dp->bev, net_server_write, last_sent);
}

void CMNetServerInterface::playerReadinessChange(DuelPlayer *dp, bool isReady)
{
    players[dp].isReady = isReady;
    log(INFO,"readiness change %d\n",isReady);
}

void CMNetServerInterface::SendBufferToPlayer(DuelPlayer* dp, unsigned char proto, void* buffer, size_t len)
{
    if( players.end() == players.find(dp))
    {
        log(INFO,"sendbuffer ignorato \n");
        return;
    }
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
    if(players.find(dp) == players.end())
    {
        log(INFO,"resend ignorato\n");
        return;
    }
    if(dp)
        bufferevent_write(dp->bev, net_server_write, last_sent);
}

}
