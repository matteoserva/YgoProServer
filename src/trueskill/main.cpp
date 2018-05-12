
#include "trueskill.h"
#include <iostream>
#include <stdio.h>

void simple_example()
{
    trueskill::Player alice ;
    alice.mu = 35.0;
    alice.sigma = 4;
    alice.weight = 0.5;

    trueskill::Player bob ;
    bob.mu = 25.0;
    bob.sigma = 4;
    bob.weight = 0.5;

    trueskill::Player chris ;
    chris.mu = 25.0;
    chris.sigma = 4;
    chris.weight = 1.0;
	
    trueskill::Player darren ;
    darren.mu = 25.0;
    darren.sigma = 4;
    darren.weight = 0.5;



	std::vector<trueskill::Team> teams;
	trueskill::Team red;
	red.members.push_back(&alice);
	red.members.push_back(&bob);
	red.rank = 1;
	teams.push_back(red);

	trueskill::Team blue;
	blue.members.push_back(&chris);
	
	blue.rank = 1;
	teams.push_back(blue);
	


    // Do the computation to find each player's new skill estimate.

    trueskill::adjust_teams(teams);
	
   

	std::cout << " Alice: " << alice << std::endl;
	std::cout << "   Bob: " << bob << std::endl;
	std::cout << " Chris: " << chris << std::endl;
	std::cout << "Darren: " << darren << std::endl;
	getchar();
   

   
}

int main()
{
   
    simple_example();
    return 0;
}
