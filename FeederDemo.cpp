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

	rally_point.x = 0.0f;
	rally_point.y = 0.0f;
	scv_scouting = false;
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

bool Feeder::BuildStructure(ABILITY_ID ability_type_for_structure, size_t count, UNIT_TYPEID unit_type) {
	const ObservationInterface *observation = Observation();

	// If a unit already is building a supply structure of this type, do nothing.
	// Also get an scv to build the structure.
	const Unit *unit_to_build = nullptr;
	Units units = observation->GetUnits(Unit::Alliance::Self);
	for (const auto &unit : units) {
		for (const auto &order : unit->orders) {
			if (order.ability_id == ability_type_for_structure) {
				count--;
				if (!count) {
					return false;
				}
			}
		}

		if (unit->unit_type == unit_type) {
			unit_to_build = unit;
		}
	}

	Point2D point = GetGoodBuildingLocation();

	Actions()->UnitCommand(unit_to_build, ability_type_for_structure, point);

	return true;
}
