'''
Created on Jun 25, 2018

@author: Alex Jonson

'''
#python3 directory
#d:\users\alex\appdata\local\programs\python\python36\lib\site-packages

#https://github.com/Dentosal/python-sc2 --- python-sc2 Github


import sc2
import random
from CustomBaseBot import BaseBot
from sc2 import run_game, maps, Race, Difficulty
from sc2.player import Bot, Computer
from sc2.constants import *

class SnowBot(BaseBot):
    
    def __init__(self):
        self.ITERATION_PER_MINUTE = 165 #OBSOLETE --> should use ingame time calculation in customBaseBot
        self.SECONDS_PER_MINUTE = 60
        self.gameTime = 0
        self.GuessedDroneCountChange = 0 #all "stats" only update at the start of the step  they dont proactivly update --> these keep track of changes as it goes on --> makes bot smarter so it wont over make stuff
        self.GuessedArmySupplyChange = 0
        self.GuessedMineralsSpent = 0
        self.GuessedGasSpent = 0
        self.firstScoutHappened = False
        self.movedFirstOverlord = False
        self.enemyIsGoingForAir = False
        self.sendMessage = True
        self.currentVersion = '1.0.1' #update when submitted to ladder: last submitted 7/10/2018
        
    # might be worth to rewrite somethings without using loops --> might solve overbuilding
    # add in scouting? --> what would we do with the scouting info
    # add in ability to make banes, hydras, lurkers, ultras --> ultra = good a move... etc... may try adding infestors to spell cast --> have them follow army around and cast fungal
    # ability to get hive and hive stufffs --> need to be able to get hydras vs void ray bs --> if stargate scouted --> make hydra instead of roach? or if voidray spotted make hydra instead of roach?
    # might be good to attack once early game then wait till brood lords and just defend or some higher tech (hydra BL or hydra ultra ling good endgame
    async def on_step(self, iteration):
        self.iteration = iteration
        self.gameTime = self.get_game_time()
        hatches = self.units(HATCHERY) | self.units(LAIR) | self.units(HIVE)
        droneCountChange = 0
        await self.distribute_workers()
        await self.roachMicro()
        await self.build_workers(hatches, droneCountChange)
        await self.build_overlords(hatches)
        await self.build_spawningpool(hatches)
        await self.build_lair()
        await self.manageLarva(hatches)
        await self.expandWell()
        await self.buildgas(hatches)
        await self.buildqueens(hatches)
        await self.larvainject(hatches)
        await self.controlArmy(hatches)
        await self.getUpgrades(hatches)
        await self.transfuse()
        await self.build_roachwarren(hatches)
        await self.build_evolution(hatches)
        await self.morphOverseer()
        await self.build_hydraden(hatches)
        await self.build_infestationpit(hatches)
        await self.build_hive()
        await self.initialScout()
        await self.moveFirstOverlord()
        await self.setup_defense(hatches) #currently causes big delay in 4th hatch
        await self.build_spire(hatches)
        await self.build_greateSpire()
        await self.morphBroodlord()
        await self.enemyGoingAir()
        await self.tell_version()
        await self.cancel_buildings()        
        #must go at the end!!!
        await self.execute_order_queue() #https://github.com/Hannessa/sc2-bots/blob/master/cannon-lover/base_bot.py -- how to use order Queue because performing 1 do at the end is more efficient 
        
    async def tell_version(self):
            if(self.sendMessage):
                self.sendMessage = False
                message = str('SnowBot: ' + self.currentVersion)
                await self.chat_send(message) #busted on desktop for some reason
                
    async def build_workers(self, hatches, droneCountChange):
        if not self.needArmyNow(hatches): #potentially add make wait for important stuff to happen
            for larva in self.units(LARVA).ready:
                if(self.can_afford(DRONE) and (self.supply_left > 1 or self.already_pending(OVERLORD)) and self.supply_left > 0 and hatches.ready.amount * 20 >= len(self.units(DRONE)) and (self.units(DRONE).amount + droneCountChange + self.already_pending(DRONE)) < 75): #75 worked well
                    droneCountChange = droneCountChange + 1
                    await self.do(larva.build(DRONE))
    
    def needArmyNow(self, hatches): #pause spending on everything except army
        #if need army dont waste larva on drones
        if(len(self.known_enemy_units.not_structure) > 2):
            for hatch in hatches:
                friendlyCounter = self.computeArmyValue()
                enemyCounter = 0
                for enemy in self.known_enemy_units.not_structure:
                    if enemy.distance_to(hatch.position) < 25 or enemy.distance_to(self.game_info.map_center) < 25:
                        enemyCounter += 1
                    if enemyCounter >= friendlyCounter - 6:
                        return True
        return False
    
    def computeArmyValue(self): #returns army supply used (not by drones)
        return self.units(ROACH).ready.amount + self.units(HYDRALISK).ready.amount + self.units(BROODLORD).ready.amount + (self.units(ZERGLING).ready.amount / 2)
    
    async def initialScout(self):
        if not self.firstScoutHappened:
            self.firstScoutHappened = True
            positionsToScout = self.enemy_start_locations
            scout = self.units(DRONE).ready[0]
            for s in positionsToScout:
                await self.do(scout.move(s, queue=True))
            await self.do(scout.move(self.start_location, queue=True))
            
    async def moveFirstOverlord(self): #moves to center currently
        if self.movedFirstOverlord is False:
            self.movedFirstOverlord = True
            overlord = self.units(OVERLORD).ready[0]
            await self.do(overlord.move(self.game_info.map_center))
                
    async def setup_defense(self, hatches): #builds spores and spines near hatches
        if (self.gameTime > self.SECONDS_PER_MINUTE * 5 and hatches.amount > 3 or self.gameTime > self.SECONDS_PER_MINUTE * 7) and await self.hasLair(): #after 5 mins build defense #waits till 4th is down
            spines = self.units(SPINECRAWLER)
            spores = self.units(SPORECRAWLER)
            for h in hatches.ready:
                if self.can_afford(SPINECRAWLER) and self.units(SPAWNINGPOOL).ready.amount > 0:
                    if len(spines) is 0:
                        await self.build(SPINECRAWLER, near=h.position)
                    elif spines.closest_to(h.position).distance_to(h.position) > 10 and not self.already_pending(SPINECRAWLER) > 2: # dont build more than x amount  fail safe bec wont deal with drones pending correctly
                        await self.build(SPINECRAWLER, near = h.position)
                if self.can_afford(SPORECRAWLER) and self.units(SPAWNINGPOOL).ready.amount > 0:
                    if len(spores) is 0:
                        await self.build(SPORECRAWLER, near=h.position)
                    elif spores.closest_to(h.position).distance_to(h.position) > 10 and not self.already_pending(SPORECRAWLER) > 2:
                        await self.build(SPORECRAWLER, near=h.position)
