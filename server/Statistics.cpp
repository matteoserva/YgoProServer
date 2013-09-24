#include "Statistics.h"
#include <stdio.h>
#include "Config.h"
#include "debug.h"

/* sql wrapper */
#include "MySqlWrapper.h"
#include <memory> //unique_ptr
#include <cppconn/prepared_statement.h>
#include <cppconn/exception.h>

namespace ygo
{

void Statistics::SendStatisticsRow(ServerStats serverStats)
{

        try
        {
            sql::Connection *con = MySqlWrapper::getInstance()->getConnection();

            std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("insert into serverstats_instances(PID,users,rooms,max_users,status) values(?,?,?,?,?) ON DUPLICATE KEY UPDATE users=?,rooms=?,max_users=?,status=?"));
            //stmt->setQueryTimeout(5);
            stmt->setInt(1, serverStats.PID);
            stmt->setInt(2, serverStats.users);
            stmt->setInt(3, serverStats.rooms);
            stmt->setInt(4, serverStats.max_users);
            stmt->setString(5, serverStats.status);
            stmt->setInt(6, serverStats.users);
            stmt->setInt(7, serverStats.rooms);
            stmt->setInt(8, serverStats.max_users);
            stmt->setString(9, serverStats.status);
            int updateCount = stmt->executeUpdate();
            return;
        }
        catch (sql::SQLException &e)
        {
            std::cout << "# ERR: SQLException in " << __FILE__;
            std::cout << "(" << __FUNCTION__ << ") on line "              << __LINE__ << std::endl;
            std::cout << "# ERR: " << e.what();
            std::cout << " (MySQL error code: " << e.getErrorCode();
            std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        }


}

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


Statistics::ServerStats::ServerStats(int PID,int users,int rooms,int max_users,std::string status):PID(PID),users(users),rooms(rooms),max_users(max_users),status(status)
{


}

int Statistics::sendStatistics()
{
    if(!isRunning)
        return 0;
    if(time(NULL)-last_send<5)
        return 0;

    last_send = time(NULL);

    char buffer[1024];
    int n = sprintf(buffer,"rooms: %d\nplayers: %d/%d",numRooms,numPlayers,Config::getInstance()->max_processes*Config::getInstance()->max_users_per_process);
    if(n>0)
    if(FILE* fp = fopen("stats.txt", "w"))
        {
                fwrite(buffer,n,1,fp);
                fclose(fp);
        }

    return 0;

}

}
