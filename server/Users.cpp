#include "Users.h"
#include <sstream>
#include <iostream>
namespace ygo
{
std::string Users::login(std::string loginString)
{
    std::string username;
    std::string password="";

    if(loginString.find('|')!=)
        return login("Player","");

    auto found=loginString.find('$');

    if (found==std::string::npos)
    {
        username = loginString;
        return login(username,password);
    }

    username = loginString.substr(0,found);
    password = loginString.substr(found+1,std::string::npos);

    printf("%d, %d\n",found,std::string::npos);
    return login(username,password);
}

std::string Users::login(std::string username, std::string password)
{
    username = username.substr(0,20);
    std::cout<<"Tento il login con "<<username<<" e "<<password<<std::endl;
    if(users.find(username) == users.end())
    {
        users[username] = new UserData(username,password);
        return username;
    }

    UserData* d = users[username];
    if(username == "Player" || d->password=="" || d->password==password )
    {
        d->password = password;
        return username;
    }

    username = getFirstAvailableUsername(username);
    return login(username,password);

}

void Users::Victory(std::string win, std::string los)
{
    users[win]->score += 10;
    std::cout << win << "ha: "<<users[win]->score;
}
void Users::Victory(std::string win1, std::string win2,std::string los1, std::string los2)
{



}




Users* Users::getInstance()
{
    static Users u;
    return &u;
}
std::string Users::getFirstAvailableUsername(std::string base)
{
    /*base = base.substr(0,17);
    for(int i = 1; i < 1000; i++)
    {
        std::ostringstream ostr;
        ostr << i;

        std::string possibleUsername = base+ostr.str();
        if(users.find(possibleUsername) == users.end())
        {
            return possibleUsername;
        }
    }*/
    return "Player";
}

}
