


#include "GameServer.h"
#include "RoomManager.h"
namespace ygo {



    void RoomManager::setGameServer(GameServer* gs)
    {
        gameServer = gs;

    }


CMNetServer* RoomManager::getFirstAvailableServer()
{
int i = 0;
	   for(std::vector<CMNetServer*>::iterator it =elencoServer.begin(); it!=elencoServer.end();it++)
	   {

          CMNetServer *p = *it;
	      if(p->state == CMNetServer::State::STOPPED)
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
        CMNetServer *netServer = new CMNetServer();
        netServer->gameServer=gameServer;
        elencoServer.push_back(netServer);
        return netServer;
        //netServer.gameServer=

}

}
