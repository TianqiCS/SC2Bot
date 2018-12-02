/*
 * The Bot class is a base abstract bot made from the example bot from sc2Api by Copyright (c) 2017 Blizzard Entertainment (MIT)
 * 
 */
#pragma once

#include "sc2api/sc2_interfaces.h"
#include "sc2api/sc2_agent.h"
#include "sc2api/sc2_map_info.h"
#include "sc2lib/sc2_lib.h"
#include <iostream>

using namespace sc2;

class Bot : public Agent {
public:
	bool nuke_detected = false;
	uint32_t nuke_detected_frame;

	bool FindEnemyStructure(const ObservationInterface * observation, const Unit *& enemy_unit);

	void PrintStatus(std::string msg);

	virtual void OnGameStart();

	size_t CountUnitType(const ObservationInterface* observation, UnitTypeID unit_type);

	size_t CountUnitTypeBuilding(const ObservationInterface* observation, UNIT_TYPEID production_building, ABILITY_ID ability);

	size_t CountUnitTypeTotal(const ObservationInterface* observation, UNIT_TYPEID unit_type, UNIT_TYPEID production, ABILITY_ID ability);

	size_t CountUnitTypeTotal(const ObservationInterface* observation, std::vector<UNIT_TYPEID> unit_type, UNIT_TYPEID production, ABILITY_ID ability);

	bool GetRandomUnit(const Unit*& unit_out, const ObservationInterface* observation, UnitTypeID unit_type);

	const Unit* FindNearestMineralPatch(const Point2D& start);

	// Tries to find a random location that can be pathed to on the map.
	// Returns 'true' if a new, random location has been found that is pathable by the unit.
	bool FindEnemyPosition(Point2D& target_pos);

	bool TryFindRandomPathableLocation(const Unit* unit, Point2D& target_pos);

	void AttackWithUnitType(UnitTypeID unit_type, const ObservationInterface* observation);

	void ScoutWithUnits(UnitTypeID unit_type, const ObservationInterface* observation);

	void RetreatWithUnits(UnitTypeID unit_type, Point2D retreat_position);

	void AttackWithUnit(const Unit* unit, const ObservationInterface* observation);

	void ScoutWithUnit(const Unit* unit, const ObservationInterface* observation);

	void RetreatWithUnit(const Unit* unit, Point2D retreat_position);

	//Try build structure given a location. This is used most of the time
	bool TryBuildStructure(AbilityID ability_type_for_structure, UnitTypeID unit_type, Point2D location, bool isExpansion);
	//Try to build a structure based on tag, Used mostly for Vespene, since the pathing check will fail even though the geyser is "Pathable"

	bool TryBuildStructure(AbilityID ability_type_for_structure, UnitTypeID unit_type, Tag location_tag);
	bool TryBuildStructure(ABILITY_ID ability_type_for_structure, size_t count, UNIT_TYPEID unit_type);
	//Expands to nearest location and updates the start location to be between the new location and old bases.

	bool TryExpand(AbilityID build_ability, UnitTypeID worker_type);
	//Tries to build a geyser for a base

	bool TryBuildGas(AbilityID build_ability, UnitTypeID worker_type, Point2D base_location);

	bool TryBuildUnit(AbilityID ability_type_for_unit, UnitTypeID unit_type);

	// Mine the nearest mineral to Town hall.
	// If we don't do this, probes may mine from other patches if they stray too far from the base after building.
	void MineIdleWorkers(const Unit* unit, AbilityID worker_gather_command, UnitTypeID vespene_building_type);

	//An estimate of how many workers we should have based on what buildings we have
	int GetExpectedWorkers(UNIT_TYPEID vespene_building_type);

	// To ensure that we do not over or under saturate any base.
	void ManageWorkers(UNIT_TYPEID worker_type, AbilityID worker_gather_command, UNIT_TYPEID vespene_building_type);

	// OUR CODE: return location that is not in mining area
	Point2D GetGoodBuildingLocation(const Unit *unit_to_build);

	virtual void OnNuclearLaunchDetected() final;

	uint32_t current_game_loop_ = 0;
	int max_worker_count_ = 100;

	//When to start building attacking units
	int target_worker_count_ = 15;
	GameInfo game_info_;
	std::vector<Point3D> expansions_;
	Point3D startLocation_;
	Point3D staging_location_;

private:
	std::string last_action_text_;

};

