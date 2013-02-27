#include <mysql_connection.h>
#include "mysql_driver.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
namespace ygo
{
 class UsersDatabase
 {
        public:
        static const int default_score = 500;

        UsersDatabase();
        ~UsersDatabase();
        bool createUser(std::string username, std::string password, int score=default_score,int wins=0,int losses=0,int draws=0);
        bool userExists(std::string username);
        bool login(std::string username,std::string password);

        private:
        sql::Connection *con;
 };


}
