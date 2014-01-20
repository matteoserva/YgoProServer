
#ifndef ROOMMANAGER_H
#define ROOMMANAGER_H
#include <list>
#include <mutex>
#include "WaitingRoom.h"
#include "netserver.h"
namespace ygo {

    class GameServer;


    class RoomManager
    {
        private:
        event* keepAliveEvent;

        std::set<std::string> bannedIPs;

        public:
        void ban(std::string);
        private:
        bool isBanned(std::string);


        static const int SecondsBeforeFillAllRooms = 3;
        static const int RemoveDeadRoomsRatio = 3;

        WaitingRoom* waitingRoom;
        void removeDeadRooms();
        bool FillRoom(CMNetServer* room);
        bool FillAllRooms();

        static void keepAlive(evutil_socket_t fd, short events, void* arg);
        public:
        event_base* net_evbase;
        std::list<CMNetServer *> elencoServer;
        std::set<CMNetServer *> playingServer;
        std::set<CMNetServer *> zombieServer;
        GameServer* gameServer;
        void setGameServer(ygo::GameServer*);

        public:
         CMNetServer* createServer(unsigned char mode);

        RoomManager();
        ~RoomManager();
        void BroadcastMessage(std::wstring, bool, bool crossServer = false);
        void BroadcastMessage(std::string, bool, bool crossServer = false);
        void BroadcastMessage(std::wstring message, bool isAdmin,CMNetServerInterface* origin = nullptr);

        void notifyStateChange(CMNetServer* room,CMNetServer::State oldstate,CMNetServer::State newstate);
        bool InsertPlayerInWaitingRoom(DuelPlayer*dp);
        bool InsertPlayer(DuelPlayer*dp);
        bool InsertPlayer(DuelPlayer*dp,unsigned char mode);
        CMNetServer* getFirstAvailableServer(DuelPlayer* referencePlayer);
        CMNetServer* getFirstAvailableServer(DuelPlayer* referencePlayer,unsigned char mode,bool);
        int getNumPlayers();
        int getNumRooms();
        void HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len);

        std::vector<CMNetServer *> getCompatibleRoomsList(int referenceScore);
        void tryToInsertPlayerInServer(DuelPlayer*dp,CMNetServer* serv);
        static int maxScoreDifference(int referenceScore){return std::max(400,(referenceScore>3000)?(referenceScore/3):(referenceScore/4));}
    };




}
#endif
