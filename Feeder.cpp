#include <iostream>
#include <cstdlib>

#include "Feeder.h"
#include "sc2api/sc2_api.h"
#include "sc2api/sc2_types.h"

using namespace sc2;

Feeder::Feeder() {
	PrintStatus("Feeder has been initialized..");

	enemy_base_point.x = 0.0f;
	enemy_base_point.y = 0.0f;

	scv_scouting = false;
}

//void Feeder::OnGameStart(){}
// this is comment
void Feeder::OnStep() {
	//Observate();
	TryBuildSCV();
	BuildStructures();
	BuildArmy();
	ManageArmy();
	if (enemy_base_point.x == 0.0f && enemy_base_point.y == 0.0f) { // not found enemy base
		//ScoutWithSCV();
	}
	else { // found enemy base
		ManageArmy();
		//AttackWithAllUnits();
	}
	
}

void Feeder::OnUnitDestroyed(const sc2::Unit *unit) {
	if (unit->alliance == Unit::Alliance::Self) {
	}
}
void Feeder::OnUnitEnterVision(const sc2::Unit *unit) {
	if (scv_scouting) {
		switch (unit->unit_type.ToType())
		{
		case UNIT_TYPEID::TERRAN_SCV: {
			break;
		}
		case UNIT_TYPEID::PROTOSS_PROBE: {
			break;
		}
		case UNIT_TYPEID::ZERG_DRONE: {
			break;
		}
		default:
			GetNeareastBaseLocation(Point2D(unit->pos.x, unit->pos.y));
			scv_scouting = false;
			break;
		}
	}
}

void Feeder::OnBuildingConstructionComplete(const sc2::Unit* unit) {
	switch (unit->unit_type.ToType()) {
	case UNIT_TYPEID::TERRAN_BARRACKS: {
		// set rally point
		break;
	}
	default:
		break;
	}
}

void Feeder::OnUnitCreated(const sc2::Unit *unit) {
	
}

void Feeder::OnUnitIdle(const sc2::Unit *unit) {
	switch (unit->unit_type.ToType()) {
	case UNIT_TYPEID::TERRAN_SCV: {
		MineIdleWorkers(unit, ABILITY_ID::HARVEST_GATHER, UNIT_TYPEID::TERRAN_REFINERY);
		break;
	}
	default:
		break;
	}
}

void Feeder::Observate() {  // not recommended to use
	clearObservation();
	const ObservationInterface* observation = Observation();
	Units units = observation->GetUnits(Unit::Alliance::Self);

	depot_num = observation->GetUnits(Unit::Alliance::Self, IsUnits(supply_depot_types)).size();
	barracks_num = observation->GetUnits(Unit::Alliance::Self, IsUnits(barrack_types)).size();
	factory_num = observation->GetUnits(Unit::Alliance::Self, IsUnits(factory_types)).size();

	refinery_num = CountUnitType(observation, UNIT_TYPEID::TERRAN_REFINERY);
	

	for (const auto& unit : units) {
		switch (unit->unit_type.ToType()) {
		case UNIT_TYPEID::TERRAN_SCV:
			scv_num++;
			break;
		case UNIT_TYPEID::TERRAN_MARINE:
			marine_num++;
			break;
		case UNIT_TYPEID::TERRAN_MARAUDER:
			marauder_num++;
			break;
		}
	}

}

void Feeder::BuildStructures() {
	TryBuildSupplyDepot();
	TryBuildTrainingFacilities();
	BuildRefinery();
	TryBuildResearch();
	ManageUpgrades();
}

