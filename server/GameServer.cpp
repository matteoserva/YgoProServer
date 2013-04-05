#include "GameServer.h"

#include "RoomManager.h"
#include "debug.h"
#include <time.h>
#include <netinet/tcp.h>
#include <thread>
#include "Statistics.h"

#include "Users.h"

using ygo::Config;
namespace ygo
{
GameServer::GameServer(int server_fd):server_fd(server_fd),chat_cb(nullptr)
{
    server_port = 0;
    net_evbase = 0;
    listener = nullptr;
    last_sent = 0;
    MAXPLAYERS = Config::getInstance()->max_users_per_process;
}

bool GameServer::StartServer()
{
    if(net_evbase)
        return false;
    net_evbase = event_base_new();
    if(!net_evbase)
        return false;

    listener =evconnlistener_new(net_evbase,ServerAccept, this, LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE ,-1,server_fd);

    if(!listener)
    {
        event_base_free(net_evbase);
        net_evbase = 0;
        return false;
    }
    isListening=true;
    evconnlistener_set_error_cb(listener, ServerAcceptError);
    roomManager.setGameServer(const_cast<ygo::GameServer *>(this));

    Thread::NewThread(ServerThread, this);



    return true;
}
int GameServer::getNumPlayers()
{
    return users.size();


}
GameServer::~GameServer()
{
    if(listener)
    {
        StopServer();
    }

    while(users.size() > 0)
    {
        log(WARN,"waiting for reboot, users connected: %lu\n",users.size());
        sleep(5);
    }

    event_base_loopexit(net_evbase, 0);
    while(net_evbase)
    {
        log(WARN,"waiting for server thread\n");
        sleep(1);
    }
}
void GameServer::StopServer()
{
    if(!net_evbase)
        return;

    if(listener == nullptr)
        return;
    StopListen();
    evconnlistener_free(listener);
    listener = nullptr;


}

void GameServer::StopListen()
{
    evconnlistener_disable(listener);
    isListening = false;
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
    dp.netServer=0;
    dp.loginStatus = Users::LoginResult::NOTENTERED;
    sockaddr_in* sa = (sockaddr_in*)address;

    inet_ntop(AF_INET, &(sa->sin_addr), dp.ip, INET_ADDRSTRLEN);

    dp.countryCode = Users::getInstance()->getCountryCode(std::string(dp.ip));

    /*
        //prendo il reverse hostname
        char node[NI_MAXHOST];

        int res = getnameinfo(address, socklen, node, sizeof(node), nullptr, 0, 0);
        if (res)
        {
            printf("%s\n", gai_strerror(res));
            //exit(1);
        }
        else
        printf("indirizzo: %s\n",node);
    */

    that->users[bev] = dp;

    bufferevent_setcb(bev, ServerEchoRead, NULL, ServerEchoEvent, ctx);
    bufferevent_enable(bev, EV_READ);
    if(that->users.size()>= that->MAXPLAYERS)
    {
        that->StopListen();

    }

    Statistics::getInstance()->setNumPlayers(that->getNumPlayers());
}
void GameServer::ServerAcceptError(evconnlistener* listener, void* ctx)
{
    GameServer* that = (GameServer*)ctx;
    that->StopListen();
}

void GameServer::setChatCallback(ChatCallback ccb,void*ptr)
{
    chat_cb = ccb;
    chat_cb_ptr=ptr;
}

void GameServer::callChatCallback(std::wstring a,bool b)
{
    if(chat_cb == nullptr)
        return;
    (*chat_cb)(a,b,chat_cb_ptr);
}

void GameServer::injectChatMessage(std::wstring a,bool b)
{
    std::lock_guard<std::mutex> lock(injectedMessages_mutex);
    injectedMessages.push_back(std::pair<std::wstring,bool>(a,b));
}

DuelPlayer* GameServer::findPlayer(std::wstring nome)
{
    for(auto it = loggedUsers.begin();it!=loggedUsers.end();++it)
    {
        if(it->first ==nome)
        {
            return (it->second);
        }
    }

    return nullptr;
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
        if(dp->netServer)
        {
            dp->netServer->LeaveGame(dp);
            if(that->users.find(bev)!= that->users.end())
            {
                log(BUG,"BUG: tcp terminated but disconnectplayer not called\n");
                that->DisconnectPlayer(dp);
            }

        }
        else that->DisconnectPlayer(dp);
    }
}

void GameServer::RestartListen()
{
    if(!isListening)
    {
        evconnlistener_enable(listener);
        isListening = true;
    }
}

int GameServer::CheckAliveThread(void* parama)
{
    GameServer*that = (GameServer*)parama;

    if(!that->net_evbase)
        return 0;

    static time_t last_check = time(NULL);
    int sleepSeconds = 60;

    if(time(NULL)- last_check < sleepSeconds)
        return 0;

    if(!that->isAlive)
    {
        volatile int *p = reinterpret_cast<volatile int*>(0);
        *p = 0x1337D00D;
        exit(EXIT_FAILURE);
    }
    that->isAlive=false;
    last_check = time(NULL);

    return 0;
}
void GameServer::keepAlive(evutil_socket_t fd, short events, void* arg)
{
    GameServer*that = (GameServer*) arg;
    that->isAlive = true;

}

