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

Users::LoginResultTuple Users::login(std::string loginString,char* ip)
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
        return Users::LoginResultTuple ("-Player",Users::LoginResult::INVALIDUSERNAME,0);
    }

    return login(username,password,ip);
}

Users::LoginResultTuple Users::login(std::string username, std::string password,char* ip)
{
    log(INFO,"Tento il login con %s e %s, ip %s\n",username.c_str(),password.c_str(),ip);
    std::string usernamel=username;
    std::transform(usernamel.begin(), usernamel.end(), usernamel.begin(), ::tolower);
    if(username[0] == '-')
        return Users::LoginResultTuple (username,Users::LoginResult::UNRANKED,0);

    if(int color = database->login(username,password,ip))
    {
        if(password != "")
            return Users::LoginResultTuple (username,Users::LoginResult::AUTHENTICATED,color-1);
        else
            return Users::LoginResultTuple (username,Users::LoginResult::NOPASSWORD,color-1);
    }
    else
    {
        return Users::LoginResultTuple ("-" + username,Users::LoginResult::INVALIDPASSWORD,0);


    }
}

std::pair<int,int> Users::getFullScore(std::string username)
{
    if(username[0] == '-')
        return std::pair<int,int>(0,0);

    try
    {
        std::pair<int,int> score = database->getScore(username);
        return score;
    }
    catch(std::exception)
    {
        return std::pair<int,int>(0,0);
    }

}

int Users::getScore(std::string username)
{
    if(username[0] == '-')
        return 0;

    std::pair<int,int> full = getFullScore(username);
    return full.first;
}

int Users::getRank(std::string username)
{
    if(username[0] == '-')
        return 0;
    return database->getRank(username);
}


float Users::win_exp(float delta)
{
    //delta is my_score - opponent score
    return 1.0/(exp((-delta)/400.0)+1.0);
}



static int k(const UserStats &us)
{
    int tempK = 50;
    int threeshold = 10;
    if(us.wins + us.losses+us.draws <= threeshold)
    {
        tempK += tempK/2;
        if(us.score <= 800 && us.score >= 200)
            tempK +=tempK/2;
    }
    else if(us.score >= 2000 && us.score < 3000 )
        tempK /=2;
    else if(us.score >= 3000)
        tempK /=3;
    return tempK;
}

void Users::Draw(std::string win, std::string los)
{
    
    std::vector<std::string> nomi;
	nomi.push_back(win);
	nomi.push_back(los);	
	
	UpdateScore(nomi,2);
}

std::string Users::getCountryCode(std::string ip)
{
    return database->getCountryCode(ip);



}

void Users::UpdateScore(std::vector<std::string> nomi, int risultato) //0= vittoria, 2 = pareggio
{
	std::vector<UserStats> us;
	
	unsigned int num_squadra1 = nomi.size()/2;
	unsigned int num_squadra2 = nomi.size() - num_squadra1;
	
	unsigned int media_squadra1=0;
	unsigned int media_squadra2=0;
	int delta = 0;
	try
    {
		int pos = 0;
		for(auto nome = nomi.cbegin(); nome != nomi.cend();++nome,++pos)
		{
			if((*nome)[0] == '-')
				return;
			UserStats us_tmp = database->getUserStats(*nome);
			us.push_back(us_tmp);
			if(pos < num_squadra1)
				media_squadra1 += us_tmp.score;
			else
				media_squadra2 += us_tmp.score;
		}
		media_squadra1 /= num_squadra1;
		media_squadra2 /= num_squadra2;
		delta = media_squadra1 - media_squadra2;
			
		
		for(int i = 0;i<nomi.size();i++)
		{
			UserStats us_tmp = us[i];
			
			if(i<num_squadra1 && risultato == 0)
			{
				us_tmp.score += k(us_tmp) * (1.0-win_exp(delta))  * 1.0*us_tmp.score/media_squadra1;
				us_tmp.wins++;
			}
			else if(i>=num_squadra1 && risultato == 0)
			{
				us_tmp.score += k(us_tmp) * (0.0-win_exp(-delta))  * 1.0*us_tmp.score/media_squadra2;
				us_tmp.losses++;
			}
			else if(i<num_squadra1 && risultato == 2)
			{
				us_tmp.score += k(us_tmp) * (0.5-win_exp(delta))  * 1.0*us_tmp.score/media_squadra1;
				us_tmp.draws++;
			}
			else if(i>=num_squadra1 && risultato == 2)
			{
				us_tmp.score += k(us_tmp) * (0.5-win_exp(-delta))  * 1.0*us_tmp.score/media_squadra2;
				us_tmp.draws++;
			}

			if(num_squadra2 > 1)
				us_tmp.tags++;
			if(us_tmp.score < 100)
				us_tmp.score = 100;
			database->setUserStats(us_tmp);
			
			log(INFO,"%s score: %d >(%+d)-> %d\n",us_tmp.username.c_str(),us[i].score,(us_tmp.score-us[i].score),us_tmp.score);
			
		}
		
	}
    catch (std::exception e)
    {

    }
	
}

