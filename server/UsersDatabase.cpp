#include "UsersDatabase.h"
#include "Config.h"
#include <memory> //unique_ptr
#include <cppconn/prepared_statement.h>
namespace ygo
{

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
        std::unique_ptr<sql::PreparedStatement> stmt_stats(con->prepareStatement("INSERT INTO stats VALUES (?, ?, ?,?,?)"));
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
        std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("select count(*) FROM users WHERE username = ? AND password = ?"));
        stmt->setString(1, username);
        stmt->setString(2, password);
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        res->next();
        int trovato = res->getInt(1);
        if(trovato > 0)
        {
            std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("UPDATE users set last_login = CURRENT_TIMESTAMP WHERE username = ?"));
            stmt->setString(1, username);
            stmt->execute();
        }
        else
        {
            return false;
        }
        return (trovato>0);
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

        std::cout << "cerco matte2o: "<<login("matte2o","ciao")<<std::endl;

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
