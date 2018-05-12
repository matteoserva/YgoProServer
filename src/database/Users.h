#ifndef _USERS_H_
#define _USERS_H_

#include <string>
#include "common/debug.h"
#include <map>
#include <ctime>
#include <mutex>
#include <thread>
#include <vector>
#include "common/common_structures.h"
//#include "DuelLogger.h"
namespace ygo
{
class MySqlWrapper;
class LoginException: public std::exception
{
    std::string username;
    std::string whatString;

public:
    virtual const char* what() const throw()
    {
        return whatString.c_str();
    }
    std::string altUsername()
    {
        return username;
    }
    ~LoginException() throw() {};
    LoginException(std::string username, std::string what):username(username),whatString(what) {}
};

struct UserData
{
    std::string username;
    std::string password;
    unsigned int score;
    time_t  last_login;
    unsigned int rank;
    unsigned int wins;
    unsigned int loses;
    unsigned int tags;
    UserData(std::string username,std::string password):username(username),password(password),score(500),rank(0),wins(0),loses(0),tags(0) {}
    UserData():username("Player"),password(""),score(500),rank(0),wins(0),loses(0),tags(0) {}
    UserData(std::string username,std::string password,unsigned int score):username(username),password(password),score(score),rank(0),wins(0),loses(0),tags(0) {}
};



class UsersDatabase;

class Users
{
    public:

    struct LoginResultTuple
    {
        std::string first; //nam
        LoginResult second; //result
        int color;
        LoginResultTuple(std::string name,LoginResult result,int color=0):first(name),second(result),color(color){}
    };

	Users(MySqlWrapper *);
    ~Users();
private:
	UsersDatabase* database;
	
	
	
    std::pair<std::string,std::string> splitLoginString(std::string);
    bool validLoginString(std::string);
    

    std::string getFirstAvailableUsername(std::string base);
    void LoadDB();
    void SaveDB();
	void UpdateStats(std::vector<LoggerPlayerData> &nomi, int risultato);
	void updateSkill(std::vector<LoggerPlayerData> &nomi, int risultato);
	
    



public:

    std::string getCountryCode(std::string);
    static float win_exp(float delta);
    int getScore(std::string username);
    std::pair<int,int> getScoreRank(std::string username);
    int getRank(std::string username);
    static Users* getInstance();
    LoginResultTuple login(std::string,char* ip);
    LoginResultTuple login(std::string,std::string,char* ip);
    
	
	
	
	void saveDuelResult(std::vector<LoggerPlayerData> nomi, int risultato,std::string replay);
	
	
};

}
#endif
