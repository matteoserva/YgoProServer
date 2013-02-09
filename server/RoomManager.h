
#ifndef ROOMMANAGER_H
#define ROOMMANAGER_H
#include <list>

#include "WaitingRoom.h"
namespace ygo {

    class GameServer;


    class RoomManager
    {
        private:
        WaitingRoom* waitingRoom;
        void removeDeadRooms();
        bool FillRoom(CMNetServer* room);
        bool FillAllRooms();
        CMNetServer* createServer(unsigned char mode);
        static int RoomManagerThread(void* );
        static void keepAlive(evutil_socket_t fd, short events, void* arg);
        public:
        event_base* net_evbase;
        std::list<CMNetServer *> elencoServer;
        GameServer* gameServer;
        void setGameServer(ygo::GameServer*);

        public:
        RoomManager();
        ~RoomManager();
        bool InsertPlayerInWaitingRoom(DuelPlayer*dp);
        bool InsertPlayer(DuelPlayer*dp);
        bool InsertPlayer(DuelPlayer*dp,unsigned char mode);
        CMNetServer* getFirstAvailableServer();
        CMNetServer* getFirstAvailableServer(unsigned char mode);
    };




}
#endif
