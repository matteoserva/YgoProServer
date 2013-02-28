#include "Users.h"
#include "UsersDatabase.h"
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
    database = new UsersDatabase();
}

Users::~Users()
{
    delete database;
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


std::pair<std::string,std::string> Users::splitLoginString(std::string loginString)
{
    log(INFO,"splitto %s\n",loginString.c_str());
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

    std::string usernamel = username;
    std::transform(usernamel.begin(), usernamel.end(), usernamel.begin(), ::tolower);
    if(usernamel == "duelista" || usernamel == "player")
        username = "-" + username;

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
        username ="-Player";
        password = "";
    }

    return login(username,password);
}

std::string Users::login(std::string username, std::string password)
{
    log(INFO,"Tento il login con %s e %s\n",username.c_str(),password.c_str());
    std::string usernamel=username;
    std::transform(usernamel.begin(), usernamel.end(), usernamel.begin(), ::tolower);
    if(username[0] == '-')
        return username;

    try
    {
        if(!database->userExists(username))
        {
            database->createUser(username,password);
            return username;
        }

        if(database->login(username,password))
            return username;
        else
            return "-" + username;
    }
    catch(std::exception e)
    {
        return "-" + username;
    }

    username = getFirstAvailableUsername(username);
    return login(username,password);

}

int Users::getScore(std::string username)
{
    if(username[0] == '-')
        return 0;

    UserStats us = database->getUserStats(username);
    return us.score;
}

int Users::getRank(std::string username)
{
    if(username[0] == '-')
        return 0;
    return database->getRank(username);
}

void Users::Draw(std::string d1, std::string d2)
{

}

void Users::Draw(std::string d1, std::string d2,std::string d3, std::string d4)
{

}


static float win_exp(float delta)
{
    //delta is my_score - opponent score
    return 1.0/(exp((-delta)/400.0)+1.0);
}

static int k(int score)
{
    if(score <= 1000 && score >= 200)
        return 200;
    return 100;
}

void Users::Victory(std::string win, std::string los)
{
    if(win[0] == '-')
        return;
    if(los[0] == '-')
        return;

    try
    {
        UserStats us_win = database->getUserStats(win);
        UserStats us_los = database->getUserStats(los);
        int winscore = us_win.score;
        int losescore = us_los.score;
        int delta = winscore-losescore;
        us_win.score = winscore + k(winscore)*(1.0-win_exp(delta));
        us_los.score = losescore + k(losescore)*(0.0 - win_exp(-delta));

        us_win.wins++;
        us_los.losses++;
        if(us_los.score < 100)
            us_los.score = 100;
        database->setUserStats(us_win);
        database->setUserStats(us_los);

        log(INFO,"%s score: %d --> %d\n",us_win.username.c_str(),winscore,us_win.score);
        log(INFO,"%s score: %d --> %d\n",us_los.username.c_str(),losescore,us_los.score);
    }
    catch (std::exception e)
    {

    }

}
void Users::Victory(std::string win1, std::string win2,std::string los1, std::string los2)
{

    if(win1[0] == '-')
        return;
    if(los1[0] == '-')
        return;
    if(win2[0] == '-')
        return;
    if(los2[0] == '-')
        return;

    try
    {
        UserStats us_win1 = database->getUserStats(win1);
        UserStats us_win2 = database->getUserStats(win2);
        UserStats us_los1 = database->getUserStats(los1);
        UserStats us_los2 = database->getUserStats(los2);

        float win1score = us_win1.score;
        float lose1score = us_los1.score;
        float win2score = us_win2.score;
        float lose2score = us_los2.score;
        int delta = (win1score+win2score-lose1score-lose2score)/2; //<-- /2!

        us_win1.score += k(win1score)  * (1.0-win_exp(delta))  * 2*win1score/(win1score+win2score);
        us_win2.score += k(win2score)  * (1.0-win_exp(delta))  * 2*win2score/(win1score+win2score);
        us_los1.score += k(lose1score) * (0.0-win_exp(-delta)) * 2*lose1score/(lose1score+lose2score);
        us_los2.score += k(lose2score) * (0.0-win_exp(-delta)) * 2*lose2score/(lose1score+lose2score);

        if(us_los1.score < 100)
            us_los1.score = 100;
        if(us_los2.score < 100)
            us_los2.score = 100;

        us_win1.wins++;
        us_los1.losses++;
        us_win2.wins++;
        us_los2.losses++;

        log(INFO,"%s score: %d --> %d\n",win1.c_str(),win1score,us_win1.score);
        log(INFO,"%s score: %d --> %d\n",win2.c_str(),win2score,us_win2.score);
        log(INFO,"%s score: %d --> %d\n",los1.c_str(),lose1score,us_los1.score);
        log(INFO,"%s score: %d --> %d\n",los2.c_str(),lose2score,us_los2.score);
    }
    catch (std::exception e)
    {

    }

}

Users* Users::getInstance()
{
    static Users u;
    return &u;
}
std::string Users::getFirstAvailableUsername(std::string base)
{
    return "-" + base;
}
static bool compareUserData (UserData* u1, UserData* u2)
{
    return (u1->score > u2->score);
}


}


