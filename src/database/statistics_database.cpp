#include "statistics_database.h"
#include "MySqlWrapper.h"
#include <memory> //unique_ptr
#include <cppconn/prepared_statement.h>
#include <cppconn/exception.h>
#include "common/server_protocol.h"
#include "MySqlWrapper.h"

StatisticsDatabase::StatisticsDatabase(ygo::MySqlWrapper * m)
{
	mysqlWrapper = m;
	
}
void StatisticsDatabase::SendStatisticsRow(protocol::GameServerStats serverStats)
{
	 try
        {
            sql::Connection *con = mysqlWrapper->getConnection();

            std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("insert into serverstats_instances(PID,users,rooms,max_users,status) values(?,?,?,?,?) ON DUPLICATE KEY UPDATE users=?,rooms=?,max_users=?,status=?"));
            const char * status = serverStats.isAlive?"ALIVE":"DYING";
			
			//stmt->setQueryTimeout(5);
            stmt->setInt(1, serverStats.pid);
            stmt->setInt(2, serverStats.players);
            stmt->setInt(3, serverStats.rooms);
            stmt->setInt(4, serverStats.max_users);
            stmt->setString(5, status);
            stmt->setInt(6, serverStats.players);
            stmt->setInt(7, serverStats.rooms);
            stmt->setInt(8, serverStats.max_users);
            stmt->setString(9, status);
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