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
    log(VERBOSE,"splitto %s\n",loginString.c_str());
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

std::pair<std::string,Users::LoginResult> Users::login(std::string loginString,char* ip)
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
        return std::pair<std::string,Users::LoginResult> ("-Player",Users::LoginResult::INVALIDUSERNAME);
    }

    return login(username,password,ip);
}

std::pair<std::string,Users::LoginResult> Users::login(std::string username, std::string password,char* ip)
{
    log(INFO,"Tento il login con %s e %s\n",username.c_str(),password.c_str());
    std::string usernamel=username;
    std::transform(usernamel.begin(), usernamel.end(), usernamel.begin(), ::tolower);
    if(username[0] == '-')
        return std::pair<std::string,Users::LoginResult> (username,Users::LoginResult::UNRANKED);

    if(database->login(username,password,ip))
    {
        if(password != "")
            return std::pair<std::string,Users::LoginResult> (username,Users::LoginResult::AUTHENTICATED);
        else
            return std::pair<std::string,Users::LoginResult> (username,Users::LoginResult::NOPASSWORD);
    }
    else
    {
        return std::pair<std::string,Users::LoginResult> ("-" + username,Users::LoginResult::INVALIDPASSWORD);


    }
}

int Users::getScore(std::string username)
{
    if(username[0] == '-')
        return 0;

    try
    {
        int score = database->getScore(username);
        return score;
    }
    catch(std::exception)
    {
        return 0;
    }
}

int Users::getRank(std::string username)
{
    if(username[0] == '-')
        return 0;
    return database->getRank(username);
}


static float win_exp(float delta)
{
    //delta is my_score - opponent score
    return 1.0/(exp((-delta)/400.0)+1.0);
}



static int k(const UserStats &us)
{
    int tempK = 100;
    int threeshold = 10;
    if(us.wins + us.losses+us.draws <= threeshold)
    {
        tempK *=2;
        if(us.score <= 800 && us.score >= 200)
            tempK *=2;
    }
    else if(us.score >= 2000 && us.score < 3000 )
        tempK /=2;
    else if(us.score >= 3000)
	tempK /=4;
    return tempK;
}

void Users::Draw(std::string win, std::string los)
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
        us_win.score = winscore + k(us_win) * (0.5-win_exp(delta));
        us_los.score = losescore + k(us_los) * (0.5 - win_exp(-delta));

        us_win.draws++;
        us_los.draws++;
        if(us_los.score < 100)
            us_los.score = 100;
        if(us_win.score < 100)
            us_win.score = 100;
        database->setUserStats(us_win);
        database->setUserStats(us_los);

        log(INFO,"%s score: %d --> %d\n",us_win.username.c_str(),winscore,us_win.score);
        log(INFO,"%s score: %d --> %d\n",us_los.username.c_str(),losescore,us_los.score);
    }
    catch (std::exception e)
    {

    }
}

void Users::Draw(std::string win1, std::string win2,std::string los1, std::string los2)
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

        int win1score = us_win1.score;
        int lose1score = us_los1.score;
        int win2score = us_win2.score;
        int lose2score = us_los2.score;
        int delta = (win1score+win2score-lose1score-lose2score)/2; //<-- /2!

        us_win1.score += k(us_win1) * (0.5-win_exp(delta))  * 2.0*win1score/(win1score+win2score);
        us_win2.score += k(us_win2) * (0.5-win_exp(delta))  * 2.0*win2score/(win1score+win2score);
        us_los1.score += k(us_los1) * (0.5-win_exp(-delta)) * 2.0*lose1score/(lose1score+lose2score);
        us_los2.score += k(us_los2) * (0.5-win_exp(-delta)) * 2.0*lose2score/(lose1score+lose2score);

        if(us_los1.score < 100)
            us_los1.score = 100;
        if(us_los2.score < 100)
            us_los2.score = 100;
        if(us_win1.score < 100)
            us_win1.score = 100;
        if(us_win2.score < 100)
            us_win2.score = 100;

        us_win1.draws++;
        us_los1.draws++;
        us_win2.draws++;
        us_los2.draws++;

        us_win1.tags++;
        us_los1.tags++;
        us_win2.tags++;
        us_los2.tags++;

        database->setUserStats(us_win1);
        database->setUserStats(us_los1);
        database->setUserStats(us_win2);
        database->setUserStats(us_los2);

        log(INFO,"%s score: %d --> %d\n",win1.c_str(),win1score,us_win1.score);
        log(INFO,"%s score: %d --> %d\n",win2.c_str(),win2score,us_win2.score);
        log(INFO,"%s score: %d --> %d\n",los1.c_str(),lose1score,us_los1.score);
        log(INFO,"%s score: %d --> %d\n",los2.c_str(),lose2score,us_los2.score);
    }
    catch (std::exception e)
    {

    }
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
        us_win.score = winscore + k(us_win) * (1.0-win_exp(delta));
        us_los.score = losescore + k(us_los) * (0.0 - win_exp(-delta));

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

        int win1score = us_win1.score;
        int lose1score = us_los1.score;
        int win2score = us_win2.score;
        int lose2score = us_los2.score;
        int delta = (win1score+win2score-lose1score-lose2score)/2; //<-- /2!

        us_win1.score += k(us_win1) * (1.0-win_exp(delta))  * 2.0*win1score/(win1score+win2score);
        us_win2.score += k(us_win2) * (1.0-win_exp(delta))  * 2.0*win2score/(win1score+win2score);
        us_los1.score += k(us_los1) * (0.0-win_exp(-delta)) * 2.0*lose1score/(lose1score+lose2score);
        us_los2.score += k(us_los2) * (0.0-win_exp(-delta)) * 2.0*lose2score/(lose1score+lose2score);

        if(us_los1.score < 100)
            us_los1.score = 100;
        if(us_los2.score < 100)
            us_los2.score = 100;

        us_win1.wins++;
        us_los1.losses++;
        us_win2.wins++;
        us_los2.losses++;

        us_win1.tags++;
        us_los1.tags++;
        us_win2.tags++;
        us_los2.tags++;

        database->setUserStats(us_win1);
        database->setUserStats(us_los1);
        database->setUserStats(us_win2);
        database->setUserStats(us_los2);

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





