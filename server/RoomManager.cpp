


#include "GameServer.h"
#include "RoomManager.h"
namespace ygo {



    void RoomManager::setGameServer(GameServer* gs)
    {
        gameServer = gs;

    }


NetServer* RoomManager::getFirstAvailableServer()
{
int i = 0;
	   for(std::vector<NetServer*>::iterator it =elencoServer.begin(); it!=elencoServer.end();it++)
	   {

          NetServer *p = *it;
	      if(p->state == NetServer::State::STOPPED)
            {
                printf("ho scelto il server %d\n",i);
                return *it;



            }
            i++;
	   }

        if(elencoServer.size() >= 500)
        {
            return NULL;

        }
         printf("Server non trovato, creo uno nuovo \n");
        NetServer *netServer = new NetServer();
        netServer->gameServer=gameServer;
        elencoServer.push_back(netServer);
        return netServer;
        //netServer.gameServer=

}

}
