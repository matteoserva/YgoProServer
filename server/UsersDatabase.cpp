#include "UsersDatabase.h"
#include "Config.h"
#include <memory> //unique_ptr
#include <cppconn/prepared_statement.h>
namespace ygo
{
int UsersDatabase::getRank(std::string username)
{
    try
    {
        //std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("SELECT rank from (SELECT username,score, @rownum := @rownum + 1 AS rank FROM stats, (SELECT @rownum := 0) r ORDER BY score DESC) z where username = ?"));
        std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("SELECT rank from ranking where username = ?"));

        stmt->setString(1, username);

        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        if(!res->next())
           return 0;
        int rank = res->getInt(1);
        return rank;
    }
    catch (sql::SQLException &e)
    {
        return 0;
    }
}



bool UsersDatabase::setUserStats(UserStats &us)
{
    //true is success
    try
    {
        std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("UPDATE stats SET score = ?, wins = ?, losses = ?, draws = ? WHERE username = ?"));
        stmt->setString(5, us.username);
        stmt->setInt(1, us.score);
        stmt->setInt(2, us.wins);
        stmt->setInt(3, us.losses);
        stmt->setInt(4, us.draws);
        int updateCount = stmt->executeUpdate();
        return updateCount > 0;
    }
    catch (sql::SQLException &e)
    {
        std::cout << "# ERR: SQLException in " << __FILE__;
        std::cout << "(" << __FUNCTION__ << ") on line "              << __LINE__ << std::endl;
        std::cout << "# ERR: " << e.what();
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

UserStats UsersDatabase::getUserStats(std::string username)
{
    //true is success
    try
    {
        std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("SELECT * FROM stats where username = ?"));
        //sql::PreparedStatement *stmt = con->prepareStatement("INSERT INTO users VALUES (?, ?, ?)");
        stmt->setString(1, username);
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        if(!res->next())
            throw std::exception();
        UserStats us;
        us.username =res->getString(1);
        us.score = res->getInt(2);
        us.wins = res->getInt(3);
        us.losses = res->getInt(4);
        us.draws =res->getInt(5);
        return us;

    }
    catch (sql::SQLException &e)
    {
        std::cout << "# ERR: SQLException in " << __FILE__;
        std::cout << "(" << __FUNCTION__ << ") on line "              << __LINE__ << std::endl;
        std::cout << "# ERR: " << e.what();
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        throw std::exception();
    }

}


bool UsersDatabase::createUser(std::string username, std::string password, int score,int wins,int losses,int draws)
{
    //true is success
    try
    {
        std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("INSERT INTO users(username,password) VALUES (?, ?)"));
        //sql::PreparedStatement *stmt = con->prepareStatement("INSERT INTO users VALUES (?, ?, ?)");
        stmt->setString(1, username);
        stmt->setString(2, password);
        stmt->execute();
        std::unique_ptr<sql::PreparedStatement> stmt_stats(con->prepareStatement("INSERT INTO stats(username,score,wins,losses,draws) VALUES (?, ?, ?,?,?)"));
        stmt_stats->setString(1, username);
        stmt_stats->setInt(2,score);
        stmt_stats->setInt(3,wins);
        stmt_stats->setInt(4,losses);
        stmt_stats->setInt(5,draws);
        stmt_stats->execute();
    }
    catch (sql::SQLException &e)
    {
        std::cout << "# ERR: SQLException in " << __FILE__;
        std::cout << "(" << __FUNCTION__ << ") on line "              << __LINE__ << std::endl;
        std::cout << "# ERR: " << e.what();
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
    return true;
}

bool UsersDatabase::userExists(std::string username)
{
    try
    {
        std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("select count(*) FROM users WHERE username = ?"));
        stmt->setString(1, username);

        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        res->next();
        int trovato = res->getInt(1);
        return (trovato>0);
    }
    catch (sql::SQLException &e)
    {
        std::cout<<"errore in userexists\n";
        return false;
    }
    return true;
}

bool UsersDatabase::login(std::string username,std::string password)
{
    try
    {
        std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("select password FROM users WHERE username = ?"));
        stmt->setString(1, username);
        //stmt->setString(2, password);
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        if(!res->next())
            return false;
        std::string realPassword = res->getString(1);
        if(realPassword != "" && password != realPassword)
            return false;

        //login success

        std::unique_ptr<sql::PreparedStatement> stmt2(con->prepareStatement("UPDATE users set password = ?, last_login = CURRENT_TIMESTAMP WHERE username = ?"));
        stmt2->setString(1,password);
        stmt2->setString(2, username);
        stmt2->execute();
        return true;
    }
    catch (sql::SQLException &e)
    {
        std::cout<<"errore in userexists\n";
        return false;
    }
}

UsersDatabase::UsersDatabase():con(nullptr)
{
    Config* config = Config::getInstance();
    std::string host = "tcp://" + config->mysql_host + ":3306";
    std::cout<<"Stringa host"<<host<<std::endl;
    try
    {
        /* Create a connection */
        sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();
        con = driver->connect(host, config->mysql_username, config->mysql_password);
        bool myTrue = true;
        con->setClientOption("OPT_RECONNECT", &myTrue);

        /* Connect to the MySQL database */
        con->setSchema(config->mysql_database);

    }
    catch (sql::SQLException &e)
    {
        std::cout<<"errore nella connessione\n";
        std::cout << "# ERR: SQLException in " << __FILE__;
        std::cout << "(" << __FUNCTION__ << ") on line "              << __LINE__ << std::endl;
        std::cout << "# ERR: " << e.what();
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    }
}
UsersDatabase::~UsersDatabase()
{
    if(con != nullptr)
        delete con;
}


}
