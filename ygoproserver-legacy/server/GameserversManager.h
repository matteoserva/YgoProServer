#ifndef _GAMESERVERMANAGER_H_
#define _GAMESERVERMANAGER_H_

#include "GameServer.h"
namespace ygo
{


struct ChildInfo
{
    int pid;
    int rooms;
    int players;
    bool isAlive;
    time_t last_update;
    ChildInfo():rooms(0),players(0),isAlive(true){};

};

class GameserversManager
{
private:
    /*child related */
    int parent_fd;



    static void chatCallback(std::wstring message,bool isAdmin,void*);
    int maxchildren;
    int server_fd;
    int spawn_gameserver();
    void child_loop(int);
    bool isFather;

    void ShowStats();
    void parent_loop();
    std::map<int,ChildInfo> children;

    int getNumRooms();
    int getNumPlayers();
    int getNumAliveChildren();
    int getNumPlayersInAliveChildren();
    bool serversAlmostFull();
    bool serversAlmostEmpty();
    void killOneTerminatingServer();
    bool handleChildMessage(int);
    void closeChild(int);
public:
    void StartServer(int port);
    GameserversManager();



};
}

#endif
