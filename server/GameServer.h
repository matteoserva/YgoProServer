#ifndef GAMESERVER_H
#define GAMESERVER_H

#include "config.h"
#include "network.h"
#include "data_manager.h"
#include "deck_manager.h"
#include <set>
#include <unordered_map>
#include "RoomManager.h"

#include "netserver.h"
namespace ygo {

class GameServer {
private:
	std::unordered_map<bufferevent*, DuelPlayer> users;
	 unsigned short server_port;
	 event_base* net_evbase;
	 event* broadcast_ev;
	 evconnlistener* listener;
	 DuelMode* duel_mode;
	 char net_server_read[0x2000];
	 char net_server_write[0x2000];
	 unsigned short last_sent;

public:
    RoomManager roomManager;
    GameServer();
	 bool StartServer(unsigned short port);
	 bool StartBroadcast();
	 void StopServer();
	 void StopBroadcast();
	 void StopListen();
	 void BroadcastEvent(evutil_socket_t fd, short events, void* arg);
	 static void ServerAccept(evconnlistener* listener, evutil_socket_t fd, sockaddr* address, int socklen, void* ctx);
	 static void ServerAcceptError(evconnlistener *listener, void* ctx);
	 static void ServerEchoRead(bufferevent* bev, void* ctx);
	 static void ServerEchoEvent(bufferevent* bev, short events, void* ctx);
	 static int ServerThread(void* param);
	 void DisconnectPlayer(DuelPlayer* dp);
	 void HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len);
	 void SendPacketToPlayer(DuelPlayer* dp, unsigned char proto) {
		char* p = net_server_write;
		BufferIO::WriteInt16(p, 1);
		BufferIO::WriteInt8(p, proto);
		last_sent = 3;
		if(!dp)
			return;
		bufferevent_write(dp->bev, net_server_write, last_sent);
	}
	bool handleChatCommand(DuelPlayer* dp,unsigned short* msg);

	template<typename ST>
	 void SendPacketToPlayer(DuelPlayer* dp, unsigned char proto, ST& st) {
		char* p = net_server_write;
		BufferIO::WriteInt16(p, 1 + sizeof(ST));
		BufferIO::WriteInt8(p, proto);
		memcpy(p, &st, sizeof(ST));
		last_sent = sizeof(ST) + 3;
		if(dp)
			bufferevent_write(dp->bev, net_server_write, last_sent);
	}
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