void Feeder::BuildArmy() {
	const ObservationInterface* observation = Observation();
	Units bases = observation->GetUnits(Unit::Self, IsTownHall());
	Units barracks = observation->GetUnits(Unit::Self, IsUnits(barrack_types));
	Units factorys = observation->GetUnits(Unit::Self, IsUnits(factory_types));
	Units starports = observation->GetUnits(Unit::Self, IsUnits(starport_types));
	Units barracks_tech = observation->GetUnits(Unit::Self, IsUnit(UNIT_TYPEID::TERRAN_BARRACKSTECHLAB));
	Units factorys_tech = observation->GetUnits(Unit::Self, IsUnit(UNIT_TYPEID::TERRAN_FACTORYTECHLAB));
	Units starports_tech = observation->GetUnits(Unit::Self, IsUnit(UNIT_TYPEID::TERRAN_STARPORTTECHLAB));

	Units supply_depots = observation->GetUnits(Unit::Self, IsUnit(UNIT_TYPEID::TERRAN_SUPPLYDEPOT));
	if (bases.size() < 2 && observation->GetMinerals() > 1000) {
		TryBuildExpansionCom();
		return;
	}

	for (const auto& supply_depot : supply_depots) {
		Actions()->UnitCommand(supply_depot, ABILITY_ID::MORPH_SUPPLYDEPOT_LOWER);
	}

	if (!barracks.empty()) {
		for (const auto& base : bases) {
			if (base->unit_type == UNIT_TYPEID::TERRAN_COMMANDCENTER && observation->GetMinerals() > 150) {
				Actions()->UnitCommand(base, ABILITY_ID::MORPH_ORBITALCOMMAND);
			}
		}
	}

	for (const auto& barrack : barracks) {
		if (!barrack->orders.empty() || barrack->build_progress != 1) {
			continue;
		}
		if (observation->GetUnit(barrack->add_on_tag) == nullptr) {
			if (barracks_tech.size() < barracks.size() / 2 || barracks_tech.empty()) {
				TryBuildAddOn(ABILITY_ID::BUILD_TECHLAB_BARRACKS, barrack->tag);
			}
			else {
				TryBuildAddOn(ABILITY_ID::BUILD_REACTOR_BARRACKS, barrack->tag);
			}
		}
		TryBuildMarauder();
		TryBuildMarine();
		
	}

	for (const auto& starport : starports) {
		TrybuildMedivac();
	}

	for (const auto& factory : factorys) {
		if (!factory->orders.empty()) {
			continue;
		}

		if (observation->GetUnit(factory->add_on_tag) == nullptr) {
			if (false) {
				if (factorys_tech.size() < factorys.size() / 2) {
					TryBuildAddOn(ABILITY_ID::BUILD_TECHLAB_FACTORY, factory->tag);
				}
				else {
					TryBuildAddOn(ABILITY_ID::BUILD_REACTOR_FACTORY, factory->tag);
				}
			}
			else {
				if (CountUnitType(observation, UNIT_TYPEID::TERRAN_BARRACKSREACTOR) < 1) {
					TryBuildAddOn(ABILITY_ID::BUILD_REACTOR_FACTORY, factory->tag);
				}
				else {
					TryBuildAddOn(ABILITY_ID::BUILD_TECHLAB_FACTORY, factory->tag);
				}
			}

		}
	}

	for (const auto& starport : starports) {
		if (!starport->orders.empty()) {
			continue;
		}
		if (observation->GetUnit(starport->add_on_tag) == nullptr) {
			if (false) {
				if (CountUnitType(observation, UNIT_TYPEID::TERRAN_STARPORTREACTOR) < 2) {
					TryBuildAddOn(ABILITY_ID::BUILD_REACTOR_STARPORT, starport->tag);
				}
				else {
					TryBuildAddOn(ABILITY_ID::BUILD_TECHLAB_STARPORT, starport->tag);
				}
			}
			else {
				if (CountUnitType(observation, UNIT_TYPEID::TERRAN_STARPORTREACTOR) < 1) {
					TryBuildAddOn(ABILITY_ID::BUILD_REACTOR_STARPORT, starport->tag);
				}
				else {
					TryBuildAddOn(ABILITY_ID::BUILD_TECHLAB_STARPORT, starport->tag);
				}

			}
		}
	}

	size_t barracks_count_target = std::min<size_t>(3 * bases.size(), 8);
	size_t factory_count_target = 1;
	size_t starport_count_target = 2;
	size_t armory_count_target = 1;
	/*
	if (false) {
		barracks_count_target = 1;
		armory_count_target = 2;
		factory_count_target = std::min<size_t>(2 * bases.size(), 7);
		starport_count_target = std::min<size_t>(1 * bases.size(), 4);
	}

	if (!factorys.empty() && starports.size() < std::min<size_t>(1 * bases.size(), 4)) {
		if (observation->GetMinerals() > 150 && observation->GetVespene() > 100) {
			TryBuildStructure(ABILITY_ID::BUILD_STARPORT);
		}
	}

	if (!barracks.empty() && factorys.size() < std::min<size_t>(2 * bases.size(), 7)) {
		if (observation->GetMinerals() > 150 && observation->GetVespene() > 100) {
			TryBuildStructureRandom(ABILITY_ID::BUILD_FACTORY, UNIT_TYPEID::TERRAN_SCV);
		}
	}

	if (barracks.size() < barracks_count_target) {
		if (observation->GetFoodWorkers() >= target_worker_count_) {
			TryBuildStructureRandom(ABILITY_ID::BUILD_BARRACKS, UNIT_TYPEID::TERRAN_SCV);
		}
	}

	if (!mech_build_) {
		if (CountUnitType(observation, UNIT_TYPEID::TERRAN_ENGINEERINGBAY) < std::min<size_t>(bases.size(), 2)) {
			if (observation->GetMinerals() > 150 && observation->GetVespene() > 100) {
				TryBuildStructureRandom(ABILITY_ID::BUILD_ENGINEERINGBAY, UNIT_TYPEID::TERRAN_SCV);
			}
		}
		if (!barracks.empty() && CountUnitType(observation, UNIT_TYPEID::TERRAN_GHOSTACADEMY) < 1) {
			if (observation->GetMinerals() > 150 && observation->GetVespene() > 50) {
				TryBuildStructureRandom(ABILITY_ID::BUILD_GHOSTACADEMY, UNIT_TYPEID::TERRAN_SCV);
			}
		}
		if (!factorys.empty() && CountUnitType(observation, UNIT_TYPEID::TERRAN_FUSIONCORE) < 1) {
			if (observation->GetMinerals() > 150 && observation->GetVespene() > 150) {
				TryBuildStructureRandom(ABILITY_ID::BUILD_FUSIONCORE, UNIT_TYPEID::TERRAN_SCV);
			}
		}
	}

	if (!barracks.empty() && CountUnitType(observation, UNIT_TYPEID::TERRAN_ARMORY) < armory_count_target) {
		if (observation->GetMinerals() > 150 && observation->GetVespene() > 100) {
			TryBuildStructure(ABILITY_ID::BUILD_ARMORY,1, UNIT_TYPEID::TERRAN_SCV);
		}
	}
	*/
}

