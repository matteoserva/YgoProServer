#ifndef MYSQLWRAPPER_CPP
#define MYSQLWRAPPER_CPP

#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/exception.h>
namespace ygo
{
class MySqlWrapper
{
    sql::ConnectOptionsMap connectOptionsMap;
    void connect();
    public:

    static MySqlWrapper* getInstance();
   
    void connect(sql::ConnectOptionsMap &);
    void disconnect();
    sql::Connection * getConnection();
    void notifyException(sql::SQLException &e);

    private:
    time_t backoff;
    sql::Connection *con;
    bool connectRequested;

    bool connectionEstablished();

    
    public:
	MySqlWrapper();
    ~MySqlWrapper();

};

}
#endif
