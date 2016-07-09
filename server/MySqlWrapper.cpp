#include "MySqlWrapper.h"
#include "Config.h"
#include <cppconn/driver.h>
#include "mysql_driver.h"


namespace ygo
{



MySqlWrapper::MySqlWrapper():con(nullptr),connectRequested(false),backoff(0)
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

void MySqlWrapper::notifyException(sql::SQLException &e)
{
        if(Config::getInstance()->debugSql)
        {
            std::cout<<"errore nella connessione\n";
            std::cout << "# ERR: SQLException in " << __FILE__;
            std::cout << "(" << __FUNCTION__ << ") on line "              << __LINE__ << std::endl;
            std::cout << "# ERR: " << e.what();
            std::cout << " (MySQL error code: " << e.getErrorCode();
            std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        }
        if(1205 == e.getErrorCode() )
            backoff = time(NULL);

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



        sql::ConnectOptionsMap connection_properties;

        connection_properties["hostName"]=host;
        connection_properties["userName"]=config->mysql_username;
        connection_properties["password"]=config->mysql_password;
        connection_properties["MYSQL_OPT_READ_TIMEOUT"]=5;
        con = driver->connect(connection_properties);
        //con = driver->connect(host, config->mysql_username, config->mysql_password);
        /*
        bool myTrue = true;
        unsigned int timeout = 5;
        con->setClientOption("OPT_RECONNECT", &myTrue);

        con->setClientOption("MYSQL_OPT_READ_TIMEOUT", &timeout);
        */
        /* Connect to the MySQL database */
        con->setSchema(config->mysql_database);

    }
    catch (sql::SQLException &e)
    {
        MySqlWrapper::getInstance()->notifyException(e);

    }

}

sql::Connection * MySqlWrapper::getConnection()
{
    if(backoff)
    {
        if(time(NULL) - backoff > 60)
            backoff = 0;
        else
            throw sql::SQLException();
    }

    if(Config::getInstance()->disableMysql)
        throw sql::SQLException();
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
