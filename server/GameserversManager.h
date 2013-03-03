#include "GameServer.h"
namespace ygo
{

struct GameServerStats
{
    int pid;
    int rooms;
    int players;
    bool isAlive;
    GameServerStats();
};

class GameserversManager
{
private:
    int maxchildren;
    int server_fd;
    int spawn_gameserver();
    void child_loop(int);
    bool isFather;

    void ShowStats();
    void parent_loop();
    std::map<int,GameServerStats> children;
    std::set<int> aliveChildren;

    int getNumRooms();
    int getNumPlayers();
    int getNumAliveChildren();
    int getNumPlayersInAliveChildren();
    bool serversAlmostFull();
    bool serversAlmostEmpty();
    void killOneTerminatingServer();
public:
    void StartServer(int port);
    GameserversManager();



};
}
