#ifndef _STATISTICS_H_
#define _STATISTICS_H_
#include <time.h>
namespace ygo
{

class Statistics
{

private:
    int numPlayers;
    int numRooms;
    Statistics();
    int sendStatistics();
    volatile bool isRunning;
    time_t last_send;

public:
    static Statistics* getInstance();
    void setNumPlayers(int);
    void setNumRooms(int);
    int getNumRooms();
    int getNumPlayers();
    void StartThread();
    void StopThread();
};

}

#endif
