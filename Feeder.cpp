#include <iostream>
#include <cstdlib>

#include "Feeder.h"
#include "sc2api/sc2_api.h"
#include "sc2api/sc2_types.h"

using namespace sc2;

struct IsAttackable {
	bool operator()(const Unit& unit) {
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::ZERG_OVERLORD: return false;
		case UNIT_TYPEID::ZERG_OVERSEER: return false;
		case UNIT_TYPEID::PROTOSS_OBSERVER: return false;
		default: return true;
		}
	}
};

struct IsTownHall {
	bool operator()(const Unit& unit) {
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::ZERG_HATCHERY: return true;
		case UNIT_TYPEID::ZERG_LAIR: return true;
		case UNIT_TYPEID::ZERG_HIVE: return true;
		case UNIT_TYPEID::TERRAN_COMMANDCENTER: return true;
		case UNIT_TYPEID::TERRAN_ORBITALCOMMAND: return true;
		case UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING: return true;
		case UNIT_TYPEID::TERRAN_PLANETARYFORTRESS: return true;
		case UNIT_TYPEID::PROTOSS_NEXUS: return true;
		default: return false;
		}
	}
};

struct IsStructure {
	IsStructure(const ObservationInterface* obs) : observation_(obs) {};

	bool operator()(const Unit& unit) {
		auto& attributes = observation_->GetUnitTypeData().at(unit.unit_type).attributes;
		bool is_structure = false;
		for (const auto& attribute : attributes) {
			if (attribute == Attribute::Structure) {
				is_structure = true;
			}
		}
		return is_structure;
	}

	const ObservationInterface* observation_;
};


Feeder::Feeder() {
	PrintStatus("Feeder has been initialized..");

	enemy_base_point.x = 0.0f;
	enemy_base_point.y = 0.0f;

	scv_scouting = false;
}

//void Feeder::OnGameStart(){}
// this is comment
void Feeder::OnStep() {
	Observate();
	TryBuildSCV();
	BuildStructures();
	BuildArmy();
	if (enemy_base_point.x == 0.0f && enemy_base_point.y == 0.0f) { // not found enemy base
		ScoutWithSCV();
	}
	else { // found enemy base
		ScoutWithMarines();
		//AttackWithAllUnits();
	}
	
}

void Feeder::OnUnitDestroyed(const sc2::Unit *unit) {

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
	if (bases.size() < 3 && observation->GetMinerals() > 1000) {
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
		TryBuildMarine();
		TryBuildMarauder();
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
			observation->GetUnits(Unit::Alliance::Self, IsUnits(barrack_types)).size() < 2) {
		TryBuildStructure(ABILITY_ID::BUILD_FACTORY, 1, UNIT_TYPEID::TERRAN_SCV);
	}
	else if (observation->GetUnits(Unit::Alliance::Self, IsUnits(barrack_types)).size() &&
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
				Actions()->UnitCommand(base, ABILITY_ID::EFFECT_CALLDOWNMULE);
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

void Feeder::ScoutWithSCV() {
	const ObservationInterface* observation = Observation();

	Units units = Observation()->GetUnits(Unit::Alliance::Self);

	if (!scv_scouting) {
		scv_scouting = true;
	}
	else {
		return;
	}
	
	// get one scv to scout
	for (auto unit : units){
		UnitTypeID unit_type(units.front()->unit_type);
		
		if (unit_type != UNIT_TYPEID::TERRAN_SCV)
			continue;

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
	max_p = game_info.playable_max;

	rtv.push_back(Point2D(my_start_pos.x, max_p.y - my_start_pos.y));
	rtv.push_back(Point2D(max_p.x - my_start_pos.x, my_start_pos.y));
	rtv.push_back(Point2D(max_p.x - my_start_pos.x, max_p.y - my_start_pos.y));
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
	Units units = Observation()->GetUnits(Unit::Alliance::Self);
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
		if (unit_type != UNIT_TYPEID::TERRAN_MARINE)
			continue;

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
			Actions()->UnitCommand(unit, ABILITY_ID::SMART, target_pos);
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

