#include "MySqlWrapper.h"
#include "Config.h"
#include <cppconn/driver.h>
#include "mysql_driver.h"
#include <cppconn/exception.h>

namespace ygo
{



MySqlWrapper::MySqlWrapper():con(nullptr),connectRequested(false)
{

}

MySqlWrapper::~MySqlWrapper()
{
    if(con)
    {
        delete con;
        con = nullptr;
    }
}

bool MySqlWrapper::connectionEstablished()
{
    return con && (!con->isClosed());
}

void MySqlWrapper::connect()
{
    connectRequested = true;
    if(con && !con->isClosed())
        return;

    if(con && con->isClosed())
        {
            delete con;
            con = nullptr;
        }
    Config* config = Config::getInstance();
    std::string host = "tcp://" + config->mysql_host + ":3306";
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

sql::Connection * MySqlWrapper::getConnection()
{
    if(!connectRequested)
        throw sql::SQLException();
    if(!connectionEstablished())
        connect();
    if(!connectionEstablished())
        throw sql::SQLException();
    return con;
}

void MySqlWrapper::disconnect()
{
    connectRequested = false;
    if(con == nullptr)
        return;
    con->close();

}

MySqlWrapper* MySqlWrapper::getInstance()
{
    static MySqlWrapper ec;
    return &ec;
}

}
