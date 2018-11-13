#include <iostream>
#include <cstdlib>

#include "Feeder.h"
#include "sc2api/sc2_api.h"
#include "sc2api/sc2_types.h"

using namespace sc2;

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
		if (base->assigned_harvesters <= base->ideal_harvesters && base->build_progress == 1) {

			return TryBuildUnit(ABILITY_ID::TRAIN_SCV, base->unit_type);

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
