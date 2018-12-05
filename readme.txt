1, unzip the program
2, store CactusValleyLE.SC2Map in path .../StarCraftII/Maps/CactusValleyLE.SC2MAP (create Maps directory if there is no Maps)
3, open Feeder.sln with visual studio
4, Replace/Modify the Run.cpp
	4.1, in Run.cpp: all testing codes must be in #ifdef DEBUG ...main() here... #else ...useless code here... #endif
5, set map path in Run like this coordinator.StartGame("/CactusValleyLE.SC2Map");
6, build F6
7, path to Feeder.exe is .../SC2Bot/x64/Debug/Feeder.exe