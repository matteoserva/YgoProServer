#ifndef REMATCH_H
#define REMATCH_H

#include "RoomInterface.h"
#include "RoomManager.h"

namespace ygo
{
	class RematchRoom:public RoomInterface
	{
		public:
		RematchRoom(RoomManager*roomManager,GameServer*gameServer);
		void LeaveGame(DuelPlayer* dp);
		void InsertPlayer(DuelPlayer* dp);
		void ExtractPlayer(DuelPlayer* dp);
		void HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len);
		void RoomChat(DuelPlayer* dp, std::wstring messaggio);
		void createRoom(std::map<DuelPlayer*, DuelPlayerInfo>, unsigned char mode,int required);
		~RematchRoom();
	};
	
}

#endif