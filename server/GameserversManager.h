#include "GameServer.h"
namespace ygo
{
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

    int getNumRooms();
    int getNumPlayers();
public:
    void StartServer(int port);
    GameserversManager();



};
}
