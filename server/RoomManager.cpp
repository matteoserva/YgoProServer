

#include "GameServer.h"
#include "RoomManager.h"
namespace ygo
{



void RoomManager::setGameServer(GameServer* gs)
{
    gameServer = gs;
    waitingRoom = new WaitingRoom(this,gs);
}

RoomManager::RoomManager()
{
    net_evbase = event_base_new();
    waitingRoom=0;
    Thread::NewThread(RoomManagerThread, this);

}
RoomManager::~RoomManager()
{
    if(net_evbase)
    {
        event_base_loopexit(net_evbase, 0);
    }
    delete waitingRoom;

}


void RoomManager::keepAlive(evutil_socket_t fd, short events, void* arg)
{
RoomManager*that = (RoomManager*) arg;
that->removeDeadRooms();
}

int RoomManager::RoomManagerThread(void* arg)
{
    RoomManager*that = (RoomManager*) arg;
    event_base* net_evbase = that->net_evbase;

    timeval timeout = {5, 0};

    event* ev1 = event_new(net_evbase, 0, EV_TIMEOUT | EV_PERSIST, keepAlive, arg);
    event_add(ev1, &timeout);
    event_base_dispatch(net_evbase);
    event_del(ev1);
    printf("roommanager thread terminato\n");

    event_free(ev1);

    event_base_free(net_evbase);

}

CMNetServer* RoomManager::getFirstAvailableServer(unsigned char mode)
{
    int i = 0;
    for(auto it =elencoServer.begin(); it!=elencoServer.end(); ++it)
    {
        CMNetServer *p = *it;
        printf("analizzo la lista server\n");
        if(p->state == CMNetServer::State::WAITING && p->mode == mode)
        {
            printf("ho scelto il server %d\n",i);
            return *it;
        }


        i++;
    }


    printf("Server non trovato, creo uno nuovo \n");
    return createServer(mode);
    //netServer.gameServer=

}

bool RoomManager::InsertPlayer(DuelPlayer*dp)
{//tfirst room
        CMNetServerInterface* netServer = getFirstAvailableServer();
        dp->netServer=netServer;
        netServer->InsertPlayer(dp);
        return true;
}

bool RoomManager::InsertPlayerInWaitingRoom(DuelPlayer*dp)
{//true is success

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
{//true is success
        CMNetServerInterface* netServer = getFirstAvailableServer(mode);
        dp->netServer=netServer;
        netServer->InsertPlayer(dp);
        return true;
}

CMNetServer* RoomManager::getFirstAvailableServer()
{
    int i = 0;
    for(auto it =elencoServer.begin(); it!=elencoServer.end(); ++it)
    {
        CMNetServer *p = *it;
        printf("analizzo la lista server\n");
        if(p->state == CMNetServer::State::WAITING)
        {
            printf("ho scelto il server %d\n",i);
            return *it;
        }

        i++;
    }


    printf("Server non trovato, creo uno nuovo \n");

    return createServer(MODE_SINGLE);
    //netServer.gameServer=
}
CMNetServer* RoomManager::createServer(unsigned char mode)
{
    if(elencoServer.size() >= 500)
    {
        return NULL;
    }
    CMNetServer *netServer = new CMNetServer(this,gameServer,mode);

    elencoServer.push_back(netServer);
    return netServer;
}

void RoomManager::removeDeadRooms()
{
    int i=0;
    printf("analizzo la lista server e cerco i morti\n");
    for(auto it =elencoServer.begin(); it!=elencoServer.end();)
    {
        CMNetServer *p = *it;

        if(p->state == CMNetServer::State::DEAD)
        {
            printf("elimino il server %d\n",i);
            delete (*it);
            it=elencoServer.erase(it);
        }
        else
        {
            ++it;
        }

        i++;
    }
}


}
