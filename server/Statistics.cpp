#include "Statistics.h"
#include <stdio.h>
namespace ygo
{
    Statistics::Statistics():numPlayers(0),numRooms(0)
    {

    }
    Statistics* Statistics::getInstance()
    {
        static Statistics statistics;
        return &statistics;
    }

    void Statistics::setNumPlayers(int numP)
    {
        printf("there are %d players\n",numP);
        numPlayers=numP;

    }
    void Statistics::setNumRooms(int numR)
    {
        if(numRooms != numR)
            printf("there are %d rooms\n",numR);
        numRooms=numR;


    }

}
