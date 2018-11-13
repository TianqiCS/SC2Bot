#include <iostream>
#include <cstdlib>

#include "Feeder.h"
#include "sc2api/sc2_api.h"
#include "sc2api/sc2_types.h"

using namespace sc2;

void Feeder::ManageArmy() {

	const ObservationInterface* observation = Observation();

	Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);

	Units army = observation->GetUnits(Unit::Alliance::Self, IsArmy(observation));

	if (army.empty()) {
		return;
	}
	int wait_til_supply = 30;

	Units nuke = observation->GetUnits(Unit::Self, IsUnit(UNIT_TYPEID::TERRAN_NUKE));
	for (const auto& unit : army) {
		if (enemy_units.empty() && observation->GetFoodArmy() < wait_til_supply) {
			RetreatWithUnit(unit, staging_location_);
		}
		// time to atack
		else if (!enemy_units.empty() && observation->GetFoodArmy() > wait_til_supply) {
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
						if (distance < 6 && !has_stimmed) { // old value 6
							Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_STIM_MARINE);
							//PrintStatus("use stimpack");
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
						if (distance < 7 && !has_stimmed) {  // old value 7
							Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_STIM_MARAUDER);
							//PrintStatus("use stimpack");
							break;
						}
					}
				}
				AttackWithUnit(unit, observation);
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
			default:
				AttackWithUnit(unit, observation);
			}
		}
		else {
			switch (unit->unit_type.ToType()) {
			case UNIT_TYPEID::TERRAN_MEDIVAC: {
				Units bio_units = observation->GetUnits(Unit::Self, IsUnits(bio_types));
				if (unit->orders.empty()) {
					Actions()->UnitCommand(unit, ABILITY_ID::SMART, bio_units.front()->pos);
				}
				break;
			}
			default:
				//ScoutWithUnit(unit, observation);
				break;
			}
		}
	}
}

