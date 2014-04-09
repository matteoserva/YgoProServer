#include "GameServer.h"
#include "RoomManager.h"
#include "debug.h"
#include "Statistics.h"

namespace ygo
{

int RoomManager::getNumRooms()
{
    return elencoServer.size()+playingServer.size()+zombieServer.size();
}

void RoomManager::setGameServer(GameServer* gs)
{
    gameServer = gs;

    net_evbase = gs->net_evbase;
    timeval timeout = {SecondsBeforeFillAllRooms, 0};
    keepAliveEvent = event_new(net_evbase, 0, EV_TIMEOUT | EV_PERSIST, keepAlive, this);
    waitingRoom = new WaitingRoom(this,gs);
	rematchRoom = new RematchRoom(this,gs);
    event_add(keepAliveEvent, &timeout);
}

RoomManager::RoomManager()
{
    waitingRoom=0;
	rematchRoom = nullptr;
}
RoomManager::~RoomManager()
{
    event_free(keepAliveEvent);
    delete waitingRoom;
	delete rematchRoom;
}


void RoomManager::keepAlive(evutil_socket_t fd, short events, void* arg)
{
    RoomManager*that = (RoomManager*) arg;
    static int needRemove = 0;
    needRemove = (needRemove+1) % RemoveDeadRoomsRatio;
    if(needRemove == 0)
        that->removeDeadRooms();
    that->FillAllRooms();
}

void RoomManager::tryToInsertPlayerInServer(DuelPlayer*dp,DuelRoom* serv)
{
    if(std::find(elencoServer.begin(), elencoServer.end(), serv) == elencoServer.end() ||!serv->isAvailableToPlayer(dp,MODE_ANY))
    {
        printf("Server non disponibile\n");
        waitingRoom->ToObserverPressed(dp);
        return;
    }
    
    waitingRoom->ExtractPlayer(dp);
    serv->InsertPlayer(dp);

}

void RoomManager::notifyStateChange(DuelRoom* room,DuelRoom::State oldstate,DuelRoom::State newstate)
{
    if(newstate == DuelRoom::PLAYING)
    {
        elencoServer.remove(room);
        playingServer.insert(room);
    }
    else if(newstate == DuelRoom::ZOMBIE || newstate == DuelRoom::DEAD)
    {
        if(oldstate == DuelRoom::PLAYING)
            playingServer.erase(room);
        else
            elencoServer.remove(room);
        zombieServer.insert(room);
    }
}

std::vector<DuelRoom *> RoomManager::getCompatibleRoomsList(DuelPlayer *referencePlayer)
{
    std::vector<DuelRoom *> lista;

    for(auto riga:elencoServer)
        if(riga->isAvailableToPlayer(referencePlayer, MODE_ANY))
            lista.push_back(riga);

    return lista;

}

int RoomManager::getNumPlayers()
{
    int risultato = 0;
    risultato += waitingRoom->getNumPlayers();
    for(auto it =elencoServer.begin(); it!=elencoServer.end(); ++it)
    {
        risultato += (*it)->getNumPlayers();
    }
    for(auto it =playingServer.begin(); it!=playingServer.end(); ++it)
    {
        risultato += (*it)->getNumPlayers();
    }
    return risultato;
}

DuelRoom* RoomManager::getFirstAvailableServer(DuelPlayer* referencePlayer)
{
    return getFirstAvailableServer(referencePlayer, MODE_SINGLE,true);
}

DuelRoom* RoomManager::getFirstAvailableServer(DuelPlayer* referencePlayer, unsigned char mode,bool ignoreMode)
{
    /*int lflist = referencePlayer ->lflist;
    int referenceScore = referencePlayer->cachedRankScore;


    for(auto it =elencoServer.begin(); it!=elencoServer.end(); ++it)
    {
        bool serverOk = (*it)->state == CMNetServer::State::WAITING &&
                        abs(referenceScore - (*it)->getFirstPlayer()->cachedRankScore) < maxScoreDifference(referenceScore) &&
                        (ignoreMode || (*it)->mode == mode) && (*it)->getLfList() == lflist;

        if(serverOk && referencePlayer->loginStatus == Users::LoginResult::AUTHENTICATED &&
           (*it)->getFirstPlayer()->loginStatus == Users::LoginResult::AUTHENTICATED && !wcscmp(referencePlayer->namew_low,(*it)->getFirstPlayer()->namew_low))
        {
            serverOk = false;
        }

        if(serverOk )
            return *it;
    }
    for(auto it =elencoServer.begin(); it!=elencoServer.end(); ++it)
    {
        bool serverOk = (*it)->state == CMNetServer::State::WAITING &&
                        abs(referenceScore - (*it)->getFirstPlayer()->cachedRankScore) < maxScoreDifference(referenceScore) &&
                        (ignoreMode || (*it)->mode == mode) && ((*it)->getLfList() == 3 || lflist == 3);

        if(serverOk && referencePlayer->loginStatus == Users::LoginResult::AUTHENTICATED &&
           (*it)->getFirstPlayer()->loginStatus == Users::LoginResult::AUTHENTICATED && !wcscmp(referencePlayer->namew_low,(*it)->getFirstPlayer()->namew_low))
        {
            serverOk = false;
        }

        if(serverOk )
            return *it;
    }*/

    for(auto riga:elencoServer)
        if(riga->isAvailableToPlayer(referencePlayer, ignoreMode?MODE_ANY:mode))
            return riga;
    return createServer(mode);
}

bool RoomManager::InsertPlayer(DuelPlayer*dp)
{

    //tfirst room
    DuelRoom* netServer = getFirstAvailableServer(dp);
    if(netServer == nullptr)
    {
        waitingRoom->InsertPlayer(dp);
        waitingRoom->SendMessageToPlayer(dp,"The server is full, please wait.");
        //gameServer->DisconnectPlayer(dp);
        return false;
    }
    dp->netServer=netServer;
    netServer->InsertPlayer(dp);
    FillRoom(netServer);
    return true;
}

bool RoomManager::FillRoom(DuelRoom* room)
{

    if(room->state!= DuelRoom::State::WAITING)
        return true;

    for(DuelPlayer* base = room->getFirstPlayer(); room->state== DuelRoom::State::WAITING;)
    {
        DuelPlayer* dp = waitingRoom->ExtractBestMatchPlayer(base,room->getLfList(),room->mode);
        if(dp == nullptr)
            return false;
        room->InsertPlayer(dp);

    }
    return true;
}


void RoomManager::BroadcastMessage(std::wstring message, int color,RoomInterface* origin)
{
	if(message.length() > 250)
        return;
	for(auto it =elencoServer.begin(); it!=elencoServer.end(); ++it)
    {
        if((*it) != origin)
            (*it)->BroadcastRemoteChat(message,color);
			if((*it)->state == DuelRoom::State::ZOMBIE )
				break; //MEGABUG DETECTED
    }
    for(auto it =playingServer.begin(); it!=playingServer.end(); ++it)
    {
        if((*it) != origin)
            (*it)->BroadcastRemoteChat(message,color);
    }
    if(waitingRoom != origin)
        waitingRoom->BroadcastRemoteChat(message,color);
	if(origin != nullptr)
        gameServer->callChatCallback(message,color);
}



bool RoomManager::FillAllRooms()
{
    for(auto it =elencoServer.begin(); it!=elencoServer.end(); ++it)
    {
        DuelRoom *p = *it;
        if(p->state == DuelRoom::State::WAITING )
        {
            bool result = FillRoom(p);
			if(p->state != DuelRoom::State::WAITING && p->state != DuelRoom::State::FULL )
				return false; //MEGABUG DETECTED
				
            //if(!result)
            //  return false;
        }
    }
    return true;

}

void RoomManager::ban(std::string ip)
{
	time_t now = time(NULL);
	int interval = 30;
	
	for(auto it = bannedIPs.begin();it!=bannedIPs.end();)
		if(now - it->second > interval)
			it = bannedIPs.erase(it);
		else
			++it;
			
	printf("scatta il ban per %s\n",ip.c_str());
   bannedIPs.insert(std::pair<std::string,time_t>(ip,now));
}
bool RoomManager::isBanned(std::string ip)
{
	
	auto ret = bannedIPs.equal_range(ip);
	int count = 0;
	for(auto it = ret.first;it!=ret.second;++it)
		count++;
	
    return count>1;
}

bool RoomManager::checkSpam(DuelPlayer*dp,std::wstring messaggio)
{
	dp->chatTimestamp.push_back(std::pair<time_t,std::wstring>(time(NULL),messaggio));
	if(dp->chatTimestamp.size() < 3)
		return false;
	
	auto it = dp->chatTimestamp.begin();
	auto primo = *(++it);
	auto secondo = *(++it);
	
	bool daBannare = false;
	
	
	if(messaggio == primo.second && messaggio == secondo.second)
	{
		daBannare = true;
	}
	else if(dp->chatTimestamp.size() > 5)
	{	
		if(dp->chatTimestamp.back().first - dp->chatTimestamp.front().first <5)
			daBannare = true;
		dp->chatTimestamp.pop_front();
	} 
	if(daBannare)
	{
		wchar_t name[20];

		BufferIO::CopyWStr(dp->name, name, 20);
		ban(std::string(dp->ip));
		dp->color = -3;
		std::wstring banmessage;
		if(isBanned(dp->ip))
			banmessage = std::wstring(name) + std::wstring(L" is BANNED for spamming!");
		else
			banmessage = std::wstring(name) + std::wstring(L" is muted for spamming!");

		BroadcastMessage(banmessage,-1);
		
	}
	
	return daBannare;
}

bool RoomManager::InsertPlayerInWaitingRoom(DuelPlayer*dp)
{
    //true is success

    RoomInterface* netServer = waitingRoom;
    if(netServer == nullptr || isBanned(std::string(dp->ip)))
    {
        gameServer->DisconnectPlayer(dp);
        return false;
    }
    netServer->InsertPlayer(dp);
    return true;
}

bool RoomManager::InsertPlayer(DuelPlayer*dp,unsigned char mode)
{

    //true is success
    DuelRoom* netServer = getFirstAvailableServer(dp,mode,false);
    if(netServer == nullptr)
    {
        waitingRoom->InsertPlayer(dp);
        waitingRoom->SendMessageToPlayer(dp,"The server is full, please wait.");
        return false;
    }

    dp->netServer=netServer;
    netServer->InsertPlayer(dp);
    FillRoom(netServer);
    return true;
}


DuelRoom* RoomManager::createServer(unsigned char mode)
{
    
    DuelRoom *netServer = new DuelRoom(this,gameServer,mode);

    elencoServer.push_back(netServer);

    Statistics::getInstance()->setNumRooms(getNumRooms());

    return netServer;
}

void RoomManager::removeDeadRooms()
{

    int i=0;
    //log(INFO,"analizzo la lista server e cerco i morti\n");
    for(auto it =zombieServer.begin(); it!=zombieServer.end();)
    {
        DuelRoom *p = *it;

        if(p->state == DuelRoom::State::DEAD)
        {
            log(VERBOSE,"elimino il server %d\n",i);
            delete (*it);
            it=zombieServer.erase(it);
        }
        else
        {
			
				auto d = p->ExtractAllPlayers();
				rematchRoom->createRoom(d,p->mode,p->getMaxDuelPlayers());
			
			
            ++it;
        }

        i++;
    }
    Statistics::getInstance()->setNumRooms(getNumRooms());
}

void RoomManager::HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len)
{

    
    char* pdata = data;
    unsigned char pktType = BufferIO::ReadUInt8(pdata);
    switch(pktType)
    {
    case CTOS_CHAT:
    {
        wchar_t messaggio[256];

        int msglen = BufferIO::CopyWStr((unsigned short*) pdata,messaggio, 256);
		if(msglen < 1)
			return;
			
		//ora sappiamo che abbiamo almeno un carattere
		unsigned short* msgbuf = (unsigned short*)pdata;
		
		
		//e' un comando, non un messaggio chat
		if(msgbuf[0]=='!')
		{
				if(!dp->netServer->handleChatCommand(dp,messaggio))
				dp->netServer->RemoteChatToPlayer(dp,L"Unrecognized command",-1);
				return;
		}

		if(dp->color == -3)
			return;
			
		bool daBroadcastare = 	msgbuf[0]!='-' && (dp->loginStatus == Users::LoginResult::AUTHENTICATED || dp->loginStatus == Users::LoginResult::NOPASSWORD);
        if(dp->netServer == waitingRoom ||daBroadcastare)
        { //se dobbiamo fare un controllo spam
			if(checkSpam(dp,messaggio))
				return;
		}
		if( daBroadcastare)
		{  //se dobbiamo broadcastare
		
			wchar_t name[25];

			BufferIO::CopyWStr(dp->name, name, 20);
			std::wstring tmp(dp->countryCode.begin(),dp->countryCode.end());
			tmp = L"<"+tmp+L">";
			wcscat(name,tmp.c_str());

			std::wstring sender(name);
			std::wstring message(messaggio);
			if(sender!=L"")
				message = L"["+sender+L"]: "+message;
			BroadcastMessage(message,0,dp->netServer);
		
		}
		
        if(dp->color > 0)
        {

            wchar_t stringa[256];
            stringa[0] = '[';
            stringa[1] = 0;
            BufferIO::CopyWStr(dp->name,&stringa[1],25);
            wcscat(stringa,L"]: ");
            wcsncat(stringa,messaggio,220);
			dp->netServer->BroadcastRemoteChat(stringa,dp->color);
            return;
        }
		dp->netServer->RoomChat(dp,std::wstring(messaggio));
		return;
    }
    }
	dp->netServer->HandleCTOSPacket(dp,data,len);

}

}
