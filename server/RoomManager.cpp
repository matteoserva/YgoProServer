#include "GameServer.h"
#include "RoomManager.h"
#include "debug.h"
#include "Statistics.h"

namespace ygo
{

int RoomManager::getNumRooms()
{
    log(VERBOSE,"pronti %d, giocanti %d, zombie %d\n",elencoServer.size(),playingServer.size(),zombieServer.size());
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
    if(std::find(elencoServer.begin(), elencoServer.end(), serv) == elencoServer.end())
    {
        printf("serv non trovato\n");
        waitingRoom->ToObserverPressed(dp);
        return;
    }
    if(serv->state != DuelRoom::State::WAITING)
    {
        printf("serv pieno\n");
        waitingRoom->ToObserverPressed(dp);
        return;
    }
    if(serv->getFirstPlayer() != nullptr && abs(dp->cachedRankScore - serv->getFirstPlayer()->cachedRankScore) > maxScoreDifference(dp->cachedRankScore))
    {
        printf("serv non piu' compatibile, cached %d, servscore %d\n",dp->cachedRankScore,serv->getFirstPlayer()->cachedRankScore);
        waitingRoom->ToObserverPressed(dp);
        return;
    }

    waitingRoom->ExtractPlayer(dp);
    dp->netServer=serv;
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
        dp->netServer=room;
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
			
				
            //if(!result)
            //  return false;
        }
    }
    return true;

}

void RoomManager::ban(std::string ip)
{
    if(ip != "127.0.0.1")
        bannedIPs.insert(ip);
}
bool RoomManager::isBanned(std::string ip)
{
    return bannedIPs.find(ip) != bannedIPs.end();
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
    dp->netServer = netServer;
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
    if(getNumRooms() >= 500)
    {
        return nullptr;
    }
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

			dp->chatTimestamp.push_back(time(NULL));
			if(dp->chatTimestamp.size() > 5)
			{
				if(dp->chatTimestamp.back() - dp->chatTimestamp.front() <5)
				{
					wchar_t name[20];

					BufferIO::CopyWStr(dp->name, name, 20);
					std::wstring banmessage = std::wstring(name) + std::wstring(L" is muted for spamming!");
					//ban(std::string(dp->ip));
					dp->color = -3;
					BroadcastMessage(banmessage,-1);
					//std::cout<<"banned: "<<std::string(dp->ip)<<std::endl;
					//dp->netServer->LeaveGame(dp);
					return;
				}
				else
					dp->chatTimestamp.pop_front();
			}
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
            wcscat(stringa,messaggio);
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
