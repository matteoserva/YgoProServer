


#include "GameServer.h"
#include "RoomManager.h"
namespace ygo {



    void RoomManager::setGameServer(GameServer* gs)
    {
        gameServer = gs;

        /*for(int i=0;i<5000;i++)
        {
                    CMNetServer *netServer = new CMNetServer(this,MODE_SINGLE);
        netServer->gameServer=gameServer;
        elencoServer.push_back(netServer);

        }*/


    }

    RoomManager::RoomManager()
    {
        net_evbase = event_base_new();
        Thread::NewThread(RoomManagerThread, this);

    }
    RoomManager::~RoomManager()
    {
        if(net_evbase)
        {
            event_base_loopexit(net_evbase, 0);
        }

    }

/*
 * This function is needed to avoid a bug in libevent
 */
void RoomManager::keepAlive(evutil_socket_t fd, short events, void* arg)
{

//printf("roommanager keepalive\n");


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
	   for(auto it =elencoServer.begin(); it!=elencoServer.end();)
	   {
          CMNetServer *p = *it;
          printf("analizzo la lista server\n");
	      if(p->state == CMNetServer::State::WAITING && p->mode == mode)
            {
                printf("ho scelto il server %d\n",i);
                return *it;
            }else if(p->state == CMNetServer::State::DEAD)
            {
                printf("elimino il server %d\n",i);
                delete (*it);
                it=elencoServer.erase(it);
            }else
            {
                ++it;
            }


            i++;
	   }

        if(elencoServer.size() >= 500)
        {
            return NULL;
        }
         printf("Server non trovato, creo uno nuovo \n");
        CMNetServer *netServer = new CMNetServer(this,mode);
        netServer->gameServer=gameServer;
        elencoServer.push_back(netServer);
        return netServer;
        //netServer.gameServer=

}
CMNetServer* RoomManager::getFirstAvailableServer()
{
int i = 0;
	   for(auto it =elencoServer.begin(); it!=elencoServer.end();)
	   {
          CMNetServer *p = *it;
          printf("analizzo la lista server\n");
	      if(p->state == CMNetServer::State::WAITING)
            {
                printf("ho scelto il server %d\n",i);
                return *it;
            }else if(p->state == CMNetServer::State::DEAD)
            {
                printf("elimino il server %d\n",i);
                delete (*it);
                it=elencoServer.erase(it);
            }else
            {
                ++it;
            }


            i++;
	   }

        if(elencoServer.size() >= 500)
        {
            return NULL;
        }
         printf("Server non trovato, creo uno nuovo \n");
        CMNetServer *netServer = new CMNetServer(this,MODE_SINGLE);
        netServer->gameServer=gameServer;
        elencoServer.push_back(netServer);
        return netServer;
        //netServer.gameServer=

}

}
