#include <iostream>
#include <cstdlib>

#include "Feeder.h"
#include "sc2api/sc2_api.h"
#include "sc2api/sc2_types.h"

using namespace sc2;


void Feeder::OnGameStart() {
	// call a method to calculate rally point depending on undestructible rock
	// 
	// Actions()->UnitCommand(const Unit* unit, AbilityID ability, const Point2D& point, bool queued_command = false)
	GetRallyPointOnRocks();
	std::cout << "start!" << std::endl;
	move_back_ = false;
	targeted_enemy_ = 0;


}
// this is comment
void Feeder::OnStep() {

	//Observate();

	microControl();
	/*
	TryBuildSCV();
	BuildStructures();
	BuildArmy();
	ManageArmy();
	tryScoutWithSCV();

	*/
	
}

void Feeder::OnUnitDestroyed(const sc2::Unit *unit) {
	resetMicroControl(unit);
	if (unit->alliance == Unit::Alliance::Enemy) {
		
	}

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
		Actions()->UnitCommand(unit, ABILITY_ID::RALLY_UNITS, rally_point, false);
		break;
	}
	case UNIT_TYPEID::TERRAN_FACTORY: {
		Actions()->UnitCommand(unit, ABILITY_ID::RALLY_UNITS, rally_point, false);
		break;
	}
	case UNIT_TYPEID::TERRAN_STARPORT: {
		Actions()->UnitCommand(unit, ABILITY_ID::RALLY_UNITS, rally_point, false);
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

void Feeder::OnGameEnd() {
	// result file
	//FILE * pFile;
	//pFile = fopen_s("data.txt", "w");
	//freopen("data.txt", "w", stdout);
	//std::cout << "write in file";

	std::cout << "Game Ended for: " << std::to_string(Control()->Proto().GetAssignedPort()) << std::endl;
	//fprintf(pFile, "%s", std::to_string(Control()->Proto().GetAssignedPort()));
	return;
}
