#pragma once

#include "sc2api/sc2_interfaces.h"
#include "sc2api/sc2_agent.h"
#include "sc2api/sc2_map_info.h"
#include "sc2lib/sc2_lib.h"
#include "Logger.h"
#include "Bot.h"

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

struct IsFlying {
	bool operator()(const Unit& unit) {
		return unit.is_flying;
	}
};

//Ignores Overlords, workers, and structures
struct IsArmy {
	IsArmy(const ObservationInterface* obs) : observation_(obs) {}

	bool operator()(const Unit& unit) {
		auto attributes = observation_->GetUnitTypeData().at(unit.unit_type).attributes;
		for (const auto& attribute : attributes) {
			if (attribute == Attribute::Structure) {
				return false;
			}
		}
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::ZERG_OVERLORD: return false;
		case UNIT_TYPEID::PROTOSS_PROBE: return false;
		case UNIT_TYPEID::ZERG_DRONE: return false;
		case UNIT_TYPEID::TERRAN_SCV: return false;
		case UNIT_TYPEID::ZERG_QUEEN: return false;
		case UNIT_TYPEID::ZERG_LARVA: return false;
		case UNIT_TYPEID::ZERG_EGG: return false;
		case UNIT_TYPEID::TERRAN_MULE: return false;
		case UNIT_TYPEID::TERRAN_NUKE: return false;
		default: return true;
		}
	}

	const ObservationInterface* observation_;
};

struct IsMyArmy
{
	bool operator()(const Unit& unit) {
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::TERRAN_MARINE: return true;
		case UNIT_TYPEID::TERRAN_MARAUDER: return true;
		case UNIT_TYPEID::TERRAN_GHOST: return true;
			// ...
		default: return false;
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

struct IsVespeneGeyser {
	bool operator()(const Unit& unit) {
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::NEUTRAL_VESPENEGEYSER: return true;
		case UNIT_TYPEID::NEUTRAL_SPACEPLATFORMGEYSER: return true;
		case UNIT_TYPEID::NEUTRAL_PROTOSSVESPENEGEYSER: return true;
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

class Feeder : public Bot
{
public:
	Feeder();

	// events
	virtual void OnGameStart() override;
	virtual void OnStep() override;
    virtual void OnUnitIdle(const sc2::Unit *unit) override;
	virtual void OnUnitDestroyed(const sc2::Unit *unit) override;
    virtual void OnUnitCreated(const sc2::Unit *unit) override;
    virtual void OnUnitEnterVision(const sc2::Unit *unit) override;
	virtual void OnBuildingConstructionComplete(const sc2::Unit *unit) override;
	
	// build structures
	void BuildStructures();
	bool TryBuildSupplyDepot();
	bool TryBuildTrainingFacilities();
	bool TryBuildExpansionCom();
	bool TryBuildAddOn(AbilityID ability_type_for_structure, Tag base_structure);
	bool BuildRefinery();
	bool TryBuildResearch();
	

	// build units
	void BuildArmy();
	bool TryBuildSCV();
	bool TryBuildMarine();
	bool TryBuildMarauder();
	bool TrybuildMedivac();

	// manage && research
	void ManageArmy();
	void ManageUpgrades();

	void ScoutWithMarines();
	void tryScoutWithSCV();
	void ScoutWithSCV();

	// utilitys
	void GetRallyPointOnRocks();
	void GetAllEnemyBaseLocation(std::vector<Point2D> &rtv);
	void GetNeareastBaseLocation(Point2D &point);

	// demo || unsorted
	void AttackWithAllUnits();
	void Observate();

private:
	// different typenames of one type unit
	std::vector<UNIT_TYPEID> barrack_types = { UNIT_TYPEID::TERRAN_BARRACKSFLYING, UNIT_TYPEID::TERRAN_BARRACKS };
	std::vector<UNIT_TYPEID> factory_types = { UNIT_TYPEID::TERRAN_FACTORYFLYING, UNIT_TYPEID::TERRAN_FACTORY };
	std::vector<UNIT_TYPEID> starport_types = { UNIT_TYPEID::TERRAN_STARPORTFLYING, UNIT_TYPEID::TERRAN_STARPORT };
	std::vector<UNIT_TYPEID> supply_depot_types = { UNIT_TYPEID::TERRAN_SUPPLYDEPOT, UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED };
	std::vector<UNIT_TYPEID> bio_types = { UNIT_TYPEID::TERRAN_MARINE, UNIT_TYPEID::TERRAN_MARAUDER, UNIT_TYPEID::TERRAN_GHOST, UNIT_TYPEID::TERRAN_REAPER /*reaper*/ };
	std::vector<UNIT_TYPEID> widow_mine_types = { UNIT_TYPEID::TERRAN_WIDOWMINE, UNIT_TYPEID::TERRAN_WIDOWMINEBURROWED };
	std::vector<UNIT_TYPEID> siege_tank_types = { UNIT_TYPEID::TERRAN_SIEGETANK, UNIT_TYPEID::TERRAN_SIEGETANKSIEGED };
	std::vector<UNIT_TYPEID> viking_types = { UNIT_TYPEID::TERRAN_VIKINGASSAULT, UNIT_TYPEID::TERRAN_VIKINGFIGHTER };
	std::vector<UNIT_TYPEID> hellion_types = { UNIT_TYPEID::TERRAN_HELLION, UNIT_TYPEID::TERRAN_HELLIONTANK };

	// temp identifiers (direct copy from example terranBot) @FIX ME
	bool nuke_built = false;
	bool stim_researched_ = false;
	bool ghost_cloak_researched_ = true;
	bool banshee_cloak_researched_ = true;

	// my identifiers 
	size_t scv_num;
	size_t depot_num;
	size_t barracks_num;
	size_t factory_num;
	size_t refinery_num;
	size_t marine_num;
	size_t marauder_num;
	size_t food_used;
	size_t food_cap;
	size_t minerals;
	size_t gas;

	// game stats results
	Point2D my_base_point;
	Point2D enemy_base_point;
	Point2D rally_point;
	GameInfo game_info_;
	bool scv_scouting;

	// ...
	void clearObservation() {
		scv_num = 0;
		depot_num = 0;
		barracks_num = 0;
		factory_num = 0;
		refinery_num = 0;
		marine_num = 0;
		marauder_num = 0;
		food_used = 0;
		food_cap = 0;
		minerals = 0;
		gas = 0;
	}

	// interfaces 
	ActionInterface* Action();
};

