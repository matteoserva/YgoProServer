#include "NetServerInterface.h"
#include "debug.h"

namespace ygo
{

char CMNetServerInterface::net_server_read[0x20000];
char CMNetServerInterface::net_server_write[0x20000];

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

void CMNetServerInterface::BroadcastSystemChat(std::string msg)
{
    for(auto it = players.cbegin(); it!=players.cend(); ++it)
                {
                    SystemChatToPlayer(it->first,msg.c_str());
                    printf("broadcast n spedito\n");
                }

}

void CMNetServerInterface::SystemChatToPlayer(DuelPlayer*dp, const char*msg)
{
    STOC_Chat scc;
    scc.player = 8;
    int msglen = BufferIO::CopyWStr(msg, scc.msg, 256);
    SendBufferToPlayer(dp, STOC_CHAT, &scc, 4 + msglen * 2);
}

int CMNetServerInterface::getNumPlayers()
{
    return players.size();
}
void CMNetServerInterface::SendPacketToPlayer(DuelPlayer* dp, unsigned char proto)
{
    SendBufferToPlayer(dp, proto, nullptr, 0);
}

void CMNetServerInterface::playerReadinessChange(DuelPlayer *dp, bool isReady)
{
    players[dp].isReady = isReady;
    log(VERBOSE,"readiness change %d\n",isReady);
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
    if(len > 0)
        memcpy(p, buffer, std::min(len,(size_t) 0x20000));
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
