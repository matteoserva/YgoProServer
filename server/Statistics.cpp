#include "Statistics.h"
#include <stdio.h>
namespace ygo
{

    Statistics* Statistics::getInstance()
    {
        static Statistics statistics;
        return &statistics;
    }

    void Statistics::setNumPlayers(int numP)
    {
        printf("there are %d players\n",numP);


    }
    void Statistics::setNumRooms(int numR)
    {
        printf("there are %d rooms\n",numR);


    }

}
