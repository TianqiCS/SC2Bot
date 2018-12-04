1, unzip the program
2, store CactusValleyLE.SC2Map in .../StarCraftII/Maps/CactusValleyLE.SC2MAP (create Maps directory if there is no Maps)
3, open Feeder.sln with visual studio
4, Replace/Modify the Run.cpp
5, set map path in Run like this coordinator.StartGame("/CactusValleyLE.SC2Map");
6, build
7, Feeder.exe is in .../SC2Bot/x64/Debug/Feeder.exe