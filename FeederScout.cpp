#include <iostream>
#include <cstdlib>

#include "Feeder.h"
#include "sc2api/sc2_api.h"
#include "sc2api/sc2_types.h"

using namespace sc2;


void Feeder::tryScoutWithSCV() {
	if (enemy_base_point.x == 0.0f && enemy_base_point.y == 0.0f) { // not found enemy base
		ScoutWithSCV();
	}
	else { // found enemy base
		ManageArmy();
		//AttackWithAllUnits();
	}

}

// TODO KNOWN BUG: in some cases, scv would not be sent to scout, 
// probable cause: conflit between Scouting method and (BuildStructure || onIdle)

void Feeder::ScoutWithSCV() {
	const ObservationInterface* observation = Observation();

	// get all SCV
	Units SCVS = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_SCV));
	barracks_num = observation->GetUnits(Unit::Alliance::Self, IsUnits(barrack_types)).size();

	if (barracks_num >= 0 && SCVS.size() < 15) { return; }

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
	for (auto unit : SCVS) {

		Point2D target_pos;
		std::vector<Point2D> start_loactions;

		GetAllEnemyBaseLocation(start_loactions);

		// for each possible enemy base location
		for (Point2D &point : start_loactions) {
			Actions()->UnitCommand(unit, ABILITY_ID::SMART, point, true); // queue orders
		}

		my_base_point = start_loactions.back();

		break;
	}

	scv_scouting = true;

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

