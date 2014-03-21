#include <mysql_connection.h>
#include "mysql_driver.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <exception>      // std::exception
#include "DuelLogger.h"
namespace ygo
{

struct UserStats
{
    std::string username;
    unsigned int score;
    unsigned int wins;
    unsigned int losses;
    unsigned int draws;
    unsigned int tags;
};


class UsersDatabase
{
public:
    static const int default_score = 1000;

    UsersDatabase();
    ~UsersDatabase();
    bool createUser(std::string username, std::string password, int score=default_score,int wins=0,int losses=0,int draws=0);
    bool userExists(std::string username);
    int login(std::string username,std::string password,char*ip);
    UserStats getUserStats(std::string username);
    bool setUserStats(UserStats&);
	bool setUserStats(UserStats&,LoggerPlayerInfo *);
    int getRank(std::string username);
    std::pair<int,int> getScore(std::string username);
    std::string getCountryCode(std::string);

private:

};


}