#     async def spread_creep(self): # design idea --> place as far away from other tumors as possible and keep queens from wasting all energy on tumors
#         creepTumors = self.units(CREEPTUMOR) | self.units(CREEPTUMORQUEEN) | self.units(CREEPTUMORBURROWED)
#         if creepTumors.amount < 5:
#             queens = self.units(QUEEN).ready.idle
#             for q in queens:
#                 if q.energy >= 25:
#                         await self.build(unit=q, near=random.sample(self.expansion_locations, 1)[0])
#         for tumor in creepTumors:
#             abilities = await self.get_available_abilities(tumor)
#             if len(abilities) > 0:
#                 position = await self.find_placement(building=CREEPTUMOR, near=random.sample(self.expansion_locations, 1)[0])
#                 if position is not None:
#                     await self.do(tumor(abilities[0], position))
            
    async def build_overlords(self, hatches): # makes a lot of overlords at once if has money
        for larva in self.units(LARVA).ready:
            if self.can_afford(OVERLORD) and self.supply_left < 5 * len(hatches) and self.already_pending(OVERLORD) < len(hatches.ready) and self.supply_cap < 200:
                await self.do(larva.train(OVERLORD))
                
    async def build_spawningpool(self, hatches):
        if self.can_afford(SPAWNINGPOOL) and self.supply_used > 14 and not self.already_pending(SPAWNINGPOOL) and len(self.units(SPAWNINGPOOL)) < 1:
            if hatches.exists:
                await self.build(SPAWNINGPOOL, near=hatches.first, placement_step=4)
                
    async def build_roachwarren(self, hatches):
        if self.can_afford(ROACHWARREN) and self.units(SPAWNINGPOOL).ready.amount > 0 and self.units(DRONE).amount > 30 and not self.units(ROACHWARREN).amount > 0 and not self.already_pending(ROACHWARREN):
            if hatches.exists:
                await self.build(ROACHWARREN, near=hatches.first, placement_step=4)
                
    async def build_evolution(self, hatches):
        if self.can_afford(EVOLUTIONCHAMBER) and self.units(EVOLUTIONCHAMBER).amount + self.already_pending(EVOLUTIONCHAMBER) < 3 and self.gameTime > 6 * self.SECONDS_PER_MINUTE and hatches.amount > 2:
            await self.build(EVOLUTIONCHAMBER, near=hatches.first, placement_step=4)
    
    async def build_spire(self, hatches):
        if self.can_afford(SPIRE) and self.units(SPIRE).amount < 1 and self.already_pending(SPIRE) < 1 and self.units(INFESTATIONPIT).amount > 0 and self.units(GREATERSPIRE).amount < 1:
            await self.build(SPIRE, near=hatches.first, placement_step=4)
    
    async def build_greateSpire(self):
        if self.can_afford(GREATERSPIRE) and self.units(SPIRE).ready.noqueue.amount > 0 and self.already_pending(GREATERSPIRE) < 1 and self.units(GREATERSPIRE).amount < 1 and self.units(HIVE).ready.amount > 0:
            await self.do(self.units(SPIRE).ready[0](UPGRADETOGREATERSPIRE_GREATERSPIRE))
    
    async def build_hydraden(self, hatches): #builds a hydra den after 6 min once has money
        if self.can_afford(HYDRALISKDEN) and self.units(HYDRALISKDEN).amount < 1 and not self.already_pending(HYDRALISKDEN) and hatches(LAIR).ready.amount > 0 and self.gameTime > self.SECONDS_PER_MINUTE * 5:
            await self.build(HYDRALISKDEN, near=hatches.first, placement_step=4)
            
    async def build_infestationpit(self, hatches):
        if self.can_afford(INFESTATIONPIT) and self.units(INFESTATIONPIT).amount < 1 and not self.already_pending(INFESTATIONPIT) and hatches(LAIR).ready.amount > 0 and self.units(HYDRALISKDEN).ready.amount > 0:
            await self.build(INFESTATIONPIT, near=hatches.first, placement_step=4)
            
    async def build_lair(self): #builds lairs until 1 is finished
        if self.can_afford(LAIR) and self.units(DRONE).amount > 24 and not await self.hasLair() and self.units(HIVE).amount < 1 and self.units(HATCHERY).ready.noqueue.amount > 0:
            await self.do(self.units(HATCHERY).ready.noqueue[0](UPGRADETOLAIR_LAIR))    
            
    async def build_hive(self):
        if self.can_afford(HIVE) and self.units(LAIR).ready.noqueue.amount == self.units(LAIR).ready.amount and self.units(DRONE).amount > 50 and not self.units(HIVE).amount > 0 and self.units(LAIR).amount > 0:
            await self.do(self.units(LAIR).ready.noqueue[0](UPGRADETOHIVE_HIVE))  
    # this will turn into build army and maybe even manage larva
    #@todo create drones from here as well --> do all larva management from here
    
    async def enemyGoingAir(self): # switch production over to straight hydras :)
        if not self.enemyIsGoingForAir:
            if self.known_enemy_units(VOIDRAY).amount > 1:
                self.enemyIsGoingForAir = True
                await self.chat_send("VOID RAYS!")
                print ("enemy going for voids: MAKE HYDRAS!!!!")
    
                 
    async def manageLarva(self, hatches):
        if not self.supply_used == 200 and await self.doImportantStuffFirst(hatches):
            for larva in self.units(LARVA).ready:
                if(len(self.units(SPAWNINGPOOL).ready)) >= 1 and self.supply_left > 0 and self.can_afford(ZERGLING) and (self.units(ZERGLING).amount * 4 < (self.units(ROACH).amount + self.units(HYDRALISK).amount) and self.vespene < 200 or (self.units(ROACHWARREN).ready.amount < 1 and self.units(HYDRALISKDEN).ready.amount < 1)):
                    await self.do(larva.build(ZERGLING))
                elif self.units(ROACHWARREN).ready.amount >= 1 and self.supply_left > 1 and self.can_afford(ROACH) and (self.units(ROACH).amount < self.units(HYDRALISK).amount or self.units(HYDRALISKDEN).ready.amount < 1) and not self.enemyIsGoingForAir and not self.units(GREATERSPIRE).ready.amount > 0:
                    await self.do(larva.build(ROACH))
                elif (self.units(HYDRALISKDEN).ready.amount >= 1 and self.supply_left > 1 and self.can_afford(HYDRALISK) and self.units(GREATERSPIRE).ready.amount < 1) or (self.units(GREATERSPIRE).ready.amount > 0 and self.units(HYDRALISKDEN).ready.amount >= 1 and self.supply_left > 30 and self.can_afford(HYDRALISK)) or (self.enemyIsGoingForAir and self.units(HYDRALISKDEN).ready.amount > 0 and self.supply_left > 1 and self.can_afford(HYDRALISK)):
                    await self.do(larva.build(HYDRALISK))
                elif (self.units(SPIRE).ready.amount >= 1 or self.units(GREATERSPIRE).ready.amount >=1) and self.supply_left > 1 and self.can_afford(CORRUPTOR):
                    await self.do(larva.build(CORRUPTOR))
    
    async def hasLair(self):
        if(self.units(LAIR).amount > 0):
            return True
        morphingYet = False
        for h in self.units(HATCHERY):
            if CANCEL_MORPHLAIR in await self.get_available_abilities(h):
                morphingYet = True
                break
        if morphingYet:
            return True   
        return False
           
    async def doImportantStuffFirst(self, hatches): #pause spending on army if returns false # changed from def to async def to handle checking for lair being built --> not sure if correct
        if self.needArmyNow(hatches):
            return True 
        elif self.gameTime > self.SECONDS_PER_MINUTE * 4 and not await self.hasLair() and self.units(HIVE).amount < 1  and self.units(HATCHERY).amount > 1: #make sure gets lair
            return False
        elif self.gameTime > (self.SECONDS_PER_MINUTE * 3) and hatches.amount < 3 and not self.gameTime > self.SECONDS_PER_MINUTE * 4: #make sure gets 3 expo correctly
            return False
        elif self.units(LAIR).ready.amount > 0 and self.units(OVERSEER).amount < 2: # make sure 2 overseer with army at all times
            return False
        elif self.units(LAIR).ready.amount > 0 and self.units(HYDRALISKDEN).amount  < 1 and self.gameTime > self.SECONDS_PER_MINUTE * 6: #makes sure gets hydra den
            return False
        elif self.units(HYDRALISKDEN).ready.amount > 0 and self.units(INFESTATIONPIT).amount < 1 and self.gameTime > self.SECONDS_PER_MINUTE * 8: #make sure gets infestation pit
            return False
        elif self.units(INFESTATIONPIT).ready.amount > 0 and self.units(HIVE).amount < 1 and self.units(LAIR).ready.noqueue.amount == self.units(LAIR).ready.amount and self.gameTime > self.SECONDS_PER_MINUTE * 9: #makes sure gets hive
            return False
        elif self.units(GREATERSPIRE).ready.amount > 0 and self.units(CORRUPTOR).ready.amount > 0: #make broodlords
            return False
        return True
    
    # @TODO -- throws assert self.can_afford(building) occasionally for some reason
    async def expandWell(self): # make expand slower in late game
        if (self.can_afford(HATCHERY) and len(self.units(DRONE)) > (len(self.units(HATCHERY)) + self.already_pending(HATCHERY)) * 18) or (self.can_afford(HATCHERY) and not self.already_pending(HATCHERY) > 1):
            await self.expand_now()
    #take gases slower --> usually takes 3 by 3 min --> make expand faster ?        
    async def buildgas(self, hatches):
        for hatch in hatches:
            vespene = self.state.vespene_geyser.closer_than(10, hatch)
            for v in vespene:
                if (self.can_afford(EXTRACTOR) and (len(self.units(DRONE)) > (len(self.units(EXTRACTOR)) * 16) + 20 or (len(self.units(DRONE)) > 16 and self.units(EXTRACTOR).amount < 1)) and not self.already_pending(EXTRACTOR)) or (self.can_afford(EXTRACTOR) and self.gameTime > self.SECONDS_PER_MINUTE * 9):
                    worker = self.select_build_worker(v.position)
                    if not self.units(EXTRACTOR).closer_than(1, v).exists and not self.already_pending(EXTRACTOR) and not worker is None:
                        await self.do(worker.build(EXTRACTOR, v))
                        break
    
    async def buildqueens(self, hatches): #currently only builds 1 queen per hatch
        for hatch in hatches.ready.noqueue:
            if self.can_afford(QUEEN) and self.supply_left > 1 and len(self.units(QUEEN)) < (hatches.amount) and len(self.units(SPAWNINGPOOL).ready) > 0 and hatch.noqueue and self.units(QUEEN).amount < 5:
                await self.do(hatch.train(QUEEN))
                break
            
    async def larvainject(self, hatches):
        toBreak = False
        for hatch in hatches(HATCHERY).ready | hatches(LAIR) | hatches(HIVE):
            for queen in self.units(QUEEN).ready.prefer_close_to(hatch.position):
                if not hatch.has_buff(QUEENSPAWNLARVATIMER) and queen.energy >= 25: #QUEENSPAWNLARVATIMER
                    await self.do(queen(EFFECT_INJECTLARVA, target=hatch))
                    break
            if(toBreak):
                break
            #find nearest hatch that isnt waiting for inject to pop
            
    async def getUpgrades(self, hatches):
        if self.units(SPAWNINGPOOL).ready.noqueue.amount > 0:
            ups = await self.get_available_abilities(self.units(SPAWNINGPOOL).ready.noqueue[0])
            for up in ups:
                if self.can_afford(up):
                    await self.do(self.units(SPAWNINGPOOL).ready.noqueue[0](up))
                    break
        if self.units(ROACHWARREN).ready.amount > 0 and self.units(LAIR).ready.amount > 0 and self.can_afford(GLIALRECONSTITUTION):
            if RESEARCH_GLIALREGENERATION in await self.get_available_abilities(self.units(ROACHWARREN)[0]):
                await self.do(self.units(ROACHWARREN)[0](RESEARCH_GLIALREGENERATION))
            elif RESEARCH_TUNNELINGCLAWS in await self.get_available_abilities(self.units(ROACHWARREN)[0]) and self.units(ROACHWARREN)[0].noqueue:
                await self.do(self.units(ROACHWARREN)[0](RESEARCH_TUNNELINGCLAWS))
        if hatches.ready.amount > 0 and self.can_afford(BURROW):
            if RESEARCH_BURROW in await self.get_available_abilities(hatches[0]):
                await self.do(hatches[0](RESEARCH_BURROW))
        if self.units(EVOLUTIONCHAMBER).ready.noqueue.amount > 0 and (self.units(LAIR).amount > 0 or self.units(HIVE).amount > 0):
            for evo in self.units(EVOLUTIONCHAMBER).ready.noqueue:
                possibleUpgrades = await self.get_available_abilities(evo)
                for ups in possibleUpgrades:
                    if self.can_afford(ups):
                        await self.do(evo(ups))
                        break
        if self.units(HYDRALISKDEN).ready.noqueue.amount > 0:
            possibleUpgrades = await self.get_available_abilities(self.units(HYDRALISKDEN).ready.noqueue[0])
            for ups in possibleUpgrades:
                if self.can_afford(ups):
                    await self.do(self.units(HYDRALISKDEN).ready.noqueue[0](ups))
                    break
                        
    def timeToAttack(self, hatches):
        ownedUnits = self.units
        if (ownedUnits(BROODLORD).ready.amount > 4 and self.supply_used > 195): # has broodlords and is ready to kill
            return True
        if self.enemyIsGoingForAir and self.supply_used > 185 and self.units(HYDRALISK).ready.amount > 30:
            return True
        if self.gameTime > self.SECONDS_PER_MINUTE * 30:
            return True
        if self.supply_used > 185 and self.minerals > 1000 and self.vespene > 650: #free up some army supply / end game if enemy is weak
            return True
        return False
        
    def timeToRetreate(self, hatches):
        return True
    
    #attack a position = amove, attack unit = target?
    #def moveCommandTowards(self, controlUnit, endPosition): #should return the position to click on the ground --> used to micro ranged units 
        
    
    async def controlArmy(self, hatches): # need to write defense for target air units
        
        allArmyUnits = self.units(ROACH).ready | self.units(HYDRALISK).ready | self.units(ZERGLING).ready | self.units(CORRUPTOR).ready | self.units(BROODLORD).ready
        targetGround = self.units(ZERGLING).ready | self.units(ROACH).ready | self.units(BROODLORD).ready # cant shoot up --> make for 4 catagories of units --> ground att only, air att only, both, spellcaster(no attack)
        targetGroundAndAir = self.units(HYDRALISK).ready
        targetAir = self.units(CORRUPTOR).ready
        attacking = self.timeToAttack(hatches)
        
        groundTargets = self.known_enemy_units.filter(lambda unit: not unit.is_flying).not_structure | self.known_enemy_structures(PHOTONCANNON) | self.known_enemy_structures(BUNKER) | self.known_enemy_structures(SHIELDBATTERY) | self.known_enemy_structures(MISSILETURRET) | self.known_enemy_structures(SPINECRAWLER) | self.known_enemy_units(SPORECRAWLER) | self.known_enemy_units(PLANETARYFORTRESS) | self.known_enemy_units(PYLON)
        airTargets = self.known_enemy_units.filter(lambda unit: unit.is_flying).not_structure
        buildings = self.known_enemy_structures
        groundAndAirTargets = groundTargets | airTargets
        
        if attacking: # and self.iteration % 6 == 0: # attack
            for s in targetGround:
                await self.do(s.attack(self.find_ground_target(s, groundTargets, buildings)))
            for s in targetGroundAndAir:
                await self.do(s.attack(self.find_any_target(s, groundAndAirTargets, buildings)))
            for s in targetAir:
                await self.do(s.attack(self.find_air_target(s, airTargets, buildings)))
                
        else: # self.iteration % 6 == 0: # defend
            if len(groundTargets) > 0:
                for s in targetGround:
                    closeTo = groundTargets.prefer_close_to(s.position)[0]
                    if closeTo.distance_to(s.position) < 10:
                            await self.do(s.attack(closeTo))
                    else:
                        for h in hatches:
                            if groundTargets.prefer_close_to(h.position)[0].distance_to(h.position) < 20:
                                await self.do(s.attack(closeTo))
                                
            if len(groundAndAirTargets) > 0:
                for s in targetGroundAndAir:
                    closeTo = groundAndAirTargets.prefer_close_to(s.position)[0]
                    if closeTo.distance_to(s.position) < 10:
                            await self.do(s.attack(closeTo))
                    else:
                        for h in hatches:
                            if groundAndAirTargets.prefer_close_to(h.position)[0].distance_to(h.position) < 20:
                                await self.do(s.attack(closeTo))
            
            if len(airTargets) > 0:
                for s in targetAir:
                    closeTo = airTargets.prefer_close_to(s.position)[0]
                    if closeTo.distance_to(s.position) < 10:
                            await self.do(s.attack(closeTo))
                    else:
                        for h in hatches:
                            if self.known_enemy_units.prefer_close_to(h.position)[0].distance_to(h.position) < 20:
                                await self.do(s.attack(closeTo))
            #moves army units not doing anything to the "forward most" hatch  closest to enemy (positioning code if no enemy to fight)
            for s in allArmyUnits.idle:
                closestBaseToMe = hatches.closest_to(s.position)
                await self.do(s.move(closestBaseToMe))
            
                            
        if self.units(QUEEN).amount > 0 and len(self.known_enemy_units) > 0:
            for s in hatches:
                if self.known_enemy_units.prefer_close_to(s.position)[0].distance_to(s.position) < 15:
                    for q in self.units(QUEEN):
                        await self.do(q.attack(self.known_enemy_units.prefer_close_to(q.position)[0]))
                        
        if self.units(OVERSEER).amount > 0 and allArmyUnits.amount > 0: # keeps overseer with AllArmyUnits
            for o in self.units(OVERSEER).ready.idle:
                moveHere = allArmyUnits.closest_to(o.position)
                if not moveHere.position == o.position:
                    await self.do(o.move(moveHere))
                                  
    async def transfuse(self):
        queens = self.units(QUEEN)
        for queen in queens:
            if TRANSFUSION_TRANSFUSION in await self.get_available_abilities(queen):
                for unit in self.units.not_structure.owned:
                    if not unit.is_structure and unit.health_max - 125 >= unit.health and unit.distance_to(queen.position) < 15 and not unit.tag == queen.tag: #range of transfuse is 7
                        await self.do(queen(TRANSFUSION_TRANSFUSION, unit))
            
    async def roachMicro(self):
        roaches = self.units(ROACH)
        roachburrowed = self.units(ROACHBURROWED)
        for roach in roaches:
            if BURROWDOWN_ROACH in await self.get_available_abilities(roach) and roach.health < 60 and self.can_afford(BURROWDOWN_ROACH):
                await self.do(roach(BURROWDOWN_ROACH))
        for roach in roachburrowed:
            if BURROWUP_ROACH in await self.get_available_abilities(roach) and roach.health > 120 and self.can_afford(BURROWUP_ROACH):
                    await self.do(roach(BURROWUP_ROACH))
    
    #TODO make units target fire / finish hurt units
    def find_ground_target(self, unit, enemyUnits, enemyBuildings):
        if len(enemyUnits) > 0:
            return enemyUnits.prefer_close_to(unit.position)[0]
        elif len(enemyBuildings) > 0:
            return enemyBuildings.prefer_close_to(unit.position)[0]
        else:
            if(len(self.enemy_start_locations) > 1):
                return random.sample(self.enemy_start_locations, 1)[0]
            return self.enemy_start_locations[0]
        
    def find_any_target(self, unit, enemyUnits, enemyBuildings):
        if len(enemyUnits) > 0:
            return enemyUnits.prefer_close_to(unit.position)[0]
        elif len(enemyBuildings) > 0:
            return enemyBuildings.prefer_close_to(unit.position)[0]
        else:
            if(len(self.enemy_start_locations) > 1):
                return random.sample(self.enemy_start_locations, 1)[0]
            return self.enemy_start_locations[0]
        
    def find_air_target(self, unit, enemyUnits, enemyBuildings):
        if len(enemyUnits) > 0:
            return enemyUnits.prefer_close_to(unit.position)[0]
        elif len(enemyBuildings) > 0:
            return enemyBuildings.prefer_close_to(unit.position)[0]
        else:
            if(len(self.enemy_start_locations) > 1):
                return random.sample(self.enemy_start_locations, 1)[0]
            return self.enemy_start_locations[0]
        
    async def morphOverseer(self):
        if self.units(OVERSEER).amount < 2 and self.can_afford(OVERSEER) and self.units(OVERLORD).amount >= 2 and (self.units(LAIR).ready.amount > 0 or self.units(HIVE).amount > 0) and self.units(OVERLORDCOCOON).amount < 2:
            await self.do(self.units(OVERLORD).ready.noqueue[0](MORPH_OVERSEER))
           
    async def morphBroodlord(self):
        if self.can_afford(BROODLORD) and self.units(GREATERSPIRE).ready.amount > 0 and self.supply_left > 1:
            for c in self.units(CORRUPTOR).ready:
                if self.can_afford(BROODLORD):
                    await self.do(c(MORPHTOBROODLORD_BROODLORD))
    
        
#run_game(maps.get("CatalystLE"), [Bot(Race.Zerg, SnowBot()), Computer(Race.Terran, Difficulty.Harder)], realtime = False)