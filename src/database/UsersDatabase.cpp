#include "UsersDatabase.h"
#include "common/Config.h"
#include <memory> //unique_ptr
#include <cppconn/prepared_statement.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "MySqlWrapper.h"
namespace ygo
{
int UsersDatabase::getRank(std::string username)
{

    try
    {
        sql::Connection *con = mySqlWrapper->getConnection();
        //std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("SELECT rank from (SELECT username,score, @rownum := @rownum + 1 AS rank FROM stats, (SELECT @rownum := 0) r ORDER BY score DESC) z where username = ?"));
        std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("SELECT rank from ranking where username = ?"));

        stmt->setString(1, username);

        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        if(!res->next())
            return 0;
        int rank = res->getInt(1);
        return rank;
    }
    catch (sql::SQLException &e)
    {
        return 0;
    }
}

std::string UsersDatabase::getCountryCode(std::string ip)
{



    std::string binaryIP = "";
    binaryIP.resize(4);
    if(inet_pton(AF_INET,ip.c_str(),&binaryIP[0]) != 1)
        return "UNK";

    try
    {
        sql::Connection *con = mySqlWrapper->getConnection();

        std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("select country from `dbip_lookup` where addr_type = 'ipv4' and ip_start <= ? order by ip_start desc limit 1"));

        stmt->setString(1, binaryIP);

        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        if(!res->next())
            return "UNK";
        std::string country = res->getString(1);
        return country;
    }

    catch (sql::SQLException &e)
    {
        return "UNK";

    }



}




std::pair<int,int> UsersDatabase::getScoreRank(std::string username)
{

    try
    {
        sql::Connection *con = mySqlWrapper->getConnection();

        std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("select ranking.score,ranking.rank from ranking  where ranking.username = ?"));

        stmt->setString(1, username);

        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        if(!res->next())
            return std::pair<int,int>(0,0);
        int score = res->getInt(1);
        int gameScore = res->getInt(2);
        return std::pair<int,int>(score,gameScore);
    }
    catch (sql::SQLException &e)
    {
        return std::pair<int,int>(0,0);

    }
}

bool UsersDatabase::setUserStats(UserStats &us)
{

    //true is success
    int retries = 3;
    do
    {
        try
        {
                    sql::Connection *con = mySqlWrapper->getConnection();

            std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("UPDATE stats SET score = ?, wins = ?, losses = ?, draws = ?,tags = ? WHERE username = ?"));
            //stmt->setQueryTimeout(5);
            stmt->setString(6, us.username);
            stmt->setInt(1, us.score);
            stmt->setInt(2, us.wins);
            stmt->setInt(3, us.losses);
            stmt->setInt(4, us.draws);
            stmt->setInt(5, us.tags);
            int updateCount = stmt->executeUpdate();
            return updateCount > 0;
        }
        catch (sql::SQLException &e)
        {
            mySqlWrapper->notifyException(e);
        }
    }
    while (--retries > 0);
    return false;
}

bool UsersDatabase::setUserStats(UserStats &us,LoggerPlayerData &lpi)
{

    //true is success
    int retries = 3;
    do
    {
        try
        {
                    sql::Connection *con = mySqlWrapper->getConnection();

            std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement(
			"UPDATE stats SET score = ?, wins = ?, losses = ?, draws = ?,tags = ? ,maxspsummonsturn = GREATEST(maxspsummonsturn , ?), longestduel = GREATEST(longestduel, ?),maxattacksturn = GREATEST(maxattacksturn, ?), maxdamage1shot = GREATEST(maxdamage1shot,?), "
			"recoveredduel = GREATEST(recoveredduel,?),  setmonstersduel = GREATEST(setmonstersduel,?), effectsduel = GREATEST(effectsduel,?), setstduel = GREATEST(setstduel,?) WHERE username = ?"));
            //stmt->setQueryTimeout(5);
            
            stmt->setInt(1, us.score);
            stmt->setInt(2, us.wins);
            stmt->setInt(3, us.losses);
            stmt->setInt(4, us.draws);
            stmt->setInt(5, us.tags);
			
			stmt->setInt(6,lpi.maxSpSummonTurn);
			stmt->setInt(7,lpi.turns);
			stmt->setInt(8,lpi.maxAttacksTurn);
			stmt->setInt(9,lpi.maxDamage1shot);
			int i = 10;
			stmt->setInt(i++,lpi.recoveredDuel);
			stmt->setInt(i++,lpi.setMonstersDuel);
			stmt->setInt(i++,lpi.effectsDuel);
			stmt->setInt(i++,lpi.setSTDuel);
			stmt->setString(i++, us.username);
            int updateCount = stmt->executeUpdate();
            return updateCount > 0;
        }
        catch (sql::SQLException &e)
        {
            mySqlWrapper->notifyException(e);
        }
    }
    while (--retries > 0);
    return false;
}

UserStats UsersDatabase::getUserStats(std::string username)
{

    //true is success
    try
    {
                sql::Connection *con = mySqlWrapper->getConnection();

        std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("SELECT username,score,wins,losses,draws,tags FROM stats where username = ?"));
        //sql::PreparedStatement *stmt = con->prepareStatement("INSERT INTO users VALUES (?, ?, ?)");
        stmt->setString(1, username);
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        if(!res->next())
            throw std::exception();
        UserStats us;
        us.username =res->getString(1);
        us.score = res->getInt(2);
        us.wins = res->getInt(3);
        us.losses = res->getInt(4);
        us.draws =res->getInt(5);
        us.tags =res->getInt(6);
        return us;

    }
    catch (sql::SQLException &e)
    {
        mySqlWrapper->notifyException(e);
        throw std::exception();
    }

}


