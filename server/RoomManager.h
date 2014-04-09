
#ifndef ROOMMANAGER_H
#define ROOMMANAGER_H
#include <list>
#include <mutex>
#include "WaitingRoom.h"
#include "DuelRoom.h"
#include "RematchRoom.h"
namespace ygo {

    class GameServer;
    class RematchRoom;

    class RoomManager
    {
        private:
        event* keepAliveEvent;

        std::multimap<std::string,time_t> bannedIPs;

        public:
        void ban(std::string);
        private:
        bool isBanned(std::string);


        static const int SecondsBeforeFillAllRooms = 3;
        static const int RemoveDeadRoomsRatio = 2;

        WaitingRoom* waitingRoom;
		RematchRoom* rematchRoom;
        void removeDeadRooms();
        bool FillRoom(DuelRoom* room);
        bool FillAllRooms();

        static void keepAlive(evutil_socket_t fd, short events, void* arg);
        public:
        event_base* net_evbase;
        std::list<DuelRoom *> elencoServer;
        std::set<DuelRoom *> playingServer;
        std::set<DuelRoom *> zombieServer;
        GameServer* gameServer;
        void setGameServer(ygo::GameServer*);

        public:
         DuelRoom* createServer(unsigned char mode);

        RoomManager();
        ~RoomManager();
        
		void BroadcastMessage(std::wstring message, int color =0,RoomInterface* origin = nullptr);
        
		void notifyStateChange(DuelRoom* room,DuelRoom::State oldstate,DuelRoom::State newstate);
        bool InsertPlayerInWaitingRoom(DuelPlayer*dp);
        bool InsertPlayer(DuelPlayer*dp);
        bool InsertPlayer(DuelPlayer*dp,unsigned char mode);
        DuelRoom* getFirstAvailableServer(DuelPlayer* referencePlayer);
        DuelRoom* getFirstAvailableServer(DuelPlayer* referencePlayer,unsigned char mode,bool);
        int getNumPlayers();
        int getNumRooms();
        void HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len);

		bool checkSpam(DuelPlayer*dp,std::wstring messaggio);
        std::vector<DuelRoom *> getCompatibleRoomsList(DuelPlayer* dp);
        void tryToInsertPlayerInServer(DuelPlayer*dp,DuelRoom* serv);
        static int maxScoreDifference(int referenceScore){return std::max(400,(referenceScore>3000)?(referenceScore/3):(referenceScore/4));}
    };




}
#endif
