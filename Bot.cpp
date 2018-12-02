#include "Bot.h"

using namespace sc2;

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

int CountUnitType(const ObservationInterface* observation, UnitTypeID unit_type) {
	int count = 0;
	Units my_units = observation->GetUnits(Unit::Alliance::Self);
	for (const auto unit : my_units) {
		if (unit->unit_type == unit_type)
			++count;
	}

	return count;
}

bool Bot::FindEnemyStructure(const ObservationInterface* observation, const Unit*& enemy_unit) {
	Units my_units = observation->GetUnits(Unit::Alliance::Enemy);
	for (const auto unit : my_units) {
		if (unit->unit_type == UNIT_TYPEID::TERRAN_COMMANDCENTER ||
			unit->unit_type == UNIT_TYPEID::TERRAN_SUPPLYDEPOT ||
			unit->unit_type == UNIT_TYPEID::TERRAN_BARRACKS ||
			unit->unit_type == UNIT_TYPEID::ZERG_HATCHERY ||
			unit->unit_type == UNIT_TYPEID::ZERG_SPAWNINGPOOL ||
			unit->unit_type == UNIT_TYPEID::PROTOSS_PYLON ||
			unit->unit_type == UNIT_TYPEID::PROTOSS_NEXUS ||
			unit->unit_type == UNIT_TYPEID::PROTOSS_GATEWAY
			//...
			) {
			enemy_unit = unit;
			return true;
		}
	}

	return false;
}

bool GetRandomUnit(const Unit*& unit_out, const ObservationInterface* observation, UnitTypeID unit_type) {
	Units my_units = observation->GetUnits(Unit::Alliance::Self);
	std::random_shuffle(my_units.begin(), my_units.end()); // Doesn't work, or doesn't work well.
	for (const auto unit : my_units) {
		if (unit->unit_type == unit_type) {
			unit_out = unit;
			return true;
		}
	}
	return false;
}

void Bot::PrintStatus(std::string msg) {
	int64_t bot_identifier = int64_t(this) & 0xFFFLL;
	std::cout << std::to_string(bot_identifier) << ": " << msg << std::endl;
}

void Bot::OnGameStart() {
	game_info_ = Observation()->GetGameInfo();
	PrintStatus("game started.");
	expansions_ = search::CalculateExpansionLocations(Observation(), Query());

	//Temporary, we can replace this with observation->GetStartLocation() once implemented
	startLocation_ = Observation()->GetStartLocation();
	staging_location_ = startLocation_;
};

size_t Bot::CountUnitType(const ObservationInterface* observation, UnitTypeID unit_type) {
	return observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
}

size_t Bot::CountUnitTypeBuilding(const ObservationInterface* observation, UNIT_TYPEID production_building, ABILITY_ID ability) {
	int building_count = 0;
	Units buildings = observation->GetUnits(Unit::Self, IsUnit(production_building));

	for (const auto& building : buildings) {
		if (building->orders.empty()) {
			continue;
		}

		for (const auto order : building->orders) {
			if (order.ability_id == ability) {
				building_count++;
			}
		}
	}

	return building_count;
}

size_t Bot::CountUnitTypeTotal(const ObservationInterface* observation, UNIT_TYPEID unit_type, UNIT_TYPEID production, ABILITY_ID ability) {
	return CountUnitType(observation, unit_type) + CountUnitTypeBuilding(observation, production, ability);
}

size_t Bot::CountUnitTypeTotal(const ObservationInterface* observation, std::vector<UNIT_TYPEID> unit_type, UNIT_TYPEID production, ABILITY_ID ability) {
	size_t count = 0;
	for (const auto& type : unit_type) {
		count += CountUnitType(observation, type);
	}
	return count + CountUnitTypeBuilding(observation, production, ability);
}

// **may crash the game??
bool Bot::GetRandomUnit(const Unit*& unit_out, const ObservationInterface* observation, UnitTypeID unit_type) {
	Units my_units = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
	if (!my_units.empty()) {
		unit_out = GetRandomEntry(my_units);
		return true;
	}
	return false;
}

