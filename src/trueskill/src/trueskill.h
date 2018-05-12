#ifndef INC_TRUESKILL_H
#define INC_TRUESKILL_H


#include <vector>

#include <iostream>

namespace trueskill
{

class Player {
public:
  double mu;
  double sigma;
  double weight;
};

std::ostream& operator<<(std::ostream &strm, const Player &p);


class Team{
	public:
	std::vector<Player*> members;
	int rank;
};




  void adjust_teams(std::vector<Team> teams);

  double win_probability(Player* player1, Player * player2);
}


#endif /* INC_TRUESKILL_H */
