#ifndef _SERVER_PROTOCOL_H
#define _SERVER_PROTOCOL_H
//#include "Users.h"
#include <netinet/in.h>

#include "common/common_structures.h"


namespace protocol
{


enum MessageType {STATS,CHAT,LOGINREQUEST,LOGINRESULT,DUELRESULT};


struct GameServerStats
{
	MessageType type;
	int pid;
	int rooms;
	int players;
	unsigned int max_users;
	bool isAlive;
	
};

struct GameServerChat
{
	MessageType type;
	int chatColor;
	wchar_t messaggio[260];
};

struct LoginRequest
{
	MessageType type;
	char loginstring[45];
	char ip[INET_ADDRSTRLEN];
	void *handle;
	
};

struct LoginResult
{
	MessageType type;
	void *handle;
	char name[25];
	char countryCode[3];
	signed char color;
	unsigned int cachedRank;
    unsigned int cachedScore;
	ygo::LoginResult loginStatus;
};

struct DuelResult
{
	MessageType type;
	LoggerPlayerData players[4];
	unsigned int num_players;
	char replayCode[12];
	char winner;
};



}

#endif