const Unit* Bot::FindNearestMineralPatch(const Point2D& start) {
	Units units = Observation()->GetUnits(Unit::Alliance::Neutral);
	float distance = std::numeric_limits<float>::max();
	const Unit* target = nullptr;
	for (const auto& u : units) {
		if (u->unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD) {
			float d = DistanceSquared2D(u->pos, start);
			if (d < distance) {
				distance = d;
				target = u;
			}
		}
	}
	//If we never found one return false;
	if (distance == std::numeric_limits<float>::max()) {
		return target;
	}
	return target;
}

// Tries to find a random location that can be pathed to on the map.
// Returns 'true' if a new, random location has been found that is pathable by the unit.
bool Bot::FindEnemyPosition(Point2D& target_pos) {
	if (game_info_.enemy_start_locations.empty()) {
		return false;
	}
	target_pos = game_info_.enemy_start_locations.front();
	return true;
}

bool Bot::TryFindRandomPathableLocation(const Unit* unit, Point2D& target_pos) {
	// First, find a random point inside the playable area of the map.
	float playable_w = game_info_.playable_max.x - game_info_.playable_min.x;
	float playable_h = game_info_.playable_max.y - game_info_.playable_min.y;

	// The case where game_info_ does not provide a valid answer
	if (playable_w == 0 || playable_h == 0) {
		playable_w = 236;
		playable_h = 228;
	}

	target_pos.x = playable_w * GetRandomFraction() + game_info_.playable_min.x;
	target_pos.y = playable_h * GetRandomFraction() + game_info_.playable_min.y;

	// Now send a pathing query from the unit to that point. Can also query from point to point,
	// but using a unit tag wherever possible will be more accurate.
	// Note: This query must communicate with the game to get a result which affects performance.
	// Ideally batch up the queries (using PathingDistanceBatched) and do many at once.
	float distance = Query()->PathingDistance(unit, target_pos);

	return distance > 0.1f;
}

void Bot::AttackWithUnitType(UnitTypeID unit_type, const ObservationInterface* observation) {
	Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
	for (const auto& unit : units) {
		AttackWithUnit(unit, observation);
	}
}

void Bot::AttackWithUnit(const Unit* unit, const ObservationInterface* observation) {
	//If unit isn't doing anything make it attack.
	Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
	if (enemy_units.empty()) {
		return;
	}

	if (unit->orders.empty()) {
		Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_units.front()->pos);
		return;
	}

	//If the unit is doing something besides attacking, make it attack.
	if (unit->orders.front().ability_id != ABILITY_ID::ATTACK) {
		if (unit->orders.size() < 2)
		{
			Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_units.front()->pos, true);
		}
	}
}

void Bot::ScoutWithUnits(UnitTypeID unit_type, const ObservationInterface* observation) {
	Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
	for (const auto& unit : units) {
		ScoutWithUnit(unit, observation);
	}
}

void Bot::ScoutWithUnit(const Unit* unit, const ObservationInterface* observation) {
	Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy, IsAttackable());
	if (!unit->orders.empty()) {
		return;
	}
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
		Actions()->UnitCommand(unit, ABILITY_ID::SMART, target_pos);
	}
	else {
		if (TryFindRandomPathableLocation(unit, target_pos)) {
			Actions()->UnitCommand(unit, ABILITY_ID::SMART, target_pos);
		}
	}
}