bool Feeder::TryBuildSupplyDepot() {
	const ObservationInterface* observation = Observation();

	// If we are not supply capped, don't build a supply depot.
	if (observation->GetFoodUsed() < observation->GetFoodCap() - 3) {
		return false;
	}

	//if (observation->GetMinerals() < 100) {
	//	return false;
	//}

	//check to see if there is already on building
	Units units = observation->GetUnits(Unit::Alliance::Self, IsUnits(supply_depot_types));
	if (observation->GetFoodUsed() < 40) {
		for (const auto& unit : units) {
			if (unit->build_progress != 1) {
				return false;
			}
		}
	}

	// Try and build a supply depot. Find a random SCV and give it the order.
	float rx = GetRandomScalar();
	float ry = GetRandomScalar();
	Point2D build_location = Point2D(staging_location_.x + rx * 15, staging_location_.y + ry * 15);
	return TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT, 1, UNIT_TYPEID::TERRAN_SCV);
}

bool Feeder::TryBuildTrainingFacilities() 
{
	const ObservationInterface* observation = Observation();

	depot_num = observation->GetUnits(Unit::Alliance::Self, IsUnits(supply_depot_types)).size();
	barracks_num = observation->GetUnits(Unit::Alliance::Self, IsUnits(barrack_types)).size();
	factory_num = observation->GetUnits(Unit::Alliance::Self, IsUnits(factory_types)).size();

	if (barracks_num < 4) {
		TryBuildStructure(ABILITY_ID::BUILD_BARRACKS, 1, UNIT_TYPEID::TERRAN_SCV);
	}
	else if (CountUnitType(observation, UNIT_TYPEID::TERRAN_REFINERY) > 0 &&
			observation->GetUnits(Unit::Alliance::Self, IsUnits(barrack_types)).size() > 2 &&
		observation->GetUnits(Unit::Alliance::Self, IsUnits(factory_types)).size() < 1) {
		TryBuildStructure(ABILITY_ID::BUILD_FACTORY, 1, UNIT_TYPEID::TERRAN_SCV);
	}
	else if (observation->GetUnits(Unit::Alliance::Self, IsUnits(barrack_types)).size() > 2 &&
			observation->GetUnits(Unit::Alliance::Self, IsUnits(starport_types)).size() < 2) {
		TryBuildStructure(ABILITY_ID::BUILD_STARPORT, 1, UNIT_TYPEID::TERRAN_SCV);
	}

	return true;
}

bool Feeder::TryBuildAddOn(AbilityID ability_type_for_structure, Tag base_structure) {
	float rx = GetRandomScalar();
	float ry = GetRandomScalar();
	const Unit* unit = Observation()->GetUnit(base_structure);

	if (unit->build_progress != 1) {
		return false;
	}

	Point2D build_location = Point2D(unit->pos.x + rx * 15, unit->pos.y + ry * 15);

	Units units = Observation()->GetUnits(Unit::Self, IsStructure(Observation()));

	if (Query()->Placement(ability_type_for_structure, unit->pos, unit)) {
		Actions()->UnitCommand(unit, ability_type_for_structure);
		return true;
	}

	float distance = std::numeric_limits<float>::max();
	for (const auto& u : units) {
		float d = Distance2D(u->pos, build_location);
		if (d < distance) {
			distance = d;
		}
	}
	if (distance < 6) {
		return false;
	}

	if (Query()->Placement(ability_type_for_structure, build_location, unit)) {
		Actions()->UnitCommand(unit, ability_type_for_structure, build_location);
		return true;
	}
	return false;

}

