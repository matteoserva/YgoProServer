#ifndef _DUEL_LOGGER_
#define _DUEL_LOGGER_
#include <stdint.h>
#include <cstddef>
#include <map>

void debugp(const char *format, ...);

class LoggerPlayerInfo
{
public:
	LoggerPlayerInfo();
	unsigned char type;
	unsigned char ultimo_game_msg;
	bool hisTurn;
	char name[21];
	unsigned int turns;
	unsigned int SpSummonTurn;
	unsigned int maxSpSummonTurn;
	unsigned int maxAttacksTurn;
	unsigned int attacksTurn;
	signed char playerID;
	int maxDamage1shot;
	unsigned int recoveredDuel;
	unsigned int setMonstersDuel;
	unsigned int effectsDuel;
	unsigned int setSTDuel;
	
	void NewTurn();
	void MainPhase();
	void SpecialSummon();
	void TypeChange(unsigned char);
	void attack();
	void Damage(signed char, int);
	void Recover(signed char, int);
	void SetMonster();
	void Effect();
	void SetST();
};

class DuelLogger
{
	std::map<uintptr_t, LoggerPlayerInfo> players;
	public:
	DuelLogger();
	~DuelLogger();
	void LogClientMessage(uintptr_t dp,unsigned char, char*buffer,size_t size);
	void LogServerMessage(uintptr_t dp,unsigned char, char*buffer,size_t size);
	LoggerPlayerInfo* getPlayerInfo(uintptr_t dp);
	
};
#endif