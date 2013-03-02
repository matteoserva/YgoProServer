#ifndef _STATISTICS_H_
#define _STATISTICS_H_
#include <thread>
namespace ygo
{

class Statistics
{

private:
    int numPlayers;
    int numRooms;
    Statistics();
    int sendStatistics();
    static int StatisticsThread(void* param);
    std::thread updateThread;
    volatile bool isRunning;

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