bool Feeder::BuildRefinery() {
	const ObservationInterface* observation = Observation();
	Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());

	if (CountUnitType(observation, UNIT_TYPEID::TERRAN_REFINERY) >= observation->GetUnits(Unit::Alliance::Self, IsTownHall()).size() * 2) {
		return false;
	}

	for (const auto& base : bases) {
		if (base->assigned_harvesters >= base->ideal_harvesters) {
			if (base->build_progress == 1) {
				if (TryBuildGas(ABILITY_ID::BUILD_REFINERY, UNIT_TYPEID::TERRAN_SCV, base->pos)) {
					return true;
				}
			}
		}
	}
	return false;
}

bool Feeder::TryBuildSCV() {
	const ObservationInterface* observation = Observation();
	Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());

	for (const auto& base : bases) {
		if (base->unit_type == UNIT_TYPEID::TERRAN_ORBITALCOMMAND && base->energy > 50) {
			if (FindNearestMineralPatch(base->pos)) {
				Actions()->UnitCommand(base, ABILITY_ID::EFFECT_CALLDOWNMULE, FindNearestMineralPatch(base->pos));
			}
		}
	}

	if (observation->GetFoodWorkers() >= max_worker_count_) {
		return false;
	}

	if (observation->GetFoodUsed() >= observation->GetFoodCap()) {
		return false;
	}

	if (observation->GetFoodWorkers() > GetExpectedWorkers(UNIT_TYPEID::TERRAN_REFINERY)) {
		return false;
	}

	//if (CountUnitType(Observation(), UNIT_TYPEID::TERRAN_SCV) >= 20)
	//	return false;

	for (const auto& base : bases) {
		//if there is a base with less than ideal workers
		if (base->assigned_harvesters < base->ideal_harvesters && base->build_progress == 1) {
			if (observation->GetMinerals() >= 50) {
				return TryBuildUnit(ABILITY_ID::TRAIN_SCV, base->unit_type);
			}
		}
	}
	//return TryBuildUnit(ABILITY_ID::TRAIN_SCV, UNIT_TYPEID::TERRAN_COMMANDCENTER);
	return false;
}

bool Feeder::TryBuildMarine() {
	return TryBuildUnit(ABILITY_ID::TRAIN_MARINE, UNIT_TYPEID::TERRAN_BARRACKS);
}

bool Feeder::TryBuildMarauder() {
	return TryBuildUnit(ABILITY_ID::TRAIN_MARAUDER, UNIT_TYPEID::TERRAN_BARRACKS);
}

bool Feeder::TrybuildMedivac() {
	return TryBuildUnit(ABILITY_ID::TRAIN_MEDIVAC, UNIT_TYPEID::TERRAN_STARPORT);
}

// TODO KNOWN BUG: in some cases, scv would not be sent to scout, 
// probable cause: conflit between Scouting method and (BuildStructure || onIdle)
void Feeder::ScoutWithSCV() {
	const ObservationInterface* observation = Observation();

	// get all SCV
	Units SCVS = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_SCV));


	if (!scv_scouting) {
		scv_scouting = true;
	}
	else {
		return;
	}
	
	//if we have no workers 
	if (SCVS.empty()) {
		return;
	}

	// get one scv to scout
 	for (auto unit : SCVS){

		Point2D target_pos;
		std::vector<Point2D> start_loactions;

		GetAllEnemyBaseLocation(start_loactions);

		// for each possible enemy base location
		for (Point2D &point : start_loactions) {
			Actions()->UnitCommand(unit, ABILITY_ID::MOVE, point, true); // queue orders
		}

		break;
	}

	scv_scouting = true;

}

void Feeder::GetAllEnemyBaseLocation(std::vector<Point2D> &rtv) {
	const ObservationInterface* observation = Observation();

	Point2D my_start_pos, min_p, max_p;
	Point3D temp_pos = observation->GetStartLocation();
	GameInfo game_info;

	my_start_pos.x = temp_pos.x;
	my_start_pos.y = temp_pos.y;
	game_info = observation->GetGameInfo();
	min_p = game_info.playable_min;
	max_p = game_info.playable_max;

	rtv.push_back(Point2D(my_start_pos.x, max_p.y - my_start_pos.y + min_p.y));
	rtv.push_back(Point2D(max_p.x - my_start_pos.x + min_p.x, max_p.y - my_start_pos.y + min_p.y));
	rtv.push_back(Point2D(max_p.x - my_start_pos.x + min_p.x, my_start_pos.y));
	rtv.push_back(Point2D(my_start_pos.x, my_start_pos.y));

}

