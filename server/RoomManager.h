
#ifndef ROOMMANAGER_H
#define ROOMMANAGER_H
#include <vector>


namespace ygo {

    class GameServer;


    class RoomManager
    {
        public:
            std::vector<NetServer *> elencoServer;
        GameServer* gameServer;
        void setGameServer(ygo::GameServer*);

        public:
        NetServer* getFirstAvailableServer();

    };




}
#endif
