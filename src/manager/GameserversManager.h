#ifndef _GAMESERVERMANAGER_H_
#define _GAMESERVERMANAGER_H_
#include <map>
//#include "GameServer.h"
#include "database_facade.h"
namespace protocol
{
class Connector;
}
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
	protocol::Connector *clients_connector;


	DatabaseFacade databaseFacade;
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
	
	
	static void clientConnected(int client,void* handle);
	static void clientDisconnected(int,void *);
	static void clientMessaged(int client, void *, unsigned int , void* handle);
	
	
	
	
	
	
public:
    void StartServer(protocol::Connector *);
    GameserversManager();
	static void timeoutCallback(void *);


};
}

#endif
