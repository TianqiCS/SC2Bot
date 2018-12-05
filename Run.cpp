#include "sc2api/sc2_api.h"
#include "sc2lib/sc2_lib.h"
#include "sc2utils/sc2_manage_process.h"
#include "sc2utils/sc2_arg_parser.h"
#include "LadderInterface.h"

#include "feeder.h"
#include <iostream>

#ifdef DEBUG

/**
 * Sample testing script
 *
 * All test codes should be in following main function
 *
 * set map path = coordinator.StartGame("/CactusValleyLE.SC2Map");
 * Feeder bot; is bot Feeder to be tested
 */

int main(int argc, char* argv[])
{
	Feeder bot; // bot here
	sc2::Coordinator coordinator;
	if (!coordinator.LoadSettings(argc, argv))
	{
		std::cout << "Unable to find or parse settings." << std::endl;
		return 1;
	}
	coordinator.SetWindowSize(1080, 720);
	coordinator.SetStepSize(100); 
	coordinator.SetRealtime(false);
	coordinator.SetMultithreaded(true);
	coordinator.SetParticipants({
		CreateParticipant(sc2::Race::Terran, &bot), // create Feeder in game here
		CreateComputer(sc2::Race::Terran, CheatInsane)
		});
	// Start the game.
	coordinator.LaunchStarcraft();
	coordinator.StartGame("/CactusValleyLE.SC2Map"); // map path here


	// Step forward the game simulation.
	while (coordinator.Update())
	{
	}

	return 0;
}

// following #else should not be used, please, it might CRASH!!
#else

int main(int argc, char* argv[])
{
	RunBot(argc, argv, new Feeder(), sc2::Race::Terran);

	return 0;
}
#endif