bool UsersDatabase::createUser(std::string username, std::string password, int score,int wins,int losses,int draws)
{

    try
    {
                sql::Connection *con = mySqlWrapper->getConnection();

        std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("INSERT INTO users(username,password) VALUES (?, ?)"));
        //sql::PreparedStatement *stmt = con->prepareStatement("INSERT INTO users VALUES (?, ?, ?)");
        stmt->setString(1, username);
        stmt->setString(2, password);
        stmt->execute();
        std::unique_ptr<sql::PreparedStatement> stmt_stats(con->prepareStatement("INSERT INTO stats(username,score,wins,losses,draws) VALUES (?, ?, ?,?,?)"));
        stmt_stats->setString(1, username);
        stmt_stats->setInt(2,score);
        stmt_stats->setInt(3,wins);
        stmt_stats->setInt(4,losses);
        stmt_stats->setInt(5,draws);
        stmt_stats->execute();
    }
    catch (sql::SQLException &e)
    {
        mySqlWrapper->notifyException(e);
        return false;
    }
    return true;
}

bool UsersDatabase::userExists(std::string username)
{

    try
    {
                sql::Connection *con = mySqlWrapper->getConnection();

        std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("select count(*) FROM users WHERE username = ?"));
        stmt->setString(1, username);

        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        res->next();
        int trovato = res->getInt(1);
        return (trovato>0);
    }
    catch (sql::SQLException &e)
    {
        std::cout<<"errore in userexists\n";
        return false;
    }
    return true;
}





 std::pair<double,double> UsersDatabase::getTrueSkill(std::string username)
 {
    std::pair<double,double> result;
    result.first = result.second = 0.0;
     try
    {
        sql::Connection *con = mySqlWrapper->getConnection();

        std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("select ts_mu, ts_sigma FROM stats WHERE username = ?"));
        stmt->setString(1, username);
        
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        if(!res->next())
        {
            return result;
        }

	result.first = res->getDouble(1);
	result.second = res->getDouble(2);
        
       
        return result;
    }
    catch (sql::SQLException &e)
    {
        mySqlWrapper->notifyException(e);
        std::cout<<"errore in userexists\n";
        return result;
    }
   
   
 }
 
 bool UsersDatabase::saveDuel(std::vector<std::string> names, int result,std::string replay)
 {
	 try
      {
		  sql::Connection *con = mySqlWrapper->getConnection();

	
	  //login success

	  std::unique_ptr<sql::PreparedStatement> stmt2(con->prepareStatement("INSERT INTO duels_history(player1,player2,player3,player4,outcome,replay) values(?,?,?,?,?,?)"));
	  stmt2->setString(1,names[0]);
	  stmt2->setString(2,names[1]);
	  if(names.size() > 2)
		  stmt2->setString(3,names[2]);
		
	  else
		stmt2->setNull(3,sql::DataType::SQLNULL);
	  if(names.size() > 3)
		  stmt2->setString(4,names[3]);
		
	  else
		stmt2->setNull(4,sql::DataType::SQLNULL);
	 
	  stmt2->setInt(5,result);
	  stmt2->setString(6,replay);

	  stmt2->execute();
	  return true;
      }
      catch (sql::SQLException &e)
      {
		  mySqlWrapper->notifyException(e);
		  std::cout<<"errore in saveDuel\n";
		  return false;
      }
	 
	 
	 
 }
 
 
    void UsersDatabase::setTrueSkill(std::string username,double mu, double sigma)
    {
      
       try
      {
		  sql::Connection *con = mySqlWrapper->getConnection();

	
	  //login success

	  std::unique_ptr<sql::PreparedStatement> stmt2(con->prepareStatement("UPDATE stats set ts_mu=?, ts_sigma=? WHERE username = ?"));
	  stmt2->setDouble(1,mu);
	  stmt2->setDouble(2,sigma);
	  stmt2->setString(3, username);

	  stmt2->execute();
	  return;
      }
      catch (sql::SQLException &e)
      {
	  mySqlWrapper->notifyException(e);
	  std::cout<<"errore in setrueskill\n";
	  return;
      }
      
      
    }





int UsersDatabase::login(std::string username,std::string password,char*ip)
{

    try
    {
        sql::Connection *con = mySqlWrapper->getConnection();

        std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("select password,color FROM users WHERE username = ?"));
        stmt->setString(1, username);
        //stmt->setString(2, password);
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        if(!res->next())
        {
            createUser(username,password);
            return 1;
        }

        int risultato = 1;
        std::string realPassword = res->getString(1);
        if(realPassword != "" && password != realPassword)
            return 0;
        else if(realPassword != "" && password == realPassword)
            risultato = 1+res->getInt(2);
        //login success

        std::unique_ptr<sql::PreparedStatement> stmt2(con->prepareStatement("UPDATE users set password = ?, last_login = CURRENT_TIMESTAMP,last_ip = ? WHERE username = ?"));
        stmt2->setString(1,password);
        stmt2->setString(2,ip);
        stmt2->setString(3, username);

        stmt2->execute();
        return risultato;
    }
    catch (sql::SQLException &e)
    {
        mySqlWrapper->notifyException(e);
        std::cout<<"errore in userexists\n";
        return 0;
    }
}

UsersDatabase::UsersDatabase(MySqlWrapper * m)
{
	mySqlWrapper = m;
}
UsersDatabase::~UsersDatabase()
{

}


}