bool Feeder::TryBuildExpansionCom() {
	const ObservationInterface* observation = Observation();
	Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
	//Don't have more active bases than we can provide workers for
	if (GetExpectedWorkers(UNIT_TYPEID::TERRAN_REFINERY) > max_worker_count_) {
		return false;
	}
	// If we have extra workers around, try and build another Hatch.
	if (GetExpectedWorkers(UNIT_TYPEID::TERRAN_REFINERY) < observation->GetFoodWorkers() - 10) {
		return TryExpand(ABILITY_ID::BUILD_COMMANDCENTER, UNIT_TYPEID::TERRAN_SCV);
	}
	//Only build another Hatch if we are floating extra minerals
	if (observation->GetMinerals() > std::min<size_t>(bases.size() * 400, 1200)) {
		return TryExpand(ABILITY_ID::BUILD_COMMANDCENTER, UNIT_TYPEID::TERRAN_SCV);
	}
	return false;
}

// walk
void Feeder::ScoutWithMarines() {
	const ObservationInterface* observation = Observation();
	Units units = Observation()->GetUnits(Unit::Alliance::Self, IsMyArmy());
	Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy, IsAttackable());
	// count the number of Marines
	size_t num_Marines = 0;
	for (const auto& unit : units) {
		UnitTypeID unit_type(unit->unit_type);
		if (unit_type == UNIT_TYPEID::TERRAN_MARINE)
			++num_Marines;
	}
	if (num_Marines < 20) { return; }
	for (const auto& unit : units) {
		UnitTypeID unit_type(unit->unit_type);

		if (!unit->orders.empty())
			continue;

		// Priority to attacking enemy structures.
		/*
		Point2D target_pos;
		float neareast_possible_enemy_base = -1.0f;
		if (FindEnemyPosition(target_pos)) {
			// update enemy_base_location
			if (game_info_.enemy_start_locations.empty()) {
				return;
			}

			for (Point2D &start_point : game_info_.enemy_start_locations) {
				float temp = Distance2D(target_pos, start_point);

				if ((temp < neareast_possible_enemy_base) || (neareast_possible_enemy_base < 0)) {
					neareast_possible_enemy_base = temp;
					enemy_base_point = start_point;
				}
			}

		}
		*/
		
		
		Point2D target_pos;
		if (FindEnemyPosition(target_pos)) {
			if (Distance2D(unit->pos, target_pos) < 20 && enemy_units.empty()) {
				if (TryFindRandomPathableLocation(unit, target_pos)) {
					Actions()->UnitCommand(unit, ABILITY_ID::SMART, target_pos);
					return;
				}
			}
			else if (!enemy_units.empty())
			{
				Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_units.front());
				return;
			}
			Actions()->UnitCommand(unit, ABILITY_ID::SCAN_MOVE, target_pos);
		}
		
	}
}

// pre-conditon: enemy-location is found
void Feeder::AttackWithAllUnits() {
	const ObservationInterface* observation = Observation();
	Units units = Observation()->GetUnits(Unit::Alliance::Self);

	Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
	if (enemy_units.empty()) {
		return;
	}

	Actions()->UnitCommand(units, ABILITY_ID::ATTACK, enemy_base_point);
	/*
	if (game_info_.enemy_start_locations.empty()) {
		return false;
	}
	target_pos = game_info_.enemy_start_locations.front();
	return true;
	*/
}

void Feeder::GetNeareastBaseLocation(Point2D &point) {
	std::vector<Point2D> locations;
	float min_distance;
	Point2D min_point;

	GetAllEnemyBaseLocation(locations);
	min_point = locations.front();
	min_distance = Distance2D(point, min_point);
	for (const Point2D &loca : locations) {
		float temp = Distance2D(loca, loca);

		if (temp < min_distance) {
			min_distance = temp;
			min_point = loca;
		}
	}

	enemy_base_point = min_point;
}

bool Feeder::TryBuildResearch()
{
	const ObservationInterface* observation = Observation();

	barracks_num = observation->GetUnits(Unit::Alliance::Self, IsUnits(barrack_types)).size();
	factory_num = observation->GetUnits(Unit::Alliance::Self, IsUnits(factory_types)).size();

	if (barracks_num && !CountUnitType(observation, UNIT_TYPEID::TERRAN_ENGINEERINGBAY)) {
		TryBuildStructure(ABILITY_ID::BUILD_ENGINEERINGBAY, 1, UNIT_TYPEID::TERRAN_SCV);
	}
	else if (CountUnitType(observation, UNIT_TYPEID::TERRAN_REFINERY) > 0 &&
		observation->GetUnits(Unit::Alliance::Self, IsUnits(factory_types)).size() > 0 &&
		!CountUnitType(observation, UNIT_TYPEID::TERRAN_ARMORY)) {
		TryBuildStructure(ABILITY_ID::BUILD_ARMORY, 1, UNIT_TYPEID::TERRAN_SCV);
	}

	return true;
}