void GameServer::checkInjectedMessages_cb(evutil_socket_t fd, short events, void* arg)
{
    GameServer*that = (GameServer*) arg;
    std::lock_guard<std::mutex> lock(that->injectedMessages_mutex);
    if(that->injectedMessages.size() == 0)
        return;

    for(auto it = that->injectedMessages.begin(); it!=that->injectedMessages.end(); ++it)
    {

        that->roomManager.BroadcastMessage(it->first,it->second,true);


    }

    that->injectedMessages.clear();

}


int GameServer::ServerThread(void* parama)
{
    GameServer*that = (GameServer*)parama;

    //std::thread checkAlive(CheckAliveThread, that);
    event* keepAliveEvent = event_new(that->net_evbase, 0, EV_TIMEOUT | EV_PERSIST, keepAlive, that);
    timeval timeout = {5, 0};
    event_add(keepAliveEvent, &timeout);

    event* cicle_injected = event_new(that->net_evbase, 0, EV_TIMEOUT | EV_PERSIST, checkInjectedMessages_cb, parama);
    timeval timeout2 = {0, 200000};
    event_add(cicle_injected, &timeout2);


    event_base_dispatch(that->net_evbase);
    event_free(keepAliveEvent);
    event_free(cicle_injected);
    event_base_free(that->net_evbase);
    that->net_evbase = 0;
    //checkAlive.join();
    return 0;
}

bool GameServer::dispatchPM(std::wstring recipient,std::wstring message)
{
    std::transform(recipient.begin(), recipient.end(), recipient.begin(), ::tolower);
    printf("dispatch\n");
    DuelPlayer* dp = findPlayer(recipient);
    if(dp==nullptr)
        return false;

    dp->netServer->SystemChatToPlayer(dp,message,false);
    printf("completed\n");
    return true;
}
    bool GameServer::sendPM(std::wstring recipient,std::wstring message)
    {
        return dispatchPM(recipient,message);


    }


void GameServer::DisconnectPlayer(DuelPlayer* dp)
{
    auto bit = users.find(dp->bev);
    if(bit != users.end())
    {
        if(dp->loginStatus == Users::LoginResult::NOPASSWORD || dp->loginStatus == Users::LoginResult::AUTHENTICATED)
        {
                    wchar_t nome[25];
                    BufferIO::CopyWStr(dp->name,nome,20);
                    std::wstring nomes(nome);
                    std::transform(nomes.begin(), nomes.end(), nomes.begin(), ::tolower);
                    printf("rimuovo %Ls\n",nomes.c_str());
                    if(loggedUsers.find(nomes)!=loggedUsers.end())
                        loggedUsers.erase(nomes);
        }

        dp->netServer=NULL;
        bufferevent_flush(dp->bev, EV_WRITE, BEV_FLUSH);
        bufferevent_disable(dp->bev, EV_READ);
        bufferevent_free(dp->bev);
        users.erase(bit);



    }

    if(listener != nullptr)
        RestartListen();
    Statistics::getInstance()->setNumPlayers(users.size());
}


void GameServer::HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len)
{
    char* pdata = data;

    unsigned char pktType = BufferIO::ReadUInt8(pdata);

    if(dp->netServer == NULL)
    {
        if(pktType==CTOS_PLAYER_INFO)
        {
            CTOS_PlayerInfo* pkt = (CTOS_PlayerInfo*)pdata;
            char name[20];
            BufferIO::CopyWStr(pkt->name,name,20);
            auto result = Users::getInstance()->login(std::string(name),dp->ip);

            BufferIO::CopyWStr(result.first.c_str(), dp->name, 20);
            dp->loginStatus = result.second;

            auto score = Users::getInstance()->getFullScore(result.first);
            dp->cachedRankScore = score.first;
            dp->cachedGameScore = score.second;

            return;
        }
        else if(pktType == CTOS_JOIN_GAME && dp->name[0] != 0)
        {
            if(roomManager.InsertPlayerInWaitingRoom(dp))
            {
                if(dp->loginStatus == Users::LoginResult::NOPASSWORD || dp->loginStatus == Users::LoginResult::AUTHENTICATED)
                {
                    wchar_t nome[25];
                    BufferIO::CopyWStr(dp->name,nome,20);
                    std::wstring nomes(nome);
                    std::transform(nomes.begin(), nomes.end(), nomes.begin(), ::tolower);

                    loggedUsers[nomes] = dp;
                }

            }
            else
                return;
        }
        else if(!strcmp(data,"ping"))
        {

            printf("pong\n");
            bufferevent_write(dp->bev, "pong", 5);
            bufferevent_flush(dp->bev, EV_WRITE, BEV_FLUSH);
            //DisconnectPlayer(dp);
            return;
        }
        else
        {
            log(WARN,"player info is not the first packet\n");
            return;
        }
    }




    int time1 = clock();
    dp->netServer->HandleCTOSPacket(dp,data,len);
    int time2 = clock();
    int diffms=1000*(time2-time1)/(CLOCKS_PER_SEC);
    if(diffms > 500)
        printf("millisecs: %d\n",diffms);
    return;
}

}

