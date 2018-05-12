#ifndef MYSQLWRAPPER_CPP
#define MYSQLWRAPPER_CPP

#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/exception.h>
namespace ygo
{
class MySqlWrapper
{
    public:

    static MySqlWrapper* getInstance();
    void connect();
    void disconnect();
    sql::Connection * getConnection();
    void notifyException(sql::SQLException &e);

    private:
    time_t backoff;
    sql::Connection *con;
    bool connectRequested;

    bool connectionEstablished();

    MySqlWrapper();
    public:
    ~MySqlWrapper();

};

}
#endif
