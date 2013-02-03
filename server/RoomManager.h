
#ifndef ROOMMANAGER_H
#define ROOMMANAGER_H
#include <vector>


namespace ygo {

    class GameServer;


    class RoomManager
    {
        public:
            std::vector<CMNetServer *> elencoServer;
        GameServer* gameServer;
        void setGameServer(ygo::GameServer*);

        public:
        CMNetServer* getFirstAvailableServer();

    };




}
#endif
