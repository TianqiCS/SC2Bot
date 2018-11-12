import sc2, sys
from __init__ import run_ladder_game
from sc2 import Race, Difficulty
from sc2.player import Bot, Computer

# Load bot
from SnowBot import SnowBot
bot = Bot(Race.Zerg, SnowBot())
protossComputer = Computer(Race.Protoss, Difficulty.Harder)
terranComputer = Computer(Race.Terran, Difficulty.Harder)
zergComputer = Computer(Race.Zerg, Difficulty.Harder)
# Start game
mapsList = ['16BitLE', 'AcidPlantLE', 'CatalystLE', 'DarknessSanctuary', 'LostAndFoundLE', 'MechDepotLE', 'Redshift'] # not on laptop currently

if __name__ == '__main__':
    if "--LadderServer" in sys.argv:
        # Ladder game started by LadderManager
        print("Starting ladder game...")
        run_ladder_game(bot)
    else:
        # Local 
        folder = 'localReplays/'

#         for i in range(1000): doesnt work right now for some reason  --> starts expanding to random places after first game
#             print("Starting local game...")
#             if i % 3 == 0:
#                 sc2.run_game(sc2.maps.get('AcidPlantLE'), [Bot(Race.Zerg, SnowBot()),Computer(Race.Protoss, Difficulty.Harder)], realtime=False, save_replay_as=str(folder + 'Protoss' + mapsList[i % 7] + str(i) + '.SC2Replay'))
#             elif (i+1) % 3 == 0:
#                 sc2.run_game(sc2.maps.get(mapsList[i % 7]), [Bot(Race.Zerg, SnowBot()),Computer(Race.Terran, Difficulty.Harder)], realtime=False, save_replay_as=str(folder + 'Terran' + mapsList[i % 7]+ str(i) + '.SC2Replay'))
#             else:
#                 sc2.run_game(sc2.maps.get(mapsList[i % 7]), [Bot(Race.Zerg, SnowBot()),Computer(Race.Zerg, Difficulty.Harder)], realtime=False, save_replay_as=str(folder + 'Zerg' + mapsList[i % 7]+ str(i) + '.SC2Replay'))

        
        sc2.run_game(sc2.maps.get('AbyssalReefLE'), [bot, protossComputer], realtime=False, save_replay_as=str('SnowBot.SC2Replay'))

        #has trouble with terran tanks/ bio  --> a moves to the nice choke

        
        # possible maps to test on:
        # 4 player: (4)DarknessSanctuaryLE , 
        # 2 player: (2)LostandFoundLE, (2)DreamcatcherLE, (2)AcidPlantLE, (2)16-BitLE, CatalystLE
        # special: (2)RedshiftLE
        