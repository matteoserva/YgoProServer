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

public:
    static Statistics* getInstance();
    void setNumPlayers(int);
    void setNumRooms(int);

};

}

#endif
