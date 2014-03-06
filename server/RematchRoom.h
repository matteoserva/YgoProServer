#ifndef REMATCH_H
#define REMATCH_H

#include "RoomInterface.h"
#include "RoomManager.h"
#include <list>

namespace ygo
{
	struct VirtualRoom
	{
		std::map<DuelPlayer*, DuelPlayerInfo> players;
		unsigned char mode;
		
	};
	
	
	class RematchRoom:public RoomInterface
	{
		std::list<VirtualRoom> virtualRooms;
		public:
		RematchRoom(RoomManager*roomManager,GameServer*gameServer);
		void LeaveGame(DuelPlayer* dp);
		void InsertPlayer(DuelPlayer* dp);
		void ExtractPlayer(DuelPlayer* dp);
		void HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len);
		void RoomChat(DuelPlayer* dp, std::wstring messaggio);
		void createRoom(std::map<DuelPlayer*, DuelPlayerInfo>, unsigned char mode,int required);
		~RematchRoom();
	private:
		void killRoom(std::list<VirtualRoom>::iterator r);
		std::list<VirtualRoom>::iterator getRoomByDp(DuelPlayer*);
	};
	
}

#endif