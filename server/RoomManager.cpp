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
    event_add(keepAliveEvent, &timeout);
}

RoomManager::RoomManager()
{
    waitingRoom=0;
}
RoomManager::~RoomManager()
{
    event_free(keepAliveEvent);
    delete waitingRoom;
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

void RoomManager::tryToInsertPlayerInServer(DuelPlayer*dp,CMNetServer* serv)
{
    if(std::find(elencoServer.begin(), elencoServer.end(), serv) == elencoServer.end())
    {
        printf("serv non trovato\n");
        waitingRoom->ToObserverPressed(dp);
        return;
    }
    if(serv->state != CMNetServer::State::WAITING)
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

void RoomManager::notifyStateChange(CMNetServer* room,CMNetServer::State oldstate,CMNetServer::State newstate)
{
    if(newstate == CMNetServer::PLAYING)
    {
        elencoServer.remove(room);
        playingServer.insert(room);
    }
    else if(newstate == CMNetServer::ZOMBIE || newstate == CMNetServer::DEAD)
    {
        if(oldstate == CMNetServer::PLAYING)
            playingServer.erase(room);
        else
            elencoServer.remove(room);
        zombieServer.insert(room);
    }
}

std::vector<CMNetServer *> RoomManager::getCompatibleRoomsList(int referenceScore)
{
    std::vector<CMNetServer *> lista;
    int maxqdifference = maxScoreDifference(referenceScore);

    for(auto it =elencoServer.begin(); it!=elencoServer.end(); ++it)
    {
        if((*it)->state == CMNetServer::State::WAITING && abs(referenceScore - (*it)->getFirstPlayer()->cachedRankScore) < maxqdifference)
        {
            lista.push_back(*it);
            log(VERBOSE,"roommanager, trovato server\n");
        }
    }
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

CMNetServer* RoomManager::getFirstAvailableServer(int lflist, int referenceScore)
{
    return getFirstAvailableServer(lflist, referenceScore, MODE_SINGLE,true);
}

CMNetServer* RoomManager::getFirstAvailableServer(int lflist, int referenceScore, unsigned char mode,bool ignoreMode)
{
    for(auto it =elencoServer.begin(); it!=elencoServer.end(); ++it)
    {
        bool serverOk = (*it)->state == CMNetServer::State::WAITING &&
                        abs(referenceScore - (*it)->getFirstPlayer()->cachedRankScore) < maxScoreDifference(referenceScore) &&
                        (ignoreMode || (*it)->mode == mode) && (*it)->getLfList() == lflist;

        if(serverOk )
            return *it;
    }
    for(auto it =elencoServer.begin(); it!=elencoServer.end(); ++it)
    {
        bool serverOk = (*it)->state == CMNetServer::State::WAITING &&
                        abs(referenceScore - (*it)->getFirstPlayer()->cachedRankScore) < maxScoreDifference(referenceScore) &&
                        (ignoreMode || (*it)->mode == mode) && ((*it)->getLfList() == 3 || lflist == 3);

        if(serverOk )
            return *it;
    }
    return createServer(mode);
}

bool RoomManager::InsertPlayer(DuelPlayer*dp)
{

    //tfirst room
    CMNetServer* netServer = getFirstAvailableServer(dp->lflist,dp->cachedRankScore);
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

bool RoomManager::FillRoom(CMNetServer* room)
{

    if(room->state!= CMNetServer::State::WAITING)
        return true;

    for(DuelPlayer* base = room->getFirstPlayer(); room->state!= CMNetServer::State::FULL;)
    {
        DuelPlayer* dp = waitingRoom->ExtractBestMatchPlayer(base,room->getLfList());
        if(dp == nullptr)
            return false;
        dp->netServer=room;
        room->InsertPlayer(dp);

    }
    return true;
}

void RoomManager::BroadcastMessage(std::wstring message, bool isAdmin,bool crossServer)
{
    if(message.length() > 250)
        return;
    for(auto it =elencoServer.begin(); it!=elencoServer.end(); ++it)
    {
        (*it)->BroadcastSystemChat(message,isAdmin);
    }
    for(auto it =playingServer.begin(); it!=playingServer.end(); ++it)
    {
        (*it)->BroadcastSystemChat(message,isAdmin);
    }
    waitingRoom->BroadcastSystemChat(message,isAdmin);

    if(!crossServer)
        gameServer->callChatCallback(message,isAdmin);
}

bool RoomManager::FillAllRooms()
{
    for(auto it =elencoServer.begin(); it!=elencoServer.end(); ++it)
    {
        CMNetServer *p = *it;
        if(p->state == CMNetServer::State::WAITING )
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
    bannedIPs.insert(ip);
}
bool RoomManager::isBanned(std::string ip)
{
    return bannedIPs.find(ip) != bannedIPs.end();
}

bool RoomManager::InsertPlayerInWaitingRoom(DuelPlayer*dp)
{
    //true is success

    CMNetServerInterface* netServer = waitingRoom;
    if(netServer == nullptr || isBanned(std::string(dp->ip)))
    {
        gameServer->DisconnectPlayer(dp);
        return false;
    }
    dp->netServer = netServer;
    return true;
}

bool RoomManager::InsertPlayer(DuelPlayer*dp,unsigned char mode)
{

    //true is success
    CMNetServer* netServer = getFirstAvailableServer(dp->lflist,dp->cachedRankScore,mode,false);
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


CMNetServer* RoomManager::createServer(unsigned char mode)
{
    if(getNumRooms() >= 500)
    {
        return nullptr;
    }
    CMNetServer *netServer = new CMNetServer(this,gameServer,mode);

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
        CMNetServer *p = *it;

        if(p->state == CMNetServer::State::DEAD)
        {
            log(VERBOSE,"elimino il server %d\n",i);
            delete (*it);
            it=zombieServer.erase(it);
        }
        else
        {
            ++it;
        }

        i++;
    }
    Statistics::getInstance()->setNumRooms(getNumRooms());
}


}
