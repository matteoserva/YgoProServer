#ifndef _USERS_H_
#define _USERS_H_

#include <string>
#include "debug.h"
#include <map>
#include <ctime>
#include <mutex>
#include <thread>

namespace ygo
{

class LoginException: public std::exception
{
    std::string username;
    std::string whatString;

public:
    virtual const char* what() const throw()
    {
        return whatString.c_str();
    }
    std::string altUsername(){return username;}
    ~LoginException() throw(){};
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
    UserData(std::string username,std::string password):tags(0),wins(0),loses(0),username(username),password(password),score(500),rank(0) {}
    UserData():tags(0),wins(0),loses(0),username("Player"),password(""),score(500),rank(0) {}
    UserData(std::string username,std::string password,unsigned int score):tags(0),wins(0),loses(0),username(username),password(password),score(score),rank(0) {}
};


class UsersDatabase;

class Users
{
private:
    std::thread t1;
    std::pair<std::string,std::string> splitLoginString(std::string);
    bool validLoginString(std::string);
    Users();
    ~Users();

    std::string getFirstAvailableUsername(std::string base);
    void LoadDB();
    void SaveDB();
    std::string login(std::string,std::string);
    UsersDatabase* database;

public:
    int getScore(std::string username);
    int getRank(std::string username);
    static Users* getInstance();
    std::string login(std::string);
    void Draw(std::string d1, std::string d2);
    void Draw(std::string d1, std::string d2,std::string d3, std::string d4);
    void Victory(std::string, std::string);
    void Victory(std::string, std::string,std::string, std::string);

};

}
#endif
