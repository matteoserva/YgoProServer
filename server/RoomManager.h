
#ifndef ROOMMANAGER_H
#define ROOMMANAGER_H
#include <list>
#include <mutex>
#include "WaitingRoom.h"
namespace ygo {

    class GameServer;


    class RoomManager
    {
        private:
        event* keepAliveEvent;

        static const int SecondsBeforeFillAllRooms = 3;
        static const int RemoveDeadRoomsRatio = 3;

        WaitingRoom* waitingRoom;
        void removeDeadRooms();
        bool FillRoom(CMNetServer* room);
        bool FillAllRooms();
        CMNetServer* createServer(unsigned char mode);
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
        CMNetServer* getFirstAvailableServer(int referenceScore);
        CMNetServer* getFirstAvailableServer(int referenceScore,unsigned char mode,bool);
        int getNumPlayers();
        int getNumRooms();
        std::vector<CMNetServer *> getCompatibleRoomsList(int referenceScore);
        void tryToInsertPlayerInServer(DuelPlayer*dp,CMNetServer* serv);
        int maxScoreDifference(int referenceScore){return std::max(400,referenceScore/3);}
    };




}
#endif
