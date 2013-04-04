#ifndef GAMESERVER_H
#define GAMESERVER_H

#include "Config.h"
#include "network.h"
#include "data_manager.h"
#include "deck_manager.h"
#include <set>
#include <unordered_map>
#include "RoomManager.h"

#include "netserver.h"
namespace ygo
{

typedef void (*ChatCallback)(std::wstring ,bool ,void* );


class GameServer
{
private:
    ChatCallback chat_cb;
    void* chat_cb_ptr;


    int MAXPLAYERS;
    std::unordered_map<bufferevent*, DuelPlayer> users;
    std::map<std::wstring,DuelPlayer*> loggedUsers;


    unsigned short server_port;
    evconnlistener* listener;
    int server_fd;
    char net_server_read[0x2000];
    char net_server_write[0x2000];
    unsigned short last_sent;
    //event* keepAliveEvent;
    volatile bool isAlive;
    static void keepAlive(evutil_socket_t fd, short events, void* arg);
    //static int CheckAliveThread(void* parama);
    void RestartListen();
    bool isListening;

    std::list<std::pair<std::wstring,bool>> injectedMessages;
    std::mutex injectedMessages_mutex;

public:
    void injectChatMessage(std::wstring a,bool b);
    void callChatCallback(std::wstring a,bool b);
    void setChatCallback(ChatCallback,void*);

    static void checkInjectedMessages_cb(evutil_socket_t fd, short events, void* arg);


    DuelPlayer* findPlayer(std::wstring);
    event_base* volatile net_evbase;
    RoomManager roomManager;
    GameServer(int server_fd);
    ~GameServer();
    bool StartServer();
    void StopServer();
    void StopListen();
    static void ServerAccept(evconnlistener* listener, evutil_socket_t fd, sockaddr* address, int socklen, void* ctx);
    static void ServerAcceptError(evconnlistener *listener, void* ctx);
    static void ServerEchoRead(bufferevent* bev, void* ctx);
    static void ServerEchoEvent(bufferevent* bev, short events, void* ctx);
    static int ServerThread(void* param);
    void DisconnectPlayer(DuelPlayer* dp);
    void HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len);
    static int CheckAliveThread(void* parama);
    int getNumPlayers();
};

}

#endif //NETSERVER_H
