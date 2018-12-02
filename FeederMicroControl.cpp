#include <iostream>
#include <cstdlib>

#include "Feeder.h"
#include "sc2api/sc2_api.h"
#include "sc2api/sc2_types.h"

using namespace sc2;

void Feeder::hitAndRun(const Unit *unit, const ObservationInterface *ob, const float range) {
	// If unit is attacking
	const Unit *enemy;
	Point2D enemy_pos;

	//std::cerr << "enter h%r" << std::endl;
	
	if ((unit->health / unit->health_max) >= 0.7f)
	{
		return;
	}
	
	/*
	if (unit->orders.empty()) {
		return;
	}

	if (unit->orders.front().ability_id != ABILITY_ID::ATTACK) // no enemy
	{
		return;
	}
	*/

	if (unit->engaged_target_tag == 0)
	{
		//std::cerr << "enemy is null" << std::endl;
		return;
	}

	
	std::cerr << unit->engaged_target_tag << std::endl;
	enemy = ob->GetUnit(unit->engaged_target_tag);

	if (enemy == nullptr)
	{
		return;
	}

	if (!enemy->is_alive) {
		std::cerr << "dead" << std::endl;

		return;
	}
	//enemy_pos = unit->orders.front().target_pos;
	//return;
	//all_type_data = ob->GetUnitTypeData(false);
	
	//if (Distance2D(enemy->pos, unit->pos) <= all_type_data[unit->unit_type].weapons.front().range) // enemy in attack range
	
	Point2D back;

	//back.x = 2 * unit->pos.x - enemy_pos.x;
	//back.y = 2 * unit->pos.y - enemy_pos.y;

	back.x = 2 * unit->pos.x - enemy->pos.x;
	back.y = 2 * unit->pos.y - enemy->pos.y;



	if (unit->is_alive)
	{
		std::cerr << "moveback h%r" << std::endl;
		Actions()->UnitCommand(unit, ABILITY_ID::MOVE, back);
	}

	
}
