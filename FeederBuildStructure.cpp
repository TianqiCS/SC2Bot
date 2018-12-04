#include <iostream>
#include <cstdlib>

#include "Feeder.h"
#include "sc2api/sc2_api.h"
#include "sc2api/sc2_types.h"

using namespace sc2;


void Feeder::BuildStructures() {
	TryBuildSupplyDepot();
	TryBuildTrainingFacilities();
	BuildRefinery();
	TryBuildResearch();
	ManageUpgrades();
}

bool Feeder::TryBuildSupplyDepot() {
	const ObservationInterface* observation = Observation();
	barracks_num = observation->GetUnits(Unit::Alliance::Self, IsUnits(barrack_types)).size();
	starport_num = observation->GetUnits(Unit::Alliance::Self, IsUnits(starport_types)).size();

	// If we do not have enough minerals
	if (observation->GetMinerals() < 100) {
		return false;
	}

	// If we are not supply capped, don't build a supply depot.
	if (observation->GetFoodUsed() < observation->GetFoodCap() - (3 + barracks_num * 2 + starport_num * 1)) {
		return false;
	}

	//check to see if there is already on building
	Units units = observation->GetUnits(Unit::Alliance::Self, IsUnits(supply_depot_types));
	if (observation->GetFoodUsed() < 50) {
		for (const auto& unit : units) {
			if (unit->build_progress != 1) {
				if (observation->GetFoodUsed() < observation->GetFoodCap()-1) {
					return false;
				}
				else {
					return false;
					TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT, 2, UNIT_TYPEID::TERRAN_SCV);
				}
			}
		}
	}
	else {
		if (observation->GetFoodUsed() == observation->GetFoodCap()) {
			TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT, 1, UNIT_TYPEID::TERRAN_SCV);
		}
	}

	// Try and build a supply depot. Find a random SCV and give it the order.
	Point2D build_location = GetGoodBuildingLocation();
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
		if (observation->GetMinerals() > 200 && barracks_num < 4) {
			TryBuildStructure(ABILITY_ID::BUILD_BARRACKS, 2, UNIT_TYPEID::TERRAN_SCV);
		}
	}
	else if (observation->GetMinerals() > 500) {  // why not use the extra money to build our army
		TryBuildStructure(ABILITY_ID::BUILD_BARRACKS, 2, UNIT_TYPEID::TERRAN_SCV);
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

	//only build refinery after we have one barrack
	barracks_num = observation->GetUnits(Unit::Alliance::Self, IsUnits(barrack_types)).size();
	if (barracks_num < 0) { return false; }


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

bool Feeder::TryBuildResearch()
{
	const ObservationInterface* observation = Observation();

	barracks_num = observation->GetUnits(Unit::Alliance::Self, IsUnits(barrack_types)).size();
	factory_num = observation->GetUnits(Unit::Alliance::Self, IsUnits(factory_types)).size();
	size_t base_num = observation->GetUnits(Unit::Alliance::Self, IsTownHall()).size();

	if (barracks_num >= 4 && !CountUnitType(observation, UNIT_TYPEID::TERRAN_ENGINEERINGBAY)) {
		if (observation->GetMinerals() > 150) {
			TryBuildStructure(ABILITY_ID::BUILD_ENGINEERINGBAY, 1, UNIT_TYPEID::TERRAN_SCV);
		}
	}
	else if (CountUnitType(observation, UNIT_TYPEID::TERRAN_REFINERY) > 0 &&
		observation->GetUnits(Unit::Alliance::Self, IsUnits(factory_types)).size() > 0 &&
		!CountUnitType(observation, UNIT_TYPEID::TERRAN_ARMORY) &&
		base_num > 1) {
			TryBuildStructure(ABILITY_ID::BUILD_ARMORY, 1, UNIT_TYPEID::TERRAN_SCV);
	}

	return true;
}

