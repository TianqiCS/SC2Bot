#include <iostream>
#include <cstdlib>

#include "Feeder.h"
#include "sc2api/sc2_api.h"
#include "sc2api/sc2_types.h"

using namespace sc2;

bool Feeder::TryBuildUnitWithReactor(AbilityID ability_type_for_unit, UnitTypeID unit_type) {
	assert(unit_type == UNIT_TYPEID::TERRAN_BARRACKS || unit_type == UNIT_TYPEID::TERRAN_FACTORY || unit_type == UNIT_TYPEID::TERRAN_STARPORT);  // only these three building can use reactor

	const ObservationInterface* observation = Observation();

	//If we are at supply cap, don't build anymore units, unless its an overlord.
	if (observation->GetFoodUsed() >= observation->GetFoodCap() && ability_type_for_unit != ABILITY_ID::TRAIN_OVERLORD) {
		return false;
	}
	const Unit* unit = nullptr;
	if (!GetRandomUnit(unit, observation, unit_type)) {
		return false;
	}
	if (unit->orders.size()) {
		if (unit->orders.size() == 1) {
			// observation->GetUnit(unit->add_on_tag)->unit_type == UNIT_TYPEID::TERRAN_REACTOR &&
			Actions()->UnitCommand(unit, ability_type_for_unit);
			return true;
		}
		return false;
	}

	if (unit->build_progress != 1) {
		return false;
	}

	Actions()->UnitCommand(unit, ability_type_for_unit);
	return true;
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
	Units medivac = observation->GetUnits(Unit::Self, IsUnit(UNIT_TYPEID::TERRAN_MEDIVAC));

	Units supply_depots = observation->GetUnits(Unit::Self, IsUnit(UNIT_TYPEID::TERRAN_SUPPLYDEPOT));

	bool overloaded = false;  // our mines are empty

	TryBuildExpansionCom();

	for (const auto& supply_depot : supply_depots) {
		Actions()->UnitCommand(supply_depot, ABILITY_ID::MORPH_SUPPLYDEPOT_LOWER);
	}

	if (!barracks.empty()) {
		for (const auto& base : bases) {
			if (base->unit_type == UNIT_TYPEID::TERRAN_COMMANDCENTER && observation->GetMinerals() > 251) {
				Actions()->UnitCommand(base, ABILITY_ID::MORPH_ORBITALCOMMAND);
			}
		}
	}


	for (const auto& barrack : barracks) {

		if (!barrack->orders.empty() || barrack->build_progress != 1) {
			continue;
		}


		// need at least 2 barracks to consider addon
		// if (barracks.size() < 2) { return; }
		// combined with following if block

		if (observation->GetUnit(barrack->add_on_tag) == nullptr && barracks.size() >= 2) {
			if (barracks_tech.size() < barracks.size() / 2 || barracks_tech.empty()) {
				TryBuildAddOn(ABILITY_ID::BUILD_TECHLAB_BARRACKS, barrack->tag);
			}
			else {
				// TODO: after adding reactor, the barrack still build 1 unit at a time
				// We need to build tow if we have reactor!
				TryBuildAddOn(ABILITY_ID::BUILD_REACTOR_BARRACKS, barrack->tag);
			}
		}

		TryBuildMarauder();
		TryBuildMarine();

	}

	for (const auto& starport : starports) {
		if (medivac.size() < 8) {TrybuildMedivac();}
	}

	/*
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
    */

	for (const auto& starport : starports) {
		if (!starport->orders.empty()) {
			continue;
		}
		if (observation->GetUnit(starport->add_on_tag) == nullptr) {
			if (CountUnitType(observation, UNIT_TYPEID::TERRAN_STARPORTREACTOR) < 1) {
				TryBuildAddOn(ABILITY_ID::BUILD_REACTOR_STARPORT, starport->tag);
			}
			else {
				TryBuildAddOn(ABILITY_ID::BUILD_TECHLAB_STARPORT, starport->tag);
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