//Try build structure given a location. This is used most of the time
bool Bot::TryBuildStructure(AbilityID ability_type_for_structure, UnitTypeID unit_type, Point2D location, bool isExpansion = false) {

	const ObservationInterface* observation = Observation();
	Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));

	//if we have no workers Don't build
	if (workers.empty()) {
		return false;
	}

	// Check to see if there is already a worker heading out to build it
	for (const auto& worker : workers) {
		for (const auto& order : worker->orders) {
			if (order.ability_id == ability_type_for_structure) {
				return false;
			}
		}
	}

	// If no worker is already building one, get a random worker to build one
	const Unit* unit = GetRandomEntry(workers);

	// Check to see if unit can make it there
	if (Query()->PathingDistance(unit, location) < 0.1f) {
		return false;
	}
	if (!isExpansion) {
		for (const auto& expansion : expansions_) {
			if (Distance2D(location, Point2D(expansion.x, expansion.y)) < 7) {
				return false;
			}
		}
	}
	// Check to see if unit can build there
	if (Query()->Placement(ability_type_for_structure, location)) {
		Actions()->UnitCommand(unit, ability_type_for_structure, location);
		return true;
	}
	return false;

}

//Try to build a structure based on tag, Used mostly for Vespene, since the pathing check will fail even though the geyser is "Pathable"
bool Bot::TryBuildStructure(AbilityID ability_type_for_structure, UnitTypeID unit_type, Tag location_tag) {
	const ObservationInterface* observation = Observation();
	Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
	const Unit* target = observation->GetUnit(location_tag);

	if (workers.empty()) {
		return false;
	}

	// Check to see if there is already a worker heading out to build it
	for (const auto& worker : workers) {
		for (const auto& order : worker->orders) {
			if (order.ability_id == ability_type_for_structure) {
				return false;
			}
		}
	}

	// If no worker is already building one, get a random worker to build one
	const Unit* unit = GetRandomEntry(workers);

	// Check to see if unit can build there
	if (Query()->Placement(ability_type_for_structure, target->pos)) {
		Actions()->UnitCommand(unit, ability_type_for_structure, target);
		return true;
	}
	return false;

}

