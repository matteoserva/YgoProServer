#include "netserver.h"
#include "single_duel.h"
#include "tag_duel.h"
#include "GameServer.h"
#include "RoomManager.h"
namespace ygo
{

CMNetServer::CMNetServer(RoomManager*roomManager,unsigned char mode):roomManager(roomManager)
{
    this->mode = mode;

    duel_mode = 0;
    last_sent = 0;

    createGame();
}
void CMNetServer::clientStarted()
{
    setState(PLAYING);
}
void CMNetServer::setState(State state)
{
    this->state=state;
}

void CMNetServer::destroyGame()
{
    if(duel_mode)
    {
        duel_mode->EndDuel();
        event_del(duel_mode->etimer);
        event_free(duel_mode->etimer);
        delete duel_mode;
        duel_mode=0;
    }
}

int CMNetServer::getMaxDuelPlayers()
{
    int maxplayers = 2;
    if(mode == MODE_TAG)
        maxplayers=4;
    return maxplayers;
}

void CMNetServer::playerReadinessChange(DuelPlayer *dp, bool isReady)
{
    players[dp].isReady = isReady;


    printf("readiness change %d\n",isReady);
}

void CMNetServer::playerConnected(DuelPlayer *dp)
{
    numPlayers++;
    players[dp] = DuelPlayerInfo();
    numPlayers=players.size();

    printf("giocatori connessi:%d\n",numPlayers);
    updateServerState();
}

int CMNetServer::getNumDuelPlayers()
{
    int n=0;
    for(auto it = players.cbegin(); it!=players.cend();++it)
    {
        if(it->first->type != NETPLAYER_TYPE_OBSERVER)
            n++;
    }
    return n;
}

void CMNetServer::updateServerState()
{
    if(getNumDuelPlayers() < getMaxDuelPlayers() &&state==FULL)
    {
        setState(WAITING);
        printf("server not full\n");
    }
    if(numPlayers==0 && state==ZOMBIE)
    {
        printf("server vuoto. addio, morto\n");

        destroyGame();
        setState(DEAD);
    }
    if(getNumDuelPlayers()>=getMaxDuelPlayers())// && state==WAITING)
    {
        printf("server full\n");
        setState(FULL);
    }

}

void CMNetServer::playerDisconnected(DuelPlayer* dp )
{
    players.erase(dp);
    numPlayers=players.size();

    printf("giocatori connessi:%d\n",numPlayers);
    updateServerState();
}

void CMNetServer::createGame()
{

    event_base* net_evbase=roomManager->net_evbase;
    if(mode == MODE_SINGLE)
    {
        duel_mode = new SingleDuel(false);
        duel_mode->etimer = event_new(net_evbase, 0, EV_TIMEOUT | EV_PERSIST, SingleDuel::SingleTimer, duel_mode);
    }
    else if(mode == MODE_MATCH)
    {
        duel_mode = new SingleDuel(true);
        duel_mode->etimer = event_new(net_evbase, 0, EV_TIMEOUT | EV_PERSIST, SingleDuel::SingleTimer, duel_mode);
    }
    else if(mode == MODE_TAG)
    {
        duel_mode = new TagDuel();
        duel_mode->etimer = event_new(net_evbase, 0, EV_TIMEOUT | EV_PERSIST, TagDuel::TagTimer, duel_mode);
    }

    BufferIO::CopyWStr(L"", duel_mode->name, 20);
    BufferIO::CopyWStr(L"", duel_mode->pass, 20);

    HostInfo info;
    info.rule=2;
    info.mode=mode;
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
    duel_mode->host_info = info;
    duel_mode->setNetServer(this);
    setState(WAITING);
    numPlayers=0;
}
void CMNetServer::DisconnectPlayer(DuelPlayer* dp)
{
    printf("DisconnectPlayer called\n");

    auto bit = players.find(dp);
    if(bit != players.end())
    {
        gameServer->DisconnectPlayer(dp);
        playerDisconnected(dp);
    }
}

void CMNetServer::ExtractPlayer(DuelPlayer* dp)
{
    //it removes the player from the duel without disconnecting its tcp connection
    printf("ExtractPlayer called\n");
    playerDisconnected(dp);
    LeaveGame(dp);
}
void CMNetServer::InsertPlayer(DuelPlayer* dp)
{
    //it inserts forcefully the player into the server
    printf("InsertPlayer called\n");
    playerConnected(dp);
    CTOS_JoinGame csjg;
    csjg.version = PRO_VERSION;
    csjg.gameid = 0;
    BufferIO::CopyWStr("", csjg.pass, 20);
    dp->game=0;
    dp->type=0xff;
    duel_mode->JoinGame(dp, &csjg, false);
    SendMessageToPlayer(dp,"Welcome to the CheckMate server!");
    SendMessageToPlayer(dp,"Type !tag to enter a tag duel, !single for a single duel");
    duel_mode->host_player=NULL;
}


void CMNetServer::LeaveGame(DuelPlayer* dp)
{
    if(state != ZOMBIE && dp->game == duel_mode)
        duel_mode->LeaveGame(dp);
    else
        DisconnectPlayer(dp);
}

void CMNetServer::StopBroadcast()
{
}

void CMNetServer::StopListen()
{
}

void CMNetServer::StopServer()
{   //the duel asked me to stop
    printf("netserver server diventato zombie\n");
    setState(ZOMBIE);
}

void CMNetServer::SendMessageToPlayer(DuelPlayer*dp, char*msg)
{
    STOC_Chat scc;
    scc.player = dp->type;
    int msglen = BufferIO::CopyWStr(msg, scc.msg, 256);
    SendBufferToPlayer(dp, STOC_CHAT, &scc, 4 + msglen * 2);
}


void CMNetServer::SendPacketToPlayer(DuelPlayer* dp, unsigned char proto)
{
    char* p = net_server_write;
    BufferIO::WriteInt16(p, 1);
    BufferIO::WriteInt8(p, proto);
    last_sent = 3;
    if(!dp)
        return;
    bufferevent_write(dp->bev, net_server_write, last_sent);

    if(proto == STOC_DUEL_START)
        clientStarted();

}
void CMNetServer::HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len)
{
    char* pdata = data;

    if(state==ZOMBIE)
    {
        printf("pacchetto ricevuto per uno zombie, ignorato\n");

        return;
    }
    unsigned char pktType = BufferIO::ReadUInt8(pdata);
    if((pktType != CTOS_SURRENDER) && (pktType != CTOS_CHAT) && (dp->state == 0xff || (dp->state && dp->state != pktType)))
        return;
    switch(pktType)
    {
    case CTOS_RESPONSE:
    {
        if(!dp->game || !duel_mode->pduel)
            return;
        duel_mode->GetResponse(dp, pdata, len > 64 ? 64 : len - 1);
        break;
    }
    case CTOS_TIME_CONFIRM:
    {
        if(!dp->game || !duel_mode->pduel)
            return;
        duel_mode->TimeConfirm(dp);
        break;
    }
    case CTOS_CHAT:
    {
        if(!dp->game)
            return;
        duel_mode->Chat(dp, pdata, len - 1);
        break;
    }
    case CTOS_UPDATE_DECK:
    {
        if(!dp->game)
            return;
        duel_mode->UpdateDeck(dp, pdata);
        break;
    }
    case CTOS_HAND_RESULT:
    {
        if(!dp->game)
            return;
        CTOS_HandResult* pkt = (CTOS_HandResult*)pdata;
        dp->game->HandResult(dp, pkt->res);
        break;
    }
    case CTOS_TP_RESULT:
    {
        if(!dp->game)
            return;
        CTOS_TPResult* pkt = (CTOS_TPResult*)pdata;
        dp->game->TPResult(dp, pkt->res);
        break;
    }
    case CTOS_PLAYER_INFO:
    {
        CTOS_PlayerInfo* pkt = (CTOS_PlayerInfo*)pdata;
        BufferIO::CopyWStr(pkt->name, dp->name, 20);
        char name[20];
        BufferIO::CopyWStr(pkt->name,name,20);

        printf("Player joined %s \n",name);
        printf("playerinfo ricevuto da CMNetServer\n");
        break;
    }
    case CTOS_CREATE_GAME:
    {
        if(dp->game || duel_mode)
            return;
        CTOS_CreateGame* pkt = (CTOS_CreateGame*)pdata;

        duel_mode->setNetServer(this);
        if(pkt->info.rule > 3)
            pkt->info.rule = 0;
        if(pkt->info.mode > 2)
            pkt->info.mode = 0;
        unsigned int hash = 1;
        for(auto lfit = deckManager._lfList.begin(); lfit != deckManager._lfList.end(); ++lfit)
        {
            if(pkt->info.lflist == lfit->hash)
            {
                hash = pkt->info.lflist;
                break;
            }
        }
        if(hash == 1)
            pkt->info.lflist = deckManager._lfList[0].hash;
        duel_mode->host_info = pkt->info;
        BufferIO::CopyWStr(pkt->name, duel_mode->name, 20);
        BufferIO::CopyWStr(pkt->pass, duel_mode->pass, 20);
        duel_mode->JoinGame(dp, 0, true);
        break;
    }
    case CTOS_JOIN_GAME:
    {
        if(!duel_mode)
            break;
        InsertPlayer(dp);
        break;
    }
    case CTOS_LEAVE_GAME:
    {
        if(!duel_mode)
            break;
        playerDisconnected(dp);
        duel_mode->LeaveGame(dp);
        break;
    }
    case CTOS_SURRENDER:
    {
        if(!duel_mode)
            break;
        duel_mode->Surrender(dp);
        break;
    }
    case CTOS_HS_TODUELIST:
    {
        if(!duel_mode || duel_mode->pduel)
            break;
        playerReadinessChange(dp,false);
        duel_mode->ToDuelist(dp);
        updateServerState();
        break;
    }
    case CTOS_HS_TOOBSERVER:
    {
        if(!duel_mode || duel_mode->pduel)
            break;
        duel_mode->ToObserver(dp);
        updateServerState();
        break;
    }
    case CTOS_HS_READY:
    case CTOS_HS_NOTREADY:
    {
        if(!duel_mode || duel_mode->pduel)
            break;
        playerReadinessChange(dp,CTOS_HS_NOTREADY - pktType);

        duel_mode->PlayerReady(dp, CTOS_HS_NOTREADY - pktType);
        duel_mode->host_player = dp;
        duel_mode->StartDuel(dp);
        duel_mode->host_player=NULL;
        break;
    }
    case CTOS_HS_KICK:
    {
        if(!duel_mode || duel_mode->pduel)
            break;
        CTOS_Kick* pkt = (CTOS_Kick*)pdata;
        duel_mode->PlayerKick(dp, pkt->pos);
        break;
    }
    case CTOS_HS_START:
    {
        if(!duel_mode || duel_mode->pduel)
            break;

        break;
    }
    }
}

}
