#include <iostream>
#include <cstdlib>
#include <algorithm>

#include "Feeder.h"
#include "sc2api/sc2_api.h"
#include "sc2api/sc2_types.h"

using namespace sc2;

void Feeder::hitAndRun(const Unit *unit, Units medi, const ObservationInterface *ob, const float range) {
	// If unit is attacking
	const Unit *enemy;
	IsArmy army_test(ob);
	Point2D enemy_pos, back;
	float health_ratio;

	//std::cerr << "enter h%r" << std::endl;

	if (unit->engaged_target_tag == 0) { return; }

	enemy = ob->GetUnit(unit->engaged_target_tag);

	if (enemy == nullptr) { return; }

	if (!enemy->is_alive) { return; }

	if (!army_test(*enemy)) { return; }

	back.x = unit->pos.x - enemy->pos.x + unit->pos.x;
	back.y = unit->pos.y - enemy->pos.y + unit->pos.y;

	health_ratio = (unit->health / unit->health_max);
	if (health_ratio >= 0.7f)
	{
		return;
	}
	else if (health_ratio < 0.7f && ( true ) && (!medi.empty()) ) {// medivec aid here
		Tag current_tag;

		//std::cout << "enter elif" << std::endl;
		current_tag = unit->tag;
		if (!prev.empty()){
			//previousObserve.
			//std::cout << "preO is not empty" << std::endl;
			auto it = std::find_if(prev.begin(), prev.end(), [&current_tag](const std::pair<Tag, float>& old_pair) {return old_pair.first == current_tag;});

			if (it != prev.end()) // find old
			{
				if (unit->tag == (*it).first)
				{
					//std::cout << unit->health << " | " << (*it).second << std::endl;
				}
				if (unit->health < (*it).second) {
					// TODO find closest medivec later, instead of front()
					Actions()->UnitCommand(medi.front(), ABILITY_ID::LOAD, unit);
					Actions()->UnitCommand(medi.front(), ABILITY_ID::UNLOADALLAT, back, true);
					
				}
			}
		}
	}
	else {
		Tag current_tag;

		//std::cout << "enter elif" << std::endl;
		current_tag = unit->tag;
		if (!prev.empty()) {
			//previousObserve.
			//std::cout << "preO is not empty" << std::endl;
			auto it = std::find_if(prev.begin(), prev.end(), [&current_tag](const std::pair<Tag, float>& old_pair) {return old_pair.first == current_tag;});

			if (it != prev.end()) // find old
			{
				if (unit->tag == (*it).first)
				{
					//std::cout << unit->health << " | " << (*it).second << std::endl;
				}
				if (unit->health < (*it).second) {
					// TODO find closest medivec later, instead of front()
					Actions()->UnitCommand(unit, ABILITY_ID::MOVE, back);
					AttackWithUnit(unit, ob);

				}
			}
		}

	}
}