// OUR CODE: return location that is not in mining area
Point2D Bot::GetGoodBuildingLocation(const Unit *unit_to_build) {
	const ObservationInterface* observation = Observation();
	float rx, ry;
	Point2D build_location;
	bool IsGoodLocation = false;
	std::vector<std::pair<Point2D, Point2D> > resource_base_pair;

	// findall resources on map
	Units resources = observation->GetUnits(
		[](const Unit& unit) {
		return unit.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD750 ||
			unit.unit_type == UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD750 ||
			unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERMINERALFIELD750 ||
			unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERRICHMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERRICHMINERALFIELD750 ||
			unit.unit_type == UNIT_TYPEID::NEUTRAL_LABMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_LABMINERALFIELD750 ||
			unit.unit_type == UNIT_TYPEID::NEUTRAL_BATTLESTATIONMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_BATTLESTATIONMINERALFIELD750 ||
			unit.unit_type == UNIT_TYPEID::NEUTRAL_VESPENEGEYSER || unit.unit_type == UNIT_TYPEID::NEUTRAL_PROTOSSVESPENEGEYSER ||
			unit.unit_type == UNIT_TYPEID::NEUTRAL_SPACEPLATFORMGEYSER || unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERVESPENEGEYSER ||
			unit.unit_type == UNIT_TYPEID::NEUTRAL_SHAKURASVESPENEGEYSER || unit.unit_type == UNIT_TYPEID::NEUTRAL_RICHVESPENEGEYSER;
	}
	);

	// findall self command center
	Units command_centers = observation->GetUnits(
		[](const Unit& unit) {
		return (unit.unit_type == UNIT_TYPEID::TERRAN_COMMANDCENTER || unit.unit_type == UNIT_TYPEID::TERRAN_PLANETARYFORTRESS ||
			unit.unit_type == UNIT_TYPEID::TERRAN_ORBITALCOMMAND) && unit.alliance == Unit::Alliance::Self;
	}
	);

	// if not found command center
	if (command_centers.empty()) {
		std::cerr << "fail to find command center" << std::endl;
		rx = GetRandomScalar();
		ry = GetRandomScalar();
		build_location = Point2D(unit_to_build->pos.x + rx * 15.0f, unit_to_build->pos.y + ry * 15.0f);

		return build_location;

	}

	// for each resource, find its closest command center
	for (auto resource : resources) {
		Point2D closest_base;
		float min_distance;

		closest_base.x = command_centers.front()->pos.x;
		closest_base.y = command_centers.front()->pos.y;

		min_distance = Distance2D(closest_base, Point2D(resource->pos.x, resource->pos.y));

		// find closest base for current resource
		for (auto base : command_centers) {
			float new_d, new_x, new_y;

			new_x = base->pos.x;
			new_y = base->pos.y;
			new_d = Distance2D(Point2D(new_x, new_y), Point2D(resource->pos.x, resource->pos.y));

			if (new_d < min_distance) {
				min_distance = new_d;
				closest_base.x = new_x;
				closest_base.y = new_y;

			}
		}

		// if close enough, push to good pair
		if (min_distance < 13.0f) {
			std::pair<Point2D, Point2D> pair;

			pair.first = Point2D(resource->pos.x, resource->pos.y);
			pair.second = closest_base;
			resource_base_pair.push_back(pair);
		}
	}

	// if no good pair, then return any location
	if (resource_base_pair.empty()) {
		rx = GetRandomScalar();
		ry = GetRandomScalar();
		build_location = Point2D(unit_to_build->pos.x + rx * 15.0f, unit_to_build->pos.y + ry * 15.0f);

		return build_location;
	}

	// stay in while until good building location is found
	while (true) {
		bool IsBad = false;
		rx = GetRandomScalar();
		ry = GetRandomScalar();
		build_location = Point2D(unit_to_build->pos.x + rx * 15.0f, unit_to_build->pos.y + ry * 15.0f);

		// for each resource base pair, if angles >= 90 then reselect point
		for (auto r_b : resource_base_pair) {
			Point2D vector_a, vector_b;
			float dot_product, n_n, cos;

			vector_a.x = build_location.x - r_b.second.x;
			vector_a.y = build_location.y - r_b.second.y;
			vector_b.x = build_location.x - r_b.first.x;
			vector_b.y = build_location.y - r_b.first.y;

			dot_product = Dot2D(vector_a, vector_b);
			n_n = sqrt(pow(vector_a.x, 2.0f) + pow(vector_a.y, 2.0f)) * sqrt(pow(vector_b.x, 2.0f) + pow(vector_b.y, 2.0f));
			cos = dot_product / n_n;

			if (cos < 0) {
				IsBad = true;
				break;
			}
		}

		if (!IsBad) {
			return build_location;
		}

	}

	return Point2D();

}



//**Try to build structure(s)
bool Bot::TryBuildStructure(ABILITY_ID ability_type_for_structure, size_t count = 1, UNIT_TYPEID unit_type = UNIT_TYPEID::TERRAN_SCV) {
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
			if (unit->orders.empty()){
				unit_to_build = unit;
			}else if (unit->orders.front().ability_id != ABILITY_ID::MOVE){
				unit_to_build = unit;
			}
		}
	}
	float rx = GetRandomScalar();
	float ry = GetRandomScalar();

	/*
		Actions()->UnitCommand(unit_to_build, ability_type_for_structure,
		Point2D(unit_to_build->pos.x + rx * 15.0f, unit_to_build->pos.y + ry * 15.0f));

	*/
	Actions()->UnitCommand(unit_to_build, ability_type_for_structure,
		GetGoodBuildingLocation(unit_to_build));

	/*
	*/

	return true;
}

