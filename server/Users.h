#ifndef _USERS_H_
#define _USERS_H_

#include <string>
#include "debug.h"
#include <map>
namespace ygo
{

struct UserData
{
    std::string username;
    std::string password;
    unsigned int score;
    UserData(std::string username,std::string password):username(username),password(password),score(1000){}
    UserData():username("Player"),password(""),score(1000){}

};



class Users
{
    private:
    std::map<std::string,UserData*> users;
    std::string getFirstAvailableUsername(std::string base);
    public:
    static Users* getInstance();
    std::string login(std::string);
    std::string login(std::string,std::string);
    void Victory(std::string, std::string);
    void Victory(std::string, std::string,std::string, std::string);

};

}
#endif
