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
#include "common/debug.h"
#include "trueskill.h"
#include "MySqlWrapper.h"
namespace ygo
{
Users::Users(MySqlWrapper * m)
{
    database = new UsersDatabase(m);
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
        return Users::LoginResultTuple ("-Player",ygo::LoginResult::INVALIDUSERNAME,0);
    }

    return login(username,password,ip);
}

Users::LoginResultTuple Users::login(std::string username, std::string password,char* ip)
{
    log(INFO,"Tento il login con %s e %s, ip %s\n",username.c_str(),password.c_str(),ip);
    std::string usernamel=username;
    std::transform(usernamel.begin(), usernamel.end(), usernamel.begin(), ::tolower);
    if(username[0] == '-')
        return Users::LoginResultTuple (username,ygo::LoginResult::UNRANKED,0);

    if(int color = database->login(username,password,ip))
    {
        if(password != "")
            return Users::LoginResultTuple (username,ygo::LoginResult::AUTHENTICATED,color-1);
        else
            return Users::LoginResultTuple (username,ygo::LoginResult::NOPASSWORD,color-1);
    }
    else
    {
        return Users::LoginResultTuple ("-" + username,ygo::LoginResult::INVALIDPASSWORD,0);


    }
}

std::pair<int,int> Users::getScoreRank(std::string username)
{
    if(username[0] == '-')
        return std::pair<int,int>(0,0);

    try
    {
        std::pair<int,int> score = database->getScoreRank(username);
        return score;
    }
    catch(std::exception)
    {
        return std::pair<int,int>(0,0);
    }

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



std::string Users::getCountryCode(std::string ip)
{
    return database->getCountryCode(ip);



}


void Users::updateSkill(std::vector<LoggerPlayerData>  &nomi, int risultato)
{
	
	std::vector<trueskill::Player> players;
	std::vector<trueskill::Player> old;
	try
	{
		unsigned int nomisize = nomi.size();
		for(int i = 0; i < nomisize;i++)
		{
				if((nomi[i]).name[0] == '-')
					return;
				std::pair<double,double> skill = database->getTrueSkill(nomi[i].name);
				trueskill::Player player;
				player.mu = skill.first;
				player.sigma = skill.second;
				players.push_back(player);
				old.push_back(player);
		}
		
		
        trueskill::Team sopra;
		trueskill::Team sotto;
		if(risultato == 0)
		{
			sopra.rank = 1;
			sotto.rank = 2;
		}
		else if(risultato == 1)
		{
			sopra.rank = 2;
			sotto.rank = 1;
		}
		else if(risultato == 2)
		{
			sopra.rank = 1;
			sotto.rank = 1;
		}
		
		
		if(nomisize == 2)
		{
			players[0].weight = 1.0;
			players[1].weight = 1.0;
			sopra.members.push_back(&players[0]);
			sotto.members.push_back(&players[1]);
		}
		else if(nomisize == 3)
		{
			players[0].weight = 1.0;
			players[1].weight = 0.5;
			players[2].weight = 0.5;
			sopra.members.push_back(&players[0]);
			sotto.members.push_back(&players[1]);
			sotto.members.push_back(&players[2]);
		}
		else if(nomisize == 4)
		{
			players[0].weight = 0.5;
			players[1].weight = 0.5;
			players[2].weight = 0.5;
			players[3].weight = 0.5;
			sopra.members.push_back(&players[0]);
			sopra.members.push_back(&players[1]);
			sotto.members.push_back(&players[2]);
			sotto.members.push_back(&players[3]);
		}
		std::vector<trueskill::Team> teams;
		teams.push_back(sopra);
		teams.push_back(sotto);
		trueskill::adjust_teams(teams);
		for(int i = 0; i < nomisize;i++)
		{
			database->setTrueSkill(nomi[i].name,players[i].mu,players[i].sigma);
			printf("score %s: (%f,%f) -> (%f,%f)\n",nomi[i].name,old[i].mu, old[i].sigma, players[i].mu,players[i].sigma);
		}	
		
	}
	catch (std::exception e)
    {

    }
}



void Users::UpdateStats(std::vector<LoggerPlayerData> &nomi, int risultato) //0= vittoria, 2 = pareggio
{
	std::vector<UserStats> us;
	
	unsigned int num_squadra1 = nomi.size()/2;
	unsigned int num_squadra2 = nomi.size() - num_squadra1;
	
	
	try
    {
		int pos = 0;
		for(auto nome = nomi.cbegin(); nome != nomi.cend();++nome,++pos)
		{
			if((*nome).name[0] == '-')
				return;
			UserStats us_tmp = database->getUserStats((*nome).name);
			us.push_back(us_tmp);
			
		}
		
		
		for(int i = 0;i<nomi.size();i++)
		{
			UserStats us_tmp = us[i];
			
			if(risultato == 2)
				us_tmp.draws++;
			else if( (i<num_squadra1) == (risultato == 0 ))
				us_tmp.wins++;
			else if( (i<num_squadra1) == (risultato == 1 ))
				us_tmp.losses++;

			if(num_squadra2 > 1)
				us_tmp.tags++;
			
			if(nomi.size() == 2)
				database->setUserStats(us_tmp,nomi[i]);
			else
				database->setUserStats(us_tmp);
			
			
		}
		
	}
    catch (std::exception e)
    {

    }
	
}


void Users::saveDuelResult(std::vector<LoggerPlayerData> nomi, int risultato,std::string replay)
{
	std::vector<std::string> snomi;
	for(int i = 0; i < nomi.size();++i)
		snomi.push_back(nomi[i].name);
	
	updateSkill(nomi,risultato);
	UpdateStats(nomi,risultato);
	database->saveDuel(snomi,risultato,replay);
	
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