void Feeder::ManageUpgrades() {
	const ObservationInterface* observation = Observation();
	auto upgrades = observation->GetUpgrades();
	size_t base_count = observation->GetUnits(Unit::Alliance::Self, IsTownHall()).size();


	if (upgrades.empty()) {
		TryBuildUnit(ABILITY_ID::RESEARCH_STIMPACK, UNIT_TYPEID::TERRAN_BARRACKSTECHLAB);
	}
	else {
		for (const auto& upgrade : upgrades) {
			if (false) { // temp
				if (upgrade == UPGRADE_ID::TERRANSHIPWEAPONSLEVEL1 && base_count > 2) {
					TryBuildUnit(ABILITY_ID::RESEARCH_TERRANSHIPWEAPONS, UNIT_TYPEID::TERRAN_ARMORY);
				}
				else if (upgrade == UPGRADE_ID::TERRANVEHICLEWEAPONSLEVEL1 && base_count > 2) {
					TryBuildUnit(ABILITY_ID::RESEARCH_TERRANVEHICLEWEAPONS, UNIT_TYPEID::TERRAN_ARMORY);
				}
				else if (upgrade == UPGRADE_ID::TERRANVEHICLEANDSHIPARMORSLEVEL1 && base_count > 2) {
					TryBuildUnit(ABILITY_ID::RESEARCH_TERRANVEHICLEANDSHIPPLATING, UNIT_TYPEID::TERRAN_ARMORY);
				}
				else if (upgrade == UPGRADE_ID::TERRANVEHICLEWEAPONSLEVEL2 && base_count > 3) {
					TryBuildUnit(ABILITY_ID::RESEARCH_TERRANVEHICLEWEAPONS, UNIT_TYPEID::TERRAN_ARMORY);
				}
				else if (upgrade == UPGRADE_ID::TERRANVEHICLEANDSHIPARMORSLEVEL2 && base_count > 3) {
					TryBuildUnit(ABILITY_ID::RESEARCH_TERRANVEHICLEANDSHIPPLATING, UNIT_TYPEID::ZERG_SPIRE);
				}
				else {
					TryBuildUnit(ABILITY_ID::RESEARCH_TERRANVEHICLEWEAPONS, UNIT_TYPEID::TERRAN_ARMORY);
					TryBuildUnit(ABILITY_ID::RESEARCH_TERRANSHIPWEAPONS, UNIT_TYPEID::TERRAN_ARMORY);
					TryBuildUnit(ABILITY_ID::RESEARCH_TERRANVEHICLEANDSHIPPLATING, UNIT_TYPEID::TERRAN_ARMORY);
					TryBuildUnit(ABILITY_ID::RESEARCH_INFERNALPREIGNITER, UNIT_TYPEID::TERRAN_FACTORYTECHLAB);
				}
			}//Not mech build only
			else {
				if (CountUnitType(observation, UNIT_TYPEID::TERRAN_ARMORY) > 0) {
					if (upgrade == UPGRADE_ID::TERRANINFANTRYWEAPONSLEVEL1 && base_count > 2) {
						TryBuildUnit(ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONS, UNIT_TYPEID::TERRAN_ENGINEERINGBAY);
					}
					else if (upgrade == UPGRADE_ID::TERRANINFANTRYARMORSLEVEL1 && base_count > 2) {
						TryBuildUnit(ABILITY_ID::RESEARCH_TERRANINFANTRYARMOR, UNIT_TYPEID::TERRAN_ENGINEERINGBAY);
					}
					if (upgrade == UPGRADE_ID::TERRANINFANTRYWEAPONSLEVEL2 && base_count > 4) {
						TryBuildUnit(ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONS, UNIT_TYPEID::TERRAN_ENGINEERINGBAY);
					}
					else if (upgrade == UPGRADE_ID::TERRANINFANTRYARMORSLEVEL2 && base_count > 4) {
						TryBuildUnit(ABILITY_ID::RESEARCH_TERRANINFANTRYARMOR, UNIT_TYPEID::TERRAN_ENGINEERINGBAY);
					}
				}
				TryBuildUnit(ABILITY_ID::RESEARCH_STIMPACK, UNIT_TYPEID::TERRAN_BARRACKSTECHLAB);
				TryBuildUnit(ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONS, UNIT_TYPEID::TERRAN_ENGINEERINGBAY);
				TryBuildUnit(ABILITY_ID::RESEARCH_TERRANINFANTRYARMOR, UNIT_TYPEID::TERRAN_ENGINEERINGBAY);
				//TryBuildUnit(ABILITY_ID::RESEARCH_STIMPACK, UNIT_TYPEID::TERRAN_BARRACKSTECHLAB);
				TryBuildUnit(ABILITY_ID::RESEARCH_COMBATSHIELD, UNIT_TYPEID::TERRAN_BARRACKSTECHLAB);
				TryBuildUnit(ABILITY_ID::RESEARCH_CONCUSSIVESHELLS, UNIT_TYPEID::TERRAN_BARRACKSTECHLAB);
				//TryBuildUnit(ABILITY_ID::RESEARCH_PERSONALCLOAKING, UNIT_TYPEID::TERRAN_GHOSTACADEMY);
				//TryBuildUnit(ABILITY_ID::RESEARCH_BANSHEECLOAKINGFIELD, UNIT_TYPEID::TERRAN_STARPORTTECHLAB);
			}
		}
	}
}

