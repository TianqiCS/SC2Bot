Installation of Feeder:
1. unzip project.zip
2. make sure you have map "CactusValleyLE.SC2Map" in your game directory .../StarCraftII/Maps/CactusValleyLE.SC2MAP (create Maps directory if there is no Maps)
3. you can open the project by opening Feeder.sln with visual studio
4. if you need to replace / modify the codes to run the game. It's in run.cpp.
	4.1. In run.cpp, you have to use DEBUG mode to compile the program. (#ifdef DEBUG ...main() here... #else ...only useless code here... #endif)
	4.2. there are more instrctions in default Run.cpp if needed.
5. you should set the map path in codes like this, coordinator.StartGame("/CactusValleyLE.SC2Map");
6. you should build the whole project, which is F6
7. the location of the executable is .../SC2Bot/x64/Debug/Feeder.exe

Happy using!