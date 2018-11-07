#pragma once

#include "sc2api/sc2_interfaces.h"
#include "sc2api/sc2_agent.h"
#include "sc2api/sc2_map_info.h"
#include "sc2lib/sc2_lib.h"
#include "Logger.h"
#include "Bot.h"


class Feeder : public Bot
{
public:
	Feeder();
	//virtual void OnGameStart() override;
	virtual void OnStep() override;
    virtual void OnUnitIdle(const sc2::Unit *unit) override;
	virtual void OnUnitDestroyed(const sc2::Unit *unit) override;
    virtual void OnUnitCreated(const sc2::Unit *unit) override;
    virtual void OnUnitEnterVision(const sc2::Unit *unit) override;
	virtual void OnBuildingConstructionComplete(const sc2::Unit *unit) override;
	
	// scout function

	// buildings
	void Observate();
	void BuildStructures();
	bool TryBuildSupplyDepot();
	bool TryBuildTrainingFacilities();
	bool TryBuildExpansionCom();
	bool TryBuildAddOn(AbilityID ability_type_for_structure, Tag base_structure);
	bool BuildRefinery();

	// build units
	void BuildArmy();
	bool TryBuildSCV();
	bool TryBuildMarine();
	bool TryBuildMarauder();

	// actions
	void ScoutWithMarines();
	void ScoutWithSCV();
	void AttackWithAllUnits();

	// utilitys
	void GetAllEnemyBaseLocation(std::vector<Point2D> &rtv);

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

	//  scout results
	Point2D enemy_base_point;
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