//Expands to nearest location and updates the start location to be between the new location and old bases.
bool Bot::TryExpand(AbilityID build_ability, UnitTypeID worker_type) {
	const ObservationInterface* observation = Observation();
	float minimum_distance = std::numeric_limits<float>::max();
	Point3D closest_expansion;
	for (const auto& expansion : expansions_) {
		float current_distance = Distance2D(startLocation_, expansion);
		if (current_distance < .01f) {
			continue;
		}

		if (current_distance < minimum_distance) {
			if (Query()->Placement(build_ability, expansion)) {
				closest_expansion = expansion;
				minimum_distance = current_distance;
			}
		}
	}
	//only update staging location up till 3 bases.
	if (TryBuildStructure(build_ability, worker_type, closest_expansion, true) && observation->GetUnits(Unit::Self, IsTownHall()).size() < 3) {
		staging_location_ = Point3D(((staging_location_.x + closest_expansion.x) / 2), ((staging_location_.y + closest_expansion.y) / 2),
			((staging_location_.z + closest_expansion.z) / 2));
		return true;
	}
	return false;

}

//Tries to build a geyser for a base
bool Bot::TryBuildGas(AbilityID build_ability, UnitTypeID worker_type, Point2D base_location) {
	const ObservationInterface* observation = Observation();
	Units geysers = observation->GetUnits(Unit::Alliance::Neutral, IsVespeneGeyser());

	//only search within this radius
	float minimum_distance = 15.0f;
	Tag closestGeyser = 0;
	for (const auto& geyser : geysers) {
		float current_distance = Distance2D(base_location, geyser->pos);
		if (current_distance < minimum_distance) {
			if (Query()->Placement(build_ability, geyser->pos)) {
				minimum_distance = current_distance;
				closestGeyser = geyser->tag;
			}
		}
	}

	// In the case where there are no more available geysers nearby
	if (closestGeyser == 0) {
		return false;
	}
	return TryBuildStructure(build_ability, worker_type, closestGeyser);

}

bool Bot::TryBuildUnit(AbilityID ability_type_for_unit, UnitTypeID unit_type) {
	const ObservationInterface* observation = Observation();

	//If we are at supply cap, don't build anymore units, unless its an overlord.
	if (observation->GetFoodUsed() >= observation->GetFoodCap() && ability_type_for_unit != ABILITY_ID::TRAIN_OVERLORD) {
		return false;
	}
	const Unit* unit = nullptr;
	if (!GetRandomUnit(unit, observation, unit_type)) {
		return false;
	}
	if (!unit->orders.empty()) {
		return false;
	}

	if (unit->build_progress != 1) {
		return false;
	}

	Actions()->UnitCommand(unit, ability_type_for_unit);
	return true;
}

// Mine the nearest mineral to Town hall.
// If we don't do this, probes may mine from other patches if they stray too far from the base after building.
void Bot::MineIdleWorkers(const Unit* worker, AbilityID worker_gather_command, UnitTypeID vespene_building_type) {
	const ObservationInterface* observation = Observation();
	Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
	Units geysers = observation->GetUnits(Unit::Alliance::Self, IsUnit(vespene_building_type));

	const Unit* valid_mineral_patch = nullptr;

	if (bases.empty()) {
		return;
	}

	for (const auto& geyser : geysers) {
		if (geyser->assigned_harvesters < geyser->ideal_harvesters) {
			Actions()->UnitCommand(worker, worker_gather_command, geyser);
			return;
		}
	}
	//Search for a base that is missing workers.
	for (const auto& base : bases) {
		//If we have already mined out here skip the base.
		if (base->ideal_harvesters == 0 || base->build_progress != 1) {
			continue;
		}
		if (base->assigned_harvesters < base->ideal_harvesters) {
			valid_mineral_patch = FindNearestMineralPatch(base->pos);
			Actions()->UnitCommand(worker, worker_gather_command, valid_mineral_patch);
			return;
		}
	}

	if (!worker->orders.empty()) {
		return;
	}

	//If all workers are spots are filled just go to any base.
	const Unit* random_base = GetRandomEntry(bases);
	valid_mineral_patch = FindNearestMineralPatch(random_base->pos);
	Actions()->UnitCommand(worker, worker_gather_command, valid_mineral_patch);
}

