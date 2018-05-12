#ifndef GAMESERVER_H
#define GAMESERVER_H

#include "common/Config.h"
#include "network.h"
#include "data_manager.h"
#include "deck_manager.h"
#include <set>
#include <unordered_map>
#include "RoomManager.h"

#include "DuelRoom.h"

#include "common/server_protocol.h"

namespace protocol
{
	class Connector;
	
}


namespace ygo
{





class GameServer
{
private:
	protocol::Connector* manager_connector;
    struct bufferevent * manager_buf;
    unsigned int MAXPLAYERS;
    std::map<bufferevent*, DuelPlayer *> users;
    std::map<std::wstring,DuelPlayer*> loggedUsers;


    evconnlistener* listener;
    char net_server_read[0x2000];
    char net_server_write[0x2000];
    unsigned short last_sent;
    //event* keepAliveEvent;
    volatile bool isAlive;
    static void keepAlive(evutil_socket_t fd, short events, void* arg);
    static void sendStats(evutil_socket_t fd, short events, void* arg);
    //static int CheckAliveThread(void* parama);
    void RestartListen();
    bool isListening;

	static void handleManagerMessage(int client, void *, unsigned int , void* handle);
	
    bool dispatchPM(std::wstring,std::wstring);
	void createManagerBufferEvent(int);

public:
    void callChatCallback(std::wstring a,int color);
	void saveDuelResult(std::vector<LoggerPlayerData> nomi, int risultato,std::string replay);


    DuelPlayer* findPlayer(std::wstring);
    event_base* volatile net_evbase;
    RoomManager roomManager;
    GameServer();
    ~GameServer();
    bool StartServer(int,protocol::Connector*);
    void StopServer();
    void StopListen();
    static void ServerAccept(evconnlistener* listener, evutil_socket_t fd, sockaddr* address, int socklen, void* ctx);
    static void ServerAcceptError(evconnlistener *listener, void* ctx);
    static void ServerEchoRead(bufferevent* bev, void* ctx);
    static void ServerEchoEvent(bufferevent* bev, short events, void* ctx);
    static void ManagerRead(bufferevent* bev, void* ctx);
    static void ManagerEvent(bufferevent* bev, short events, void* ctx);
    static int ServerThread(void* param);
    void DisconnectPlayer(DuelPlayer* dp);
    void HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len);
	void handleLoginResult(protocol::LoginResult *l);
    static int CheckAliveThread(void* parama);
    int getNumPlayers();

    bool sendPM(std::wstring,std::wstring);
	void safe_bufferevent_write(DuelPlayer* dp, void* buffer, size_t len);

};

}

#endif //NETSERVER_H
