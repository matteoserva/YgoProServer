#include "RematchRoom.h"

namespace ygo
{

RematchRoom::RematchRoom(RoomManager*roomManager,GameServer*gameServer)
	:RoomInterface(roomManager,gameServer)
{

}

void RematchRoom::createRoom(std::map<DuelPlayer*, DuelPlayerInfo> p, unsigned char mode,int required)
{

	DuelRoom* dr = roomManager->createServer( mode);
	for(auto it = p.cbegin();it!=p.cend();++it)
		dr->InsertPlayer(it->first);


}

void RematchRoom::LeaveGame(DuelPlayer* dp)
{

}		

void RematchRoom::InsertPlayer(DuelPlayer* dp)
{

}
void RematchRoom::ExtractPlayer(DuelPlayer* dp)
{

}
void RematchRoom::HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len)
{

}
void RematchRoom::RoomChat(DuelPlayer* dp, std::wstring messaggio)
{

}




RematchRoom::~RematchRoom()
{

}

}
