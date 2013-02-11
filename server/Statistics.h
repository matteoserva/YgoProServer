#ifndef _STATISTICS_H_
#define _STATISTICS_H_

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

public:
    static Statistics* getInstance();
    void setNumPlayers(int);
    void setNumRooms(int);

};

}

#endif