void Users::UpdateStats(std::vector<LoggerPlayerInfo *> nomi, int risultato) //0= vittoria, 2 = pareggio
{
	std::vector<UserStats> us;
	
	unsigned int num_squadra1 = nomi.size()/2;
	unsigned int num_squadra2 = nomi.size() - num_squadra1;
	
	unsigned int media_squadra1=0;
	unsigned int media_squadra2=0;
	int delta = 0;
	try
    {
		int pos = 0;
		for(auto nome = nomi.cbegin(); nome != nomi.cend();++nome,++pos)
		{
			if((*nome)->name[0] == '-')
				return;
			UserStats us_tmp = database->getUserStats((*nome)->name);
			us.push_back(us_tmp);
			if(pos < num_squadra1)
				media_squadra1 += us_tmp.score;
			else
				media_squadra2 += us_tmp.score;
		}
		media_squadra1 /= num_squadra1;
		media_squadra2 /= num_squadra2;
		delta = media_squadra1 - media_squadra2;
			
		
		for(int i = 0;i<nomi.size();i++)
		{
			UserStats us_tmp = us[i];
			
			if(i<num_squadra1 && risultato == 0)
			{
				us_tmp.score += k(us_tmp) * (1.0-win_exp(delta))  * 1.0*us_tmp.score/media_squadra1;
				us_tmp.wins++;
			}
			else if(i>=num_squadra1 && risultato == 0)
			{
				us_tmp.score += k(us_tmp) * (0.0-win_exp(-delta))  * 1.0*us_tmp.score/media_squadra2;
				us_tmp.losses++;
			}
			else if(i<num_squadra1 && risultato == 2)
			{
				us_tmp.score += k(us_tmp) * (0.5-win_exp(delta))  * 1.0*us_tmp.score/media_squadra1;
				us_tmp.draws++;
			}
			else if(i>=num_squadra1 && risultato == 2)
			{
				us_tmp.score += k(us_tmp) * (0.5-win_exp(-delta))  * 1.0*us_tmp.score/media_squadra2;
				us_tmp.draws++;
			}

			if(num_squadra2 > 1)
				us_tmp.tags++;
			if(us_tmp.score < 100)
				us_tmp.score = 100;
			database->setUserStats(us_tmp,nomi[i]);
			
			log(INFO,"%s score: %d >(%+d)-> %d\n",us_tmp.username.c_str(),us[i].score,(us_tmp.score-us[i].score),us_tmp.score);
			
		}
		
	}
    catch (std::exception e)
    {

    }
	
}

void Users::Draw(std::string win1, std::string win2,std::string los1, std::string los2)
{
    std::vector<std::string> nomi;
	nomi.push_back(win1);
	nomi.push_back(win2);
	nomi.push_back(los1);
	nomi.push_back(los2);
	
	
	UpdateScore(nomi,2);
}

void Users::Victory(std::string win, std::string los)
{
    std::vector<std::string> nomi;
	nomi.push_back(win);
	nomi.push_back(los);	
	
	UpdateScore(nomi,0);

}
void Users::Victory(std::string win1, std::string win2,std::string los1, std::string los2)
{

    std::vector<std::string> nomi;
	nomi.push_back(win1);
	nomi.push_back(win2);
	nomi.push_back(los1);
	nomi.push_back(los2);
	
	
	UpdateScore(nomi,0);

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





