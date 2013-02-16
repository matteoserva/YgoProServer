#include "Users.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <thread>
#include <exception>
#include <algorithm>
#include <list>
#include <math.h>
#include "debug.h"
namespace ygo
{
Users::Users()
{
    LoadDB();


    t1 = std::thread(SaveThread,this);
}
bool Users::validLoginString(std::string loginString)
{
    try
    {
        auto pa = splitLoginString(loginString);
        return true;
    }
    catch(std::exception& ex)
    {
        return false;
    }
}
void Users::SaveThread(Users * that)
{
    while(1)
    {
        sleep(20);

        time_t tempo1,tempo2;
        time(&tempo1);
        that->SaveDB();
        time(&tempo2);
        int delta = tempo2-tempo1;

        printf("salvato il DB, ha impiegato %d secondi\n",delta);

    }
}

std::pair<std::string,std::string> Users::splitLoginString(std::string loginString)
{
    printf("splitto %s\n",loginString.c_str());
    std::string username;
    std::string password="";

    auto found=loginString.find('|');
    if(found != std::string::npos)
        throw std::exception();

    found=loginString.find('$');
    if(found == std::string::npos)
        username = loginString;
    else
    {
        username = loginString.substr(0,found);
        password = loginString.substr(found+1,std::string::npos);
    }

    std::string legal="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789%@![]{}\\/*:.,&-_à‘Ô?Ë+^";
    found = username.find_first_not_of(legal);
    if(found != std::string::npos)
        throw std::exception();

    if (username.length()<2 || username.length() > 20)
        throw std::exception();

    return std::pair<std::string,std::string> (username,password);
}




std::string Users::login(std::string loginString)
{
    std::string username;
    std::string password;
    try
    {
        std::pair<std::string,std::string> userpass = splitLoginString(loginString);
        username = userpass.first;
        password = userpass.second;
    }
    catch(std::exception& ex)
    {
        username ="Player";
        password = "";
    }

    std::lock_guard<std::mutex> guard(usersMutex);

    return login(username,password);
}

std::string Users::login(std::string username, std::string password)
{
    std::cout<<"Tento il login con "<<username<<" e "<<password<<std::endl;
    std::string usernamel=username;

    std::transform(usernamel.begin(), usernamel.end(), usernamel.begin(), ::tolower);
    std::cout<<"Tento il login con "<<usernamel<<" e "<<password<<std::endl;
    if(users.find(usernamel) == users.end())
    {
        std::cout<<usernamel<<std::endl;
        users[usernamel] = new UserData(username,password);
        time(&users[usernamel]->last_login);
        return username;
    }

    UserData* d = users[usernamel];
    if(usernamel == "duelista" || usernamel == "player" || d->password=="" || d->password==password )
    {
        d->password = password;
        time(&users[usernamel]->last_login);
        return users[usernamel]->username;
    }

    username = getFirstAvailableUsername(username);
    return login(username,password);

}

int Users::getScore(std::string username)
{
    std::transform(username.begin(), username.end(), username.begin(), ::tolower);
    std::lock_guard<std::mutex> guard(usersMutex);
    log(VERBOSE,"%s ha %d punti\n",username.c_str(),users[username]->score);
    return users[username]->score;
}



void Users::Draw(std::string d1, std::string d2)
{

}

void Users::Draw(std::string d1, std::string d2,std::string d3, std::string d4)
{

}


void Users::Victory(std::string win, std::string los)
{
    std::transform(win.begin(), win.end(), win.begin(), ::tolower);
    std::transform(los.begin(), los.end(), los.begin(), ::tolower);


    std::lock_guard<std::mutex> guard(usersMutex);
    int winscore = users[win]->score;
    int losescore = users[los]->score;
    int delta = losescore-winscore;
    float we = 1.0/(exp(-delta/400.0)+1.0);
    float k = 200.0;

    users[win]->score = winscore + k*(1.0-we);
    users[los]->score = losescore + k*(0.0-we);

    if(users[los]->score < 100)
        users[los]->score = 100;
    std::cout << win << " score: "<<winscore<<" --> "<<users[win]->score<<std::endl;
    std::cout << los << " score: "<<losescore<<" --> "<<users[los]->score<<std::endl;
}
void Users::Victory(std::string win1, std::string win2,std::string los1, std::string los2)
{
    std::transform(win1.begin(), win1.end(), win1.begin(), ::tolower);
    std::transform(los1.begin(), los1.end(), los1.begin(), ::tolower);
    std::transform(win2.begin(), win2.end(), win2.begin(), ::tolower);
    std::transform(los2.begin(), los2.end(), los2.begin(), ::tolower);
    std::lock_guard<std::mutex> guard(usersMutex);
    float win1score = users[win1]->score;
    float lose1score = users[los1]->score;
    float win2score = users[win2]->score;
    float lose2score = users[los2]->score;
    int delta = -(win1score+win2score-lose1score-lose2score)/2; //<-- /2!
    float we = 1.0/(exp(-delta/400.0)+1.0);
    float k = 400.0; //<--400!



    users[win1]->score += k*(1.0-we) * win1score/(win1score+win2score);
    users[win2]->score += k*(1.0-we) * win2score/(win1score+win2score);
    users[los1]->score += k*(0.0-we) * lose1score/(lose1score+lose2score);
    users[los2]->score += k*(0.0-we) * lose2score/(lose1score+lose2score);

    if(users[los1]->score < 100)
        users[los1]->score = 100;
    if(users[los2]->score < 100)
        users[los2]->score = 100;

    std::cout << win1 << " score: "<<win1score<<" --> "<<users[win1]->score<<std::endl;
    std::cout << los1 << " score: "<<lose1score<<" --> "<<users[los1]->score<<std::endl;
    std::cout << win2 << " score: "<<win2score<<" --> "<<users[win2]->score<<std::endl;
    std::cout << los2 << " score: "<<lose2score<<" --> "<<users[los2]->score<<std::endl;
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
static bool compareUserData (UserData* u1, UserData* u2) { return (u1->score > u2->score); }
void Users::SaveDB()
{
    //return;
    std::ofstream inf("users.txt");
    std::list<UserData*> userList;
    usersMutex.lock();

    for(auto it = users.cbegin(); it!=users.cend(); ++it)
    {
        userList.push_back(it->second);
    }
    userList.sort(compareUserData);
    for(auto it = userList.cbegin(); it!=userList.cend(); ++it)
    {
        inf<<(*it)->username<<"|"<<(*it)->password<<"|"<<(*it)->score;
        inf << "|"<<(*it)->last_login<<std::endl;
    }
    usersMutex.unlock();

}
void Users::LoadDB()
{
    std::cout<<"LoadDB"<<std::endl;
    std::ifstream inf("users.txt");
    std::string username;
    while(!std::getline(inf, username, '|').eof())
    {
        std::string password;
        std::string iscore;
        std::string slast_login;
        unsigned int score;
        time_t last_login;
        std::getline(inf, password, '|');
        std::getline(inf, iscore,'|');
        std::getline(inf,slast_login);
        score = stoi(iscore);
        last_login = stoul(slast_login);
        std::cout<<"adding user "<<username<<" pass: "<<password<< "score: "<<score<<std::endl;
        if(!validLoginString(username+"$"+password))
            continue;

        splitLoginString(username+"$"+password);
        UserData* ud = new UserData(username,password,score);
        ud->last_login = last_login;
        std::transform(username.begin(), username.end(), username.begin(), ::tolower);
        users[username] = ud;
    }

}

}

