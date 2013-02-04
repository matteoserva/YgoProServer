#include "GameServer.h"
#include "single_duel.h"
#include "tag_duel.h"
#include "RoomManager.h"

namespace ygo
{

GameServer::GameServer()
{
    server_port = 0;
    net_evbase = 0;
    broadcast_ev = 0;
    listener = 0;
    duel_mode = 0;
    last_sent = 0;


    roomManager.setGameServer(const_cast<ygo::GameServer *>(this));
}



bool GameServer::StartServer(unsigned short port)
{
    if(net_evbase)
        return false;
    net_evbase = event_base_new();
    if(!net_evbase)
        return false;
    sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    server_port = port;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(port);
    listener = evconnlistener_new_bind(net_evbase, ServerAccept, this,
                                       LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1, (sockaddr*)&sin, sizeof(sin));
    if(!listener)
    {
        event_base_free(net_evbase);
        net_evbase = 0;
        return false;
    }
    evconnlistener_set_error_cb(listener, ServerAcceptError);
    Thread::NewThread(ServerThread, this);
    return true;
}
void GameServer::StopServer()
{
    if(!net_evbase)
        return;
    if(duel_mode)
        duel_mode->EndDuel();
    event_base_loopexit(net_evbase, 0);
}
void GameServer::StopBroadcast()
{
    if(!net_evbase || !broadcast_ev)
        return;
    event_del(broadcast_ev);
    evutil_socket_t fd;
    event_get_assignment(broadcast_ev, 0, &fd, 0, 0, 0);
    evutil_closesocket(fd);
    event_free(broadcast_ev);
    broadcast_ev = 0;
}
void GameServer::StopListen()
{
    evconnlistener_disable(listener);
    StopBroadcast();
}
void GameServer::BroadcastEvent(evutil_socket_t fd, short events, void* arg)
{
    sockaddr_in bc_addr;
    socklen_t sz = sizeof(sockaddr_in);
    char buf[256];
    int ret = recvfrom(fd, buf, 256, 0, (sockaddr*)&bc_addr, &sz);
    if(ret == -1)
        return;
    HostRequest* pHR = (HostRequest*)buf;
    if(pHR->identifier == NETWORK_CLIENT_ID)
    {
        SOCKADDR_IN sockTo;
        sockTo.sin_addr.s_addr = bc_addr.sin_addr.s_addr;
        sockTo.sin_family = AF_INET;
        sockTo.sin_port = htons(7921);
        HostPacket hp;
        hp.identifier = NETWORK_SERVER_ID;
        hp.port = server_port;
        hp.version = PRO_VERSION;
        hp.host = duel_mode->host_info;
        BufferIO::CopyWStr(duel_mode->name, hp.name, 20);
        sendto(fd, (const char*)&hp, sizeof(HostPacket), 0, (sockaddr*)&sockTo, sizeof(sockTo));
    }
}
void GameServer::ServerAccept(evconnlistener* listener, evutil_socket_t fd, sockaddr* address, int socklen, void* ctx)
{
    GameServer* that = (GameServer*)ctx;
    bufferevent* bev = bufferevent_socket_new(that->net_evbase, fd, BEV_OPT_CLOSE_ON_FREE);
    DuelPlayer dp;
    dp.name[0] = 0;
    dp.type = 0xff;
    dp.bev = bev;
    that->users[bev] = dp;


    bufferevent_setcb(bev, ServerEchoRead, NULL, ServerEchoEvent, ctx);



    bufferevent_enable(bev, EV_READ);

    printf("utente entrato, serveraccept\n");

}
void GameServer::ServerAcceptError(evconnlistener* listener, void* ctx)
{
    GameServer* that = (GameServer*)ctx;
    event_base_loopexit(that->net_evbase, 0);
}
void GameServer::ServerEchoRead(bufferevent *bev, void *ctx)
{
    GameServer* that = (GameServer*)ctx;


    evbuffer* input = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(input);
    unsigned short packet_len = 0;
    while(true)
    {
        if(len < 2)
            return;
        evbuffer_copyout(input, &packet_len, 2);
        if(len < packet_len + 2)
            return;
        evbuffer_remove(input, that->net_server_read, packet_len + 2);
        if(packet_len)
            that->HandleCTOSPacket(&(that->users[bev]), &(that->net_server_read[2]), packet_len);
        len -= packet_len + 2;
    }
}
void GameServer::ServerEchoEvent(bufferevent* bev, short events, void* ctx)
{
    GameServer* that = (GameServer*)ctx;
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR))
    {
        DuelPlayer* dp = &(that->users[bev]);
        DuelMode* dm = dp->game;
        if(dp->netServer)
            dp->netServer->LeaveGame(dp);
        else that->DisconnectPlayer(dp);
    }
}
int GameServer::ServerThread(void* parama)
{
    GameServer*that = (GameServer*)parama;
    event_base_dispatch(that->net_evbase);

    for(auto bit = that->users.begin(); bit != that->users.end(); ++bit)
    {
        bufferevent_disable(bit->first, EV_READ);
        bufferevent_free(bit->first);
    }
    that->users.clear();
    evconnlistener_free(that->listener);
    that->listener = 0;
    if(that->broadcast_ev)
    {
        evutil_socket_t fd;
        event_get_assignment(that->broadcast_ev, 0, &fd, 0, 0, 0);
        evutil_closesocket(fd);
        event_free(that->broadcast_ev);
        that->broadcast_ev = 0;
    }

    event_base_free(that->net_evbase);
    that->net_evbase = 0;
    return 0;
}
void GameServer::DisconnectPlayer(DuelPlayer* dp)
{
    auto bit = users.find(dp->bev);
    if(bit != users.end())
    {
        dp->netServer=NULL;
        bufferevent_flush(dp->bev, EV_WRITE, BEV_FLUSH);
        bufferevent_disable(dp->bev, EV_READ);
        bufferevent_free(dp->bev);
        users.erase(bit);
    }
}

