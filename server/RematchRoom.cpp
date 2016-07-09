#include "RematchRoom.h"
#include "GameServer.h"
namespace ygo
{

RematchRoom::RematchRoom(RoomManager*roomManager,GameServer*gameServer)
	:RoomInterface(roomManager,gameServer)
{

}

void RematchRoom::killRoom(std::list<VirtualRoom>::iterator r)
{
	if(r == virtualRooms.end())
		return;
	char buffer[10];
	char* pbuf = buffer;
		BufferIO::WriteInt16(pbuf, 1);
		BufferIO::WriteInt8(pbuf, STOC_DUEL_END);
	
	for(auto it = r->players.cbegin();it!=r->players.cend();++it)
	{
		bufferevent_write(it->first->bev, buffer, 3);
		
	}
	virtualRooms.erase(r);
	
	
}
std::list<VirtualRoom>::iterator RematchRoom::getRoomByDp(DuelPlayer* dp)
{
	for(auto it = virtualRooms.begin();it!=virtualRooms.end();++it)
		if(it->players.find(dp) != it->players.end())
			return it;
	dp->netServer = nullptr;
	gameServer->DisconnectPlayer(dp);
	return virtualRooms.end();
}
void RematchRoom::createRoom(std::map<DuelPlayer*, DuelPlayerInfo> p, unsigned char mode,int required)
{
	int count = 0;
	for(auto it = p.begin();it!=p.end();++it)
	{
		it->second.isReady=false;
		if(it->first->type < NETPLAYER_TYPE_OBSERVER)
			count++;
		it->first->netServer = this;
	}

	VirtualRoom vr = {p,mode};
	virtualRooms.push_front(vr);
	
	if(count != required)
		killRoom(virtualRooms.begin());
	else
	{
		char buffer[10];
		char* pbuf = buffer;
		BufferIO::WriteInt16(pbuf, 7);
		BufferIO::WriteInt8(pbuf, STOC_GAME_MSG);
		buffer[3] = MSG_SELECT_YESNO;
		buffer[4] = 0;
		buffer[5] = 30;
		buffer[6] = buffer[7] = buffer[8] = 0;
		for(auto it = p.begin();it!=p.end();++it)
		{
			if(it->first->type < NETPLAYER_TYPE_OBSERVER)
			{
				bufferevent_write(it->first->bev, buffer, 9);
				it->first->state = 0xff;
			}
				
		}
	}
}

void RematchRoom::LeaveGame(DuelPlayer* dp)
{
	auto it = getRoomByDp(dp);
	if(it== virtualRooms.end())
		return;
	
	if(dp->type == NETPLAYER_TYPE_OBSERVER)
		it->players.erase(dp);
	else
		killRoom(it);
	gameServer->DisconnectPlayer(dp);
}		

void RematchRoom::InsertPlayer(DuelPlayer* dp)
{

}
void RematchRoom::ExtractPlayer(DuelPlayer* dp)
{

}
void RematchRoom::HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len)
{
	


	if(data[0] != CTOS_RESPONSE)
		return;
		
	auto it = getRoomByDp(dp);
	if(it== virtualRooms.end())
		return;
	if(data[1] == 0)
	{
		killRoom(it);
		
	}
	else
	{
		it->players[dp].isReady = true;
		int count = 0;
		for(auto i = it->players.cbegin();i!=it->players.cend(); ++i)
			if(!i->second.isReady && i->first->type < NETPLAYER_TYPE_OBSERVER)
				return;
		
		DuelRoom* dr = roomManager->createServer(it->mode);
		
		for(count = 0;count < 4;count++)
			for(auto i = it->players.cbegin();i!=it->players.cend(); ++i)
				if(i->first->type == count)
					dr->InsertPlayer(i->first);
		for(auto i = it->players.cbegin();i!=it->players.cend(); ++i)
				if(i->first->type ==NETPLAYER_TYPE_OBSERVER)
					dr->InsertPlayer(i->first);
		char readyMSG = CTOS_HS_READY;
		for(auto i = it->players.begin();i!=it->players.end(); ++i)
				if(i->first->type !=NETPLAYER_TYPE_OBSERVER)
				{
					
					i->first->state = 0;
					dr->HandleCTOSPacket(i->first,i->second.deck,1024);
					dr->HandleCTOSPacket(i->first,&readyMSG,1);
					
				}
					
		printf("--rematch richiesto\n");
		virtualRooms.erase(it);
	}
	
   
	
	
}
void RematchRoom::RoomChat(DuelPlayer* dp, std::wstring messaggio)
{
	auto it = getRoomByDp(dp);
	if(it== virtualRooms.end())
		return;
	STOC_Chat scc;
    scc.player = dp->type;
    int msglen = BufferIO::CopyWStr(messaggio.c_str(), scc.msg, 256);
	
	char buffer[1024];
	char *pbuf = buffer;
	msglen = 4 + msglen*2;
	BufferIO::WriteInt16(pbuf, 1 + msglen);
    BufferIO::WriteInt8(pbuf, STOC_CHAT);
	memcpy(pbuf,&scc,msglen);
	
	for(auto p = it->players.cbegin();p!=it->players.cend();p++)
		if(p->first != dp)
			bufferevent_write(p->first->bev, buffer, 3+msglen);
	
}




RematchRoom::~RematchRoom()
{

}

}
