#ifndef _COMMON_STRUCTURES_H
#define _COMMON_STRUCTURES_H

struct LoggerPlayerData
{
	unsigned int maxSpSummonTurn;
	unsigned int maxAttacksTurn;
	unsigned int turns;
	int maxDamage1shot;
	unsigned int recoveredDuel;
	unsigned int setMonstersDuel;
	unsigned int effectsDuel;
	unsigned int setSTDuel;
	char name[21];
};
namespace ygo
{
enum class LoginResult {NOTENTERED,WAITINGJOIN,WAITINGJOIN2,UNRANKED,INVALIDUSERNAME,INVALIDPASSWORD,NOPASSWORD,AUTHENTICATED};
}



#endif
