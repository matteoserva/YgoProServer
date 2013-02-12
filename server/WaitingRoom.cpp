#include "WaitingRoom.h"
#include "GameServer.h"
namespace ygo
{
int WaitingRoom::minSecondsWaiting=4;
int WaitingRoom::maxSecondsWaiting=6;


WaitingRoom::WaitingRoom(RoomManager*roomManager,GameServer*gameServer):
    CMNetServerInterface(roomManager,gameServer),cicle_users(0)
{
    event_base* net_evbase=roomManager->net_evbase;
    cicle_users = event_new(net_evbase, 0, EV_TIMEOUT | EV_PERSIST, cicle_users_cb, const_cast<WaitingRoom*>(this));
    timeval timeout = {1, 0};
    event_add(cicle_users, &timeout);
}

WaitingRoom::~WaitingRoom()
{
    event_free(cicle_users);
}
void WaitingRoom::cicle_users_cb(evutil_socket_t fd, short events, void* arg)
{
    WaitingRoom*that = (WaitingRoom*)arg;

    if(!that->players.size())
        return;

    std::list<DuelPlayer*> players_bored;
    int numPlayersReady=0;
    for(auto it=that->players.begin(); it!=that->players.end(); ++it)
    {
        it->second.secondsWaiting++;
        char nome[30];
        BufferIO::CopyWStr(it->first->name,nome,30);
        if(!that->players[it->first].isReady)
                    continue;
        //printf("%s aspetta da %d secondi\n",nome,it->second.secondsWaiting);
        if(it->second.secondsWaiting>= maxSecondsWaiting)
            players_bored.push_back(it->first);
        if(it->second.secondsWaiting>= minSecondsWaiting)
            numPlayersReady++;
    }

    if(players_bored.size() && numPlayersReady>=2)
        for (auto it = players_bored.cbegin(); it != players_bored.cend(); ++it)
        {
            if(that->players.find(*it) != that->players.end())
            {

                that->ExtractPlayer(*it);
                that->roomManager->InsertPlayer(*it);
            }
        }
}


void WaitingRoom::ExtractPlayer(DuelPlayer* dp)
{
    players.erase(dp);
    dp->netServer=0;
}

void WaitingRoom::updateObserversNum()
{
    int num = players.size() -1;
    STOC_HS_WatchChange scwc;
    scwc.watch_count = num;
    for(auto it=players.begin(); it!=players.end(); ++it)
    {
        SendPacketToPlayer(it->first, STOC_HS_WATCH_CHANGE, scwc);
    }



}
void WaitingRoom::InsertPlayer(DuelPlayer* dp)
{
    dp->netServer=this;
    players[dp] = DuelPlayerInfo();

    HostInfo info;
    info.rule=2;
    info.mode=MODE_SINGLE;
    info.draw_count=1;
    info.no_check_deck=false;
    info.start_hand=5;
    info.lflist=1;
    info.time_limit=120;
    info.start_lp=8000;
    info.enable_priority=false;
    info.no_shuffle_deck=false;

    unsigned int hash = 1;
    for(auto lfit = deckManager._lfList.begin(); lfit != deckManager._lfList.end(); ++lfit)
    {
        if(info.lflist == lfit->hash)
        {
            hash = info.lflist;
            break;
        }
    }
    if(hash == 1)
        info.lflist = deckManager._lfList[0].hash;

    dp->type = NETPLAYER_TYPE_PLAYER1;

    STOC_JoinGame scjg;
    scjg.info=info;
    SendPacketToPlayer(dp, STOC_JOIN_GAME, scjg);

    STOC_TypeChange sctc;
    sctc.type = dp->type;
    SendPacketToPlayer(dp, STOC_TYPE_CHANGE, sctc);

    STOC_HS_PlayerEnter scpe;
    BufferIO::CopyWStr(dp->name, scpe.name, 20);
    scpe.pos = 0;
    SendPacketToPlayer(dp, STOC_HS_PLAYER_ENTER, scpe);

    //STOC_HS_PlayerEnter scpe;
    BufferIO::CopyWStr("Wait, please", scpe.name, 20);
    scpe.pos = 1;
    SendPacketToPlayer(dp, STOC_HS_PLAYER_ENTER, scpe);

    usleep(50000);
    SendMessageToPlayer(dp,"Welcome to the CheckMate server!");
    SendMessageToPlayer(dp,"Type !tag to enter a tag duel, !single for a single duel or !match");

    updateObserversNum();

    playerReadinessChange(dp,true);

    STOC_HS_PlayerChange scpc;
    scpc.status = (dp->type << 4) | PLAYERCHANGE_READY;
    SendPacketToPlayer(dp, STOC_HS_PLAYER_CHANGE, scpc);

}
void WaitingRoom::LeaveGame(DuelPlayer* dp)
{
    ExtractPlayer(dp);
    gameServer->DisconnectPlayer(dp);
    updateObserversNum();
}

DuelPlayer* WaitingRoom::ExtractBestMatchPlayer(DuelPlayer*)
{
    //printf("Utenti in attesa: %d\n",players.size());
    if(!players.size())
        return nullptr;

    for(auto it=players.cbegin(); it!=players.cend(); ++it)
    {
        if(it->second.secondsWaiting >= minSecondsWaiting)
        {
            DuelPlayer* dp = it->first;
            if(!players[dp].isReady)
                continue;
            ExtractPlayer(dp);
            return dp;
        }
    }
    return nullptr;
}

bool WaitingRoom::handleChatCommand(DuelPlayer* dp,unsigned short* msg)
{
    char messaggio[256];
    int msglen = BufferIO::CopyWStr(msg, messaggio, 256);
    printf("ricevuto messaggio %s\n",messaggio);

    if(!strcmp(messaggio,"!tag"))
    {
        ExtractPlayer(dp);
        return roomManager->InsertPlayer(dp,MODE_TAG);

    }
    if(!strcmp(messaggio,"!single"))
    {
        ExtractPlayer(dp);
        return roomManager->InsertPlayer(dp,MODE_SINGLE);
    }
    if(!strcmp(messaggio,"!match"))
    {
        ExtractPlayer(dp);
        return roomManager->InsertPlayer(dp,MODE_MATCH);
    }

    return false;
}
void WaitingRoom::HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len)
{
    char*pdata=data;
    unsigned char pktType = BufferIO::ReadUInt8(pdata);

    switch(pktType)
    {

    case CTOS_PLAYER_INFO:
    {
        CTOS_PlayerInfo* pkt = (CTOS_PlayerInfo*)pdata;
        BufferIO::CopyWStr(pkt->name, dp->name, 20);
        char name[20];
        BufferIO::CopyWStr(pkt->name,name,20);

        printf("WaitingRoom:Player joined %s \n",name);
        break;
    }
    case CTOS_CHAT:
    {
        handleChatCommand(dp,(unsigned short*)pdata);

        break;
    }

    case CTOS_JOIN_GAME:
    {
        InsertPlayer(dp);
        break;
    }
    case CTOS_HS_READY:
    case CTOS_HS_NOTREADY:
    {

        playerReadinessChange(dp,CTOS_HS_NOTREADY - pktType);
        break;
    }

    }
}

}
