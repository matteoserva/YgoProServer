
#include "GameServer.h"
#include "RoomManager.h"
#include "Statistics.h"
#include "debug.h"
namespace ygo
{



void RoomManager::setGameServer(GameServer* gs)
{
    gameServer = gs;

    net_evbase = gs->net_evbase;
    timeval timeout = {5, 0};
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
    that->removeDeadRooms();
    that->FillAllRooms();
}


int RoomManager::getNumPlayers()
{
    int risultato = 0;
    risultato += waitingRoom->getNumPlayers();
    for(auto it =elencoServer.begin(); it!=elencoServer.end(); ++it)
    {
        risultato += (*it)->getNumPlayers();
    }
    return risultato;
}

CMNetServer* RoomManager::getFirstAvailableServer(unsigned char mode)
{
    int i = 0;
    log(INFO,"analizzo la lista server\n");
    for(auto it =elencoServer.begin(); it!=elencoServer.end(); ++it)
    {
        CMNetServer *p = *it;

        if(p->state == CMNetServer::State::WAITING && p->mode == mode)
        {
            log(INFO,"ho scelto il server %d\n",i);
            return *it;
        }
        i++;
    }
    log(INFO,"Server non trovato, creo uno nuovo \n");
    return createServer(mode);
}

bool RoomManager::InsertPlayer(DuelPlayer*dp)
{

    //tfirst room
    CMNetServer* netServer = getFirstAvailableServer();
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
        DuelPlayer* dp = waitingRoom->ExtractBestMatchPlayer(base);
        if(dp == nullptr)
            return false;
        dp->netServer=room;
        room->InsertPlayer(dp);

    }
    return true;
}

bool RoomManager::FillAllRooms()
{
    for(auto it =elencoServer.begin(); it!=elencoServer.end(); ++it)
    {
        CMNetServer *p = *it;
        if(p->state == CMNetServer::State::WAITING )
        {
            bool result = FillRoom(p);
            if(!result)
                return false;
        }
    }
    return true;

}

bool RoomManager::InsertPlayerInWaitingRoom(DuelPlayer*dp)
{
    //true is success

    CMNetServerInterface* netServer = waitingRoom;
    if(netServer == NULL)
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
    CMNetServer* netServer = getFirstAvailableServer(mode);
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

CMNetServer* RoomManager::getFirstAvailableServer()
{
    int i = 0;
    log(INFO,"analizzo la lista server\n");
    for(auto it =elencoServer.begin(); it!=elencoServer.end(); ++it)
    {
        CMNetServer *p = *it;
        if(p->state == CMNetServer::State::WAITING)
        {
            log(INFO,"ho scelto il server %d\n",i);
            return *it;
        }
        i++;
    }


    log(INFO,"Server non trovato, creo uno nuovo \n");

    return createServer(MODE_SINGLE);
    //netServer.gameServer=
}
CMNetServer* RoomManager::createServer(unsigned char mode)
{
    if(elencoServer.size() >= 500)
    {
        return nullptr;
    }
    CMNetServer *netServer = new CMNetServer(this,gameServer,mode);

    elencoServer.push_back(netServer);

    Statistics::getInstance()->setNumRooms(elencoServer.size());

    return netServer;
}

void RoomManager::removeDeadRooms()
{

    int i=0;
    //log(INFO,"analizzo la lista server e cerco i morti\n");
    for(auto it =elencoServer.begin(); it!=elencoServer.end();)
    {
        CMNetServer *p = *it;

        if(p->state == CMNetServer::State::DEAD)
        {
            log(INFO,"elimino il server %d\n",i);
            delete (*it);
            it=elencoServer.erase(it);
        }
        else
        {
            ++it;
        }

        i++;
    }
    Statistics::getInstance()->setNumRooms(elencoServer.size());
}


}
