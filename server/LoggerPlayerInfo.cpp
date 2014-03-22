
#include <stdio.h>
#include "DuelLogger.h"

LoggerPlayerInfo::LoggerPlayerInfo():type(7),turns(0),maxSpSummonTurn(0),maxAttacksTurn(0),maxDamage1shot(0),recoveredDuel(0),
setMonstersDuel(0),effectsDuel(0),setSTDuel(0)
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

void LoggerPlayerInfo::SetMonster()
{
	if(!hisTurn)
		return;
	debugp("set monster\n");
	setMonstersDuel++;
}
	void LoggerPlayerInfo::Effect()
	{
		if(!hisTurn)
		return;
		debugp("Effect\n");
		effectsDuel++;
		
	}
	void LoggerPlayerInfo::SetST()
	{
		if(!hisTurn)
		return;
		debugp("set st\n");
		setSTDuel++;
		
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
	if(ID == (1-playerID))
	{
	if(maxDamage1shot < damage)
		maxDamage1shot = damage;
	debugp("%s inflicts %d damage\n",name,damage);
	
	}
	
	
}
void LoggerPlayerInfo::Recover(signed char ID, int damage)
{
	if(!hisTurn)
		return;
	if(ID == playerID)
	{
		recoveredDuel += damage;
		debugp("%s recovers %d damage\n",name,damage);
	
	}

	
}
void LoggerPlayerInfo::TypeChange(unsigned char t)
{
	type = t;
}