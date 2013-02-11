#include "Statistics.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include "mythread.h"
#include <string>
#include <sstream>
namespace ygo
{
Statistics::Statistics():numPlayers(0),numRooms(0)
{
    Thread::NewThread(StatisticsThread, this);
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

int Statistics::StatisticsThread(void* param)
{
    Statistics*that = (Statistics*) param;

    for(;;)
    {
        int result = that->sendStatistics();
        if(result)
            printf("risultato statistiche: %d\n",result);

        sleep(20);
    }

}

int Statistics::sendStatistics()
{
    int portno = 80;
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    std::string hostname = "ygopro.cyberplanet.it";

    char buffer[256];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        return -1;
    server = gethostbyname(hostname.c_str());
    if (server == NULL)
    {
        fprintf(stderr,"ERROR, no such host\n");
        return -2;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        return -3;



    std::ostringstream convert;
    convert << "GET /statistics.php?rooms="<<numRooms<<"&players="<<numPlayers<<" HTTP/1.1\n";
    convert << "Host: " << hostname <<"\n";
    convert << "\n";
    std::string getString = convert.str();

    n = write(sockfd,getString.c_str(),getString.length());
    if (n < 0)
        return -4;
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0)
        return -5;

    close(sockfd);
    return 0;

}

}