bool GameServer::handleChatCommand(DuelPlayer* dp,unsigned short* msg)
{

    char messaggio[256];
    int msglen = BufferIO::CopyWStr(msg, messaggio, 256);
    printf("ricevuto messaggio %s\n",messaggio);

    if(!strcmp(messaggio,"!tag"))
    {
        dp->netServer->ExtractPlayer(dp);
        CMNetServer* netServer = roomManager.getFirstAvailableServer(MODE_TAG);
        dp->netServer=netServer;
        netServer->InsertPlayer(dp);
        return true;
    }
    if(!strcmp(messaggio,"!single"))
    {
        dp->netServer->ExtractPlayer(dp);
        CMNetServer* netServer = roomManager.getFirstAvailableServer(MODE_SINGLE);
        dp->netServer=netServer;
        netServer->InsertPlayer(dp);
        return true;
    }



    return false;


}

void GameServer::HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len)
{
    char* pdata = data;




    unsigned char pktType = BufferIO::ReadUInt8(pdata);


    if(pktType ==CTOS_PLAYER_INFO)
    {
        CTOS_PlayerInfo* pkt = (CTOS_PlayerInfo*)pdata;
        printf("playerinfo ricevuto da GameServer\n");
        BufferIO::CopyWStr(pkt->name, dp->name, 20);
        return;
    }

    if(dp->netServer == NULL)
    {
        CMNetServer* netServer = roomManager.getFirstAvailableServer();
        if(netServer == NULL)
        {
            DisconnectPlayer(dp);
            return;
        }
        dp->netServer = netServer;
    }

    if(pktType==CTOS_CHAT && handleChatCommand(dp,(unsigned short*)pdata))
    {


    }
    else
        dp->netServer->HandleCTOSPacket(dp,data,len);

    return;


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
        printf("tempo confermato\n");
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
        printf("playerinfo ricevuto da GameServer\n");
        BufferIO::CopyWStr(pkt->name, dp->name, 20);
        break;
    }
    case CTOS_CREATE_GAME:
    {
        if(dp->game || duel_mode)
            return;
        CTOS_CreateGame* pkt = (CTOS_CreateGame*)pdata;
        if(pkt->info.mode == MODE_SINGLE)
        {
            duel_mode = new SingleDuel(false);
            duel_mode->etimer = event_new(net_evbase, 0, EV_TIMEOUT | EV_PERSIST, SingleDuel::SingleTimer, duel_mode);
        }
        else if(pkt->info.mode == MODE_MATCH)
        {
            duel_mode = new SingleDuel(true);
            duel_mode->etimer = event_new(net_evbase, 0, EV_TIMEOUT | EV_PERSIST, SingleDuel::SingleTimer, duel_mode);
        }
        else if(pkt->info.mode == MODE_TAG)
        {
            duel_mode = new TagDuel();
            duel_mode->etimer = event_new(net_evbase, 0, EV_TIMEOUT | EV_PERSIST, TagDuel::TagTimer, duel_mode);
        }

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
        StartBroadcast();
        break;
    }

    case CTOS_LEAVE_GAME:
    {
        if(!duel_mode)
            break;
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
        duel_mode->ToDuelist(dp);
        break;
    }
    case CTOS_HS_TOOBSERVER:
    {
        if(!duel_mode || duel_mode->pduel)
            break;
        duel_mode->ToObserver(dp);
        break;
    }
    case CTOS_HS_READY:
    case CTOS_HS_NOTREADY:
    {
        if(!duel_mode || duel_mode->pduel)
            break;
        duel_mode->PlayerReady(dp, CTOS_HS_NOTREADY - pktType);
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
        duel_mode->StartDuel(dp);
        break;
    }
    }
}

}
