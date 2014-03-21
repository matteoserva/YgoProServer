
#include <stdio.h>
#include "DuelLogger.h"

LoggerPlayerInfo::LoggerPlayerInfo():type(7),turns(0),maxSpSummonTurn(0),maxAttacksTurn(0),maxDamage1shot(0)
{
	
}

void LoggerPlayerInfo::NewTurn()
{
	hisTurn = false;
	turns++;
	SpSummonTurn = 0;
	attacksTurn = 0;
}
	void LoggerPlayerInfo::MainPhase()
	{
		hisTurn = true;
	}
void LoggerPlayerInfo::SpecialSummon()
{
	if(!hisTurn)
		return;
	SpSummonTurn++;
	if(SpSummonTurn > maxSpSummonTurn)
		maxSpSummonTurn = SpSummonTurn;
	debugp("%s special summons\n",name);
}

void LoggerPlayerInfo::attack()
{
	if(!hisTurn)
		return;
	attacksTurn++;
	if(attacksTurn > maxAttacksTurn)
		maxAttacksTurn = attacksTurn;
	debugp("%s attacks summons\n",name);
}

void LoggerPlayerInfo::Damage(signed char ID, int damage)
{
	if(!hisTurn)
		return;
	if(ID != (1-playerID))
		return;
	if(maxDamage1shot < damage)
		maxDamage1shot = damage;
	debugp("%s inflicts %d damage\n",name,damage);
}

void LoggerPlayerInfo::TypeChange(unsigned char t)
{
	type = t;
}