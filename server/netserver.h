#ifndef NETSERVER_H
#define NETSERVER_H

#include "config.h"
#include "network.h"
#include "data_manager.h"
#include "deck_manager.h"
#include <set>
#include <unordered_map>
#include <map>

namespace ygo {
class GameServer;
class RoomManager;

class DuelPlayerInfo{


};

class CMNetServer {
private:

    unsigned char mode;
	 unsigned short server_port;
        RoomManager* roomManager;
	 event* broadcast_ev;
	 evconnlistener* listener;
	 DuelMode* duel_mode;
	 char net_server_read[0x2000];
	 char net_server_write[0x2000];
	 unsigned short last_sent;
     int numPlayers;
     std::map<DuelPlayer*, DuelPlayerInfo> players;
     void playerConnected(DuelPlayer* dp);
     void playerDisconnected(DuelPlayer* dp);
    int getMaxPlayers();
    void clientStarted();
public:

    enum State {WAITING,FULL,PLAYING,ZOMBIE};
    State state;
    GameServer* gameServer;
    CMNetServer(RoomManager*roomManager,unsigned char mode);


     void LeaveGame(DuelPlayer* dp);
	 bool StartServer(unsigned short port);
	 bool StartBroadcast();
	 void StopServer();
	 void StopBroadcast();
	 void StopListen();


         void createGame();
	 void BroadcastEvent(evutil_socket_t fd, short events, void* arg);
	 static void ServerAccept(evconnlistener* listener, evutil_socket_t fd, sockaddr* address, int socklen, void* ctx);
	 static void ServerAcceptError(evconnlistener *listener, void* ctx);
	 static void ServerEchoRead(bufferevent* bev, void* ctx);
	 static void ServerEchoEvent(bufferevent* bev, short events, void* ctx);
	 void DisconnectPlayer(DuelPlayer* dp);
	 void HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len);
	 void SendMessageToPlayer(DuelPlayer*dp, char*msg);
	 void SendPacketToPlayer(DuelPlayer* dp, unsigned char proto);
	template<typename ST>
	 void SendPacketToPlayer(DuelPlayer* dp, unsigned char proto, ST& st){
        char* p = net_server_write;
		BufferIO::WriteInt16(p, 1 + sizeof(ST));
		BufferIO::WriteInt8(p, proto);
		memcpy(p, &st, sizeof(ST));
		last_sent = sizeof(ST) + 3;
		if(dp)
			bufferevent_write(dp->bev, net_server_write, last_sent);

	 }
	 void InsertPlayer(DuelPlayer* dp);
	 void ExtractPlayer(DuelPlayer* dp);
	 void SendBufferToPlayer(DuelPlayer* dp, unsigned char proto, void* buffer, size_t len) {

		char* p = net_server_write;
		BufferIO::WriteInt16(p, 1 + len);
		BufferIO::WriteInt8(p, proto);
		memcpy(p, buffer, len);
		last_sent = len + 3;
		if(dp)
			bufferevent_write(dp->bev, net_server_write, last_sent);
	}
	 void ReSendToPlayer(DuelPlayer* dp) {

		if(dp)
			bufferevent_write(dp->bev, net_server_write, last_sent);
	}
};

}

#endif //NETSERVER_H
