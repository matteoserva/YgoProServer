#include "GameServer.h"

#include "RoomManager.h"
#include "Statistics.h"
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/tcp.h>
#include "debug.h"
namespace ygo
{
GameServer::GameServer()
{
    server_port = 0;
    net_evbase = 0;
    listener = 0;
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

    //LIBEVENT race condition
    //ignoring sigpipe
    signal(SIGPIPE, SIG_IGN);


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

    StopListen();
    evconnlistener_free(listener);
    listener = 0;
    while(users.size() > 0)
    {
        printf("waiting for reboot, users connected: %lu\n",users.size());
        sleep(1);
    }

    event_base_loopexit(net_evbase, 0);
    while(net_evbase)
    {
        printf("waiting for server thread\n");
        sleep(1);
    }

}

void GameServer::StopListen()
{
    evconnlistener_disable(listener);
}

void GameServer::ServerAccept(evconnlistener* listener, evutil_socket_t fd, sockaddr* address, int socklen, void* ctx)
{
    GameServer* that = (GameServer*)ctx;

    int optval=1;
    int optlen = sizeof(optval);
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen);
    optval = 120;
    setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, &optval, optlen);
    optval = 4;
    setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &optval, optlen);
    optval = 30;
    setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &optval, optlen);


    bufferevent* bev = bufferevent_socket_new(that->net_evbase, fd, BEV_OPT_CLOSE_ON_FREE);
    DuelPlayer dp;
    dp.name[0] = 0;
    dp.type = 0xff;
    dp.bev = bev;
    that->users[bev] = dp;
    dp.netServer=0;
    bufferevent_setcb(bev, ServerEchoRead, NULL, ServerEchoEvent, ctx);
    bufferevent_enable(bev, EV_READ);
    Statistics::getInstance()->setNumPlayers(that->users.size());
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
    /*
    for(auto bit = that->users.begin(); bit != that->users.end(); ++bit)
    {
        bufferevent_disable(bit->first, EV_READ);
        bufferevent_free(bit->first);
    }
    that->users.clear();*/

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

    Statistics::getInstance()->setNumPlayers(users.size());
}


void GameServer::HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len)
{
    char* pdata = data;

    unsigned char pktType = BufferIO::ReadUInt8(pdata);



    if(dp->netServer == NULL)
    {
        if(pktType!=CTOS_PLAYER_INFO)
        {
            printf("BUG: player info is not the first packet\n");
            return;
        }
        if(!roomManager.InsertPlayerInWaitingRoom(dp))
            return;
        int wnumplayers=roomManager.getNumPlayers();
        log(INFO,"rommmanager: there are %d players\n",wnumplayers+1);
    }


    dp->netServer->HandleCTOSPacket(dp,data,len);

    return;

}

}
