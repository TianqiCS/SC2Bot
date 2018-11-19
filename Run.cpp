#include "sc2api/sc2_api.h"
#include "sc2lib/sc2_lib.h"
#include "sc2utils/sc2_manage_process.h"
#include "sc2utils/sc2_arg_parser.h"
#include "LadderInterface.h"

#include "feeder.h"
#include <iostream>

#ifdef DEBUG

/*int main(int argc, char* argv[])
{
	/*
	Feeder bot;
	sc2::Coordinator coordinator;
	if (!coordinator.LoadSettings(argc, argv))
	{
		std::cout << "Unable to find or parse settings." << std::endl;
		return 1;
	}
	coordinator.SetWindowSize(1920, 1080);
	coordinator.SetStepSize(100); // speed
	coordinator.SetRealtime(false);
	coordinator.SetMultithreaded(true);
	coordinator.SetParticipants({
		CreateParticipant(sc2::Race::Terran, &bot),
		//sc2::PlayerSetup(sc2::PlayerType::Observer,Util::GetRaceFromString(enemyRaceString)),
		CreateComputer(sc2::Race::Random, VeryHard)
	});
	// Start the game.
	coordinator.LaunchStarcraft();
	coordinator.StartGame("/CactusValleyLE.SC2Map");
	//coordinator.StartGame("Interloper LE");


	// Step forward the game simulation.
	while (coordinator.Update())
	{
	}
	
	RunBot(argc, argv, new Feeder(), sc2::Race::Terran);
	return 0;
}
*/
// This main function is for quick local demo only
// please DO NOT EDIT/DELETE


int main(int argc, char* argv[])
{
	// result file
	//FILE * pFile;
	//pFile = fopen("data.txt", "w");

	Feeder bot;
	sc2::Coordinator coordinator;
	if (!coordinator.LoadSettings(argc, argv))
	{
		std::cout << "Unable to find or parse settings." << std::endl;
		return 1;
	}
	coordinator.SetWindowSize(1080, 720);
	coordinator.SetStepSize(100); // speed
	coordinator.SetRealtime(false);
	coordinator.SetMultithreaded(true);
	coordinator.SetParticipants({
		CreateParticipant(sc2::Race::Terran, &bot),
		//sc2::PlayerSetup(sc2::PlayerType::Observer,Util::GetRaceFromString(enemyRaceString)),
		CreateComputer(sc2::Race::Random, Easy)
		});
	// Start the game.
	coordinator.LaunchStarcraft();
	coordinator.StartGame("/CactusValleyLE.SC2Map");
	//coordinator.StartGame("Interloper LE");


	// Step forward the game simulation.
	while (coordinator.Update())
	{
	}

	//RunBot(argc, argv, new Feeder(), sc2::Race::Terran);
	//fclose(pFile);
	return 0;
}


#else

int main(int argc, char* argv[])
{
	RunBot(argc, argv, new Feeder(), sc2::Race::Terran);

	return 0;
}
#endif
