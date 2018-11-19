#include <iostream>
#include <cstdlib>

#include "Feeder.h"
#include "sc2api/sc2_api.h"
#include "sc2api/sc2_types.h"

using namespace sc2;

void Feeder::microControl() {
	const ObservationInterface* observation = Observation();
	
	Point2D mp, zp;

	//std::cout << backup_target_.x << "!" << backup_target_.y << std::endl;


	if (!UpdatePosition(unit_type_container, Unit::Alliance::Self, mp)) {
		return;
	}

	if (!UpdateEnemyPosition(unit_filter, Unit::Alliance::Enemy, zp)) {

		return;
	}

	if (!GetNearestEnemy(unit_filter, mp)) {

		return;
	}


	Units units = observation->GetUnits(Unit::Alliance::Self);
	for (const auto& u : units) {

		//std::cout << backup_target_.x << " " << backup_target_.y << std::endl;

		switch (static_cast<UNIT_TYPEID>(u->unit_type)) {

		case UNIT_TYPEID::TERRAN_MARINE: {

			if (!move_back_) {
				//std::cout << "true"<< std::endl;

				//std::cout << "target:" << targeted_enemy_->pos.x << " " << targeted_enemy_->pos.y << std::endl;
				Actions()->UnitCommand(u, ABILITY_ID::ATTACK, targeted_enemy_);
			}
			else {

				std::cout << "back:" << backup_target_.x << "!" << backup_target_.y << std::endl;
				std::cout << "mp:" << mp.x << "!" << mp.y << std::endl;
				if (Distance2D(mp, backup_target_) < 1.5f) {

					move_back_ = false;
				}

				Actions()->UnitCommand(u, ABILITY_ID::SMART, backup_target_);
			}
			break;
		}
		default: {
			break;
		}
		}
	}

}

void Feeder::resetMicroControl(const Unit* unit) {
	if (unit->alliance == Unit::Alliance::Enemy) {
		Point2D mp, zp;

		if (!UpdatePosition(unit_type_container, Unit::Alliance::Self, mp)) {
			return;
		}

		if (!UpdatePosition(unit_type_container, Unit::Alliance::Enemy, zp)) {
			return;
		}

		Vector2D diff = mp - zp;
		Normalize2D(diff);

		targeted_enemy_ = 0;
		move_back_ = true;
		backup_start_ = mp;
		backup_target_ = mp + diff * 3.0f;
	}

}

// modified from microbot.cpp in example
// find average position of my units, result stores in position
bool Feeder::UpdatePosition(std::set<UNIT_TYPEID> unit_type_container, Unit::Alliance alliace, Point2D& position) {
	const ObservationInterface* observation = Observation();
	Units units = observation->GetUnits(alliace);

	if (units.empty()) {
		return false;
	}

	position = Point2D(0.0f, 0.0f);
	unsigned int count = 0;

	for (const auto& u : units) {
		if (unit_type_container.find(u->unit_type) != unit_type_container.end()) {
			position += u->pos;
			++count;
		}
	}

	if (!count) {
		return false;
	}
	position /= (float)count;
	//std::cout << position.x << " " << position.y << std::endl;
	//std::cout << count << std::endl;

	return true;
}

bool Feeder::UpdateEnemyPosition(std::set<UNIT_TYPEID> unit_filter, Unit::Alliance enemy, Point2D& position) {
	const ObservationInterface* observation = Observation();
	Units units = observation->GetUnits(enemy);

	if (units.empty()) {
		return false;
	}

	position = Point2D(0.0f, 0.0f);
	unsigned int count = 0;

	for (const auto& u : units) {
		if (true) {
		//if (unit_filter.find(u->unit_type) == unit_filter.end()) {
			position += u->pos;
			++count;
		}
	}

	if (!count) {
		return false;
	}
	position /= (float)count;
	//std::cout << position.x << " " << position.y << std::endl;
	//std::cout << count << std::endl;

	return true;
}


bool Feeder::GetNearestEnemy(std::set<UNIT_TYPEID> unit_filter, const Point2D& from) {
	const ObservationInterface* observation = Observation();
	Units units = observation->GetUnits(Unit::Alliance::Enemy);

	if (units.empty()) {
		return false;
	}

	float distance = std::numeric_limits<float>::max();
	for (const auto& u : units) {
		if (unit_filter.find(u->unit_type) == unit_filter.end()) {
			float d = DistanceSquared2D(u->pos, from);
			if (d < distance) {
				distance = d;
				targeted_enemy_ = u;
			}
		}
	}
	//std::cout << targeted_enemy_->pos.x << std::endl;

	return true;
}