//An estimate of how many workers we should have based on what buildings we have
int Bot::GetExpectedWorkers(UNIT_TYPEID vespene_building_type) {
	const ObservationInterface* observation = Observation();
	Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
	Units geysers = observation->GetUnits(Unit::Alliance::Self, IsUnit(vespene_building_type));
	int expected_workers = 0;
	for (const auto& base : bases) {
		if (base->build_progress != 1) {
			continue;
		}
		expected_workers += base->ideal_harvesters;
	}

	for (const auto& geyser : geysers) {
		if (geyser->vespene_contents > 0) {
			if (geyser->build_progress != 1) {
				continue;
			}
			expected_workers += geyser->ideal_harvesters;
		}
	}

	return expected_workers;
}

// To ensure that we do not over or under saturate any base.
void Bot::ManageWorkers(UNIT_TYPEID worker_type, AbilityID worker_gather_command, UNIT_TYPEID vespene_building_type) {
	const ObservationInterface* observation = Observation();
	Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
	Units geysers = observation->GetUnits(Unit::Alliance::Self, IsUnit(vespene_building_type));

	if (bases.empty()) {
		return;
	}

	for (const auto& base : bases) {
		//If we have already mined out or still building here skip the base.
		if (base->ideal_harvesters == 0 || base->build_progress != 1) {
			continue;
		}
		//if base is
		if (base->assigned_harvesters > base->ideal_harvesters) {
			Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(worker_type));

			for (const auto& worker : workers) {
				if (!worker->orders.empty()) {
					if (worker->orders.front().target_unit_tag == base->tag) {
						//This should allow them to be picked up by mineidleworkers()
						MineIdleWorkers(worker, worker_gather_command, vespene_building_type);
						return;
					}
				}
			}
		}
	}
	Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(worker_type));
	for (const auto& geyser : geysers) {
		if (geyser->ideal_harvesters == 0 || geyser->build_progress != 1) {
			continue;
		}
		if (geyser->assigned_harvesters > geyser->ideal_harvesters) {
			for (const auto& worker : workers) {
				if (!worker->orders.empty()) {
					if (worker->orders.front().target_unit_tag == geyser->tag) {
						//This should allow them to be picked up by mineidleworkers()
						MineIdleWorkers(worker, worker_gather_command, vespene_building_type);
						return;
					}
				}
			}
		}
		else if (geyser->assigned_harvesters < geyser->ideal_harvesters) {
			for (const auto& worker : workers) {
				if (!worker->orders.empty()) {
					//This should move a worker that isn't mining gas to gas
					const Unit* target = observation->GetUnit(worker->orders.front().target_unit_tag);
					if (target == nullptr) {
						continue;
					}
					if (target->unit_type != vespene_building_type) {
						//This should allow them to be picked up by mineidleworkers()
						MineIdleWorkers(worker, worker_gather_command, vespene_building_type);
						return;
					}
				}
			}
		}
	}
}

void Bot::RetreatWithUnits(UnitTypeID unit_type, Point2D retreat_position) {
	const ObservationInterface* observation = Observation();
	Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
	for (const auto& unit : units) {
		RetreatWithUnit(unit, retreat_position);
	}
}

void Bot::RetreatWithUnit(const Unit* unit, Point2D retreat_position) {
	float dist = Distance2D(unit->pos, retreat_position);

	if (dist < 10) {
		if (unit->orders.empty()) {
			return;
		}
		Actions()->UnitCommand(unit, ABILITY_ID::STOP);
		return;
	}

	if (unit->orders.empty() && dist > 14) {
		Actions()->UnitCommand(unit, ABILITY_ID::MOVE, retreat_position);
	}
	else if (!unit->orders.empty() && dist > 14) {
		if (unit->orders.front().ability_id != ABILITY_ID::MOVE) {
			Actions()->UnitCommand(unit, ABILITY_ID::MOVE, retreat_position);
		}
	}
}

void Bot::OnNuclearLaunchDetected() {
	const ObservationInterface* observation = Observation();
	nuke_detected = true;
	nuke_detected_frame = observation->GetGameLoop();
}