void Feeder::ManageArmy() {

	const ObservationInterface* observation = Observation();

	Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);

	Units army = observation->GetUnits(Unit::Alliance::Self, IsArmy(observation));
	int wait_til_supply = 50;

	Units nuke = observation->GetUnits(Unit::Self, IsUnit(UNIT_TYPEID::TERRAN_NUKE));
	for (const auto& unit : army) {
		if (enemy_units.empty() && observation->GetFoodArmy() < wait_til_supply) {
			switch (unit->unit_type.ToType()) {
			case UNIT_TYPEID::TERRAN_SIEGETANKSIEGED: {
				Actions()->UnitCommand(unit, ABILITY_ID::MORPH_UNSIEGE);
				break;
			}
			default:
				RetreatWithUnit(unit, staging_location_);
				break;
			}
		}
		else if (!enemy_units.empty()) {
			switch (unit->unit_type.ToType()) {
			case UNIT_TYPEID::TERRAN_WIDOWMINE: {
				float distance = std::numeric_limits<float>::max();
				for (const auto& u : enemy_units) {
					float d = Distance2D(u->pos, unit->pos);
					if (d < distance) {
						distance = d;
					}
				}
				if (distance < 6) {
					Actions()->UnitCommand(unit, ABILITY_ID::BURROWDOWN);
				}
				break;
			}
			case UNIT_TYPEID::TERRAN_MARINE: {
				if (stim_researched_ && !unit->orders.empty()) {
					if (unit->orders.front().ability_id == ABILITY_ID::ATTACK) {
						float distance = std::numeric_limits<float>::max();
						for (const auto& u : enemy_units) {
							float d = Distance2D(u->pos, unit->pos);
							if (d < distance) {
								distance = d;
							}
						}
						bool has_stimmed = false;
						for (const auto& buff : unit->buffs) {
							if (buff == BUFF_ID::STIMPACK) {
								has_stimmed = true;
							}
						}
						if (distance < 6 && !has_stimmed) {
							Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_STIM);
							break;
						}
					}

				}
				AttackWithUnit(unit, observation);
				break;
			}
			case UNIT_TYPEID::TERRAN_MARAUDER: {
				if (stim_researched_ && !unit->orders.empty()) {
					if (unit->orders.front().ability_id == ABILITY_ID::ATTACK) {
						float distance = std::numeric_limits<float>::max();
						for (const auto& u : enemy_units) {
							float d = Distance2D(u->pos, unit->pos);
							if (d < distance) {
								distance = d;
							}
						}
						bool has_stimmed = false;
						for (const auto& buff : unit->buffs) {
							if (buff == BUFF_ID::STIMPACK) {
								has_stimmed = true;
							}
						}
						if (distance < 7 && !has_stimmed) {
							Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_STIM);
							break;
						}
					}
				}
				AttackWithUnit(unit, observation);
				break;
			}
			case UNIT_TYPEID::TERRAN_GHOST: {
				float distance = std::numeric_limits<float>::max();
				const Unit* closest_unit = nullptr;
				for (const auto& u : enemy_units) {
					float d = Distance2D(u->pos, unit->pos);
					if (d < distance) {
						distance = d;
						closest_unit = u;
					}
				}
				if (ghost_cloak_researched_) {
					if (distance < 7 && unit->energy > 50) {
						Actions()->UnitCommand(unit, ABILITY_ID::BEHAVIOR_CLOAKON);
						break;
					}
				}
				if (closest_unit && nuke_built ) {
					Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_NUKECALLDOWN, closest_unit->pos);
				}
				else if (unit->energy > 50 && !unit->orders.empty()) {
					if (unit->orders.front().ability_id == ABILITY_ID::ATTACK)
						Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_GHOSTSNIPE, unit);
					break;
				}
				AttackWithUnit(unit, observation);
				break;
			}
			case UNIT_TYPEID::TERRAN_SIEGETANK: {
				float distance = std::numeric_limits<float>::max();
				for (const auto& u : enemy_units) {
					float d = Distance2D(u->pos, unit->pos);
					if (d < distance) {
						distance = d;
					}
				}
				if (distance < 11) {
					Actions()->UnitCommand(unit, ABILITY_ID::MORPH_SIEGEMODE);
				}
				else {
					AttackWithUnit(unit, observation);
				}
				break;
			}
			case UNIT_TYPEID::TERRAN_SIEGETANKSIEGED: {
				float distance = std::numeric_limits<float>::max();
				for (const auto& u : enemy_units) {
					float d = Distance2D(u->pos, unit->pos);
					if (d < distance) {
						distance = d;
					}
				}
				if (distance > 13) {
					Actions()->UnitCommand(unit, ABILITY_ID::MORPH_UNSIEGE);
				}
				else {
					AttackWithUnit(unit, observation);
				}
				break;
			}
			case UNIT_TYPEID::TERRAN_MEDIVAC: {
				Units bio_units = observation->GetUnits(Unit::Self, IsUnits(bio_types));
				if (unit->orders.empty()) {
					for (const auto& bio_unit : bio_units) {
						if (bio_unit->health < bio_unit->health_max) {
							Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_HEAL, bio_unit);
							break;
						}
					}
					if (!bio_units.empty()) {
						Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, bio_units.front());
					}
				}
				break;
			}
			case UNIT_TYPEID::TERRAN_VIKINGFIGHTER: {
				Units flying_units = observation->GetUnits(Unit::Enemy, IsFlying());
				if (flying_units.empty()) {
					Actions()->UnitCommand(unit, ABILITY_ID::MORPH_VIKINGASSAULTMODE);
				}
				else {
					Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, flying_units.front());
				}
				break;
			}
			case UNIT_TYPEID::TERRAN_VIKINGASSAULT: {
				Units flying_units = observation->GetUnits(Unit::Enemy, IsFlying());
				if (!flying_units.empty()) {
					Actions()->UnitCommand(unit, ABILITY_ID::MORPH_VIKINGFIGHTERMODE);
				}
				else {
					AttackWithUnit(unit, observation);
				}
				break;
			}
			case UNIT_TYPEID::TERRAN_CYCLONE: {
				Units flying_units = observation->GetUnits(Unit::Enemy, IsFlying());
				if (!flying_units.empty() && unit->orders.empty()) {
					Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_LOCKON, flying_units.front());
				}
				else if (!flying_units.empty() && !unit->orders.empty()) {
					if (unit->orders.front().ability_id != ABILITY_ID::EFFECT_LOCKON) {
						Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_LOCKON, flying_units.front());
					}
				}
				else {
					AttackWithUnit(unit, observation);
				}
				break;
			}
			case UNIT_TYPEID::TERRAN_HELLION: {
				if (CountUnitType(observation, UNIT_TYPEID::TERRAN_ARMORY) > 0) {
					Actions()->UnitCommand(unit, ABILITY_ID::MORPH_HELLBAT);
				}
				AttackWithUnit(unit, observation);
				break;
			}
			case UNIT_TYPEID::TERRAN_BANSHEE: {
				if (banshee_cloak_researched_) {
					float distance = std::numeric_limits<float>::max();
					for (const auto& u : enemy_units) {
						float d = Distance2D(u->pos, unit->pos);
						if (d < distance) {
							distance = d;
						}
					}
					if (distance < 7 && unit->energy > 50) {
						Actions()->UnitCommand(unit, ABILITY_ID::BEHAVIOR_CLOAKON);
					}
				}
				AttackWithUnit(unit, observation);
				break;
			}
			case UNIT_TYPEID::TERRAN_RAVEN: {
				if (unit->energy > 125) {
					Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_HUNTERSEEKERMISSILE, enemy_units.front());
					break;
				}
				if (unit->orders.empty()) {
					Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, army.front()->pos);
				}
				break;
			}
			default: {
				AttackWithUnit(unit, observation);
			}
			}
		}
		else {
			switch (unit->unit_type.ToType()) {
			case UNIT_TYPEID::TERRAN_SIEGETANKSIEGED: {
				Actions()->UnitCommand(unit, ABILITY_ID::MORPH_UNSIEGE);
				break;
			}
			case UNIT_TYPEID::TERRAN_MEDIVAC: {
				Units bio_units = observation->GetUnits(Unit::Self, IsUnits(bio_types));
				if (unit->orders.empty()) {
					Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, bio_units.front()->pos);
				}
				break;
			}
			default:
				ScoutWithUnit(unit, observation);
				break;
			}
		}
	}
}