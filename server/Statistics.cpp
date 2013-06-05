#include "Statistics.h"
#include <stdio.h>

#include "debug.h"
namespace ygo
{
Statistics::Statistics():numPlayers(0),numRooms(0),last_send(0)
{
    isRunning = false;
}

int Statistics::getNumRooms()
{
    return numRooms;
}
int Statistics::getNumPlayers()
{
    return numPlayers;
}

void Statistics::StartThread()
{
    if(!isRunning)
    {
        isRunning = true;


    }
}

void Statistics::StopThread()
{
    if(isRunning)
    {
        isRunning = false;


    }
}


Statistics* Statistics::getInstance()
{
    static Statistics statistics;
    return &statistics;
}

void Statistics::setNumPlayers(int numP)
{
    log(VERBOSE,"there are %d players\n",numP);
    numPlayers=numP;
    if(isRunning)
        sendStatistics();

}
void Statistics::setNumRooms(int numR)
{
    if(numRooms != numR)
        log(VERBOSE,"there are %d rooms\n",numR);
    numRooms=numR;
    if(isRunning)
        sendStatistics();
}



int Statistics::sendStatistics()
{
    if(!isRunning)
        return 0;
    if(time(NULL)-last_send<5)
        return 0;

    last_send = time(NULL);

    char buffer[1024];
    int n = sprintf(buffer,"rooms: %d\nplayers: %d",numRooms,numPlayers);
    if(n>0)
    if(FILE* fp = fopen("stats.txt", "w"))
        {
                fwrite(buffer,n,1,fp);
                fclose(fp);
        }

    return 0;

}

}
