#include <iostream>
#include <cstdlib>
#include <math.h>

#include "Feeder.h"
#include "sc2api/sc2_api.h"
#include "sc2api/sc2_types.h"

using namespace sc2;

Point2D Feeder::GetGoodBuildingLocation() {
	const ObservationInterface* observation = Observation();
	float rx, ry;
	Point2D build_location;
	bool IsGoodLocation = false;
	std::vector<std::pair<Point2D, Point2D> > resource_base_pair;

	Units rocks = observation->GetUnits(
		[](const Unit& unit) {
		return unit.unit_type == 472;
	}
	);

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

	Units command_centers = observation->GetUnits(
		[](const Unit& unit) {
		return (unit.unit_type == UNIT_TYPEID::TERRAN_COMMANDCENTER || unit.unit_type == UNIT_TYPEID::TERRAN_PLANETARYFORTRESS ||
			unit.unit_type == UNIT_TYPEID::TERRAN_ORBITALCOMMAND) && unit.alliance == Unit::Alliance::Self;
	}
	);

	if (command_centers.empty()) {
		std::cerr << "fail to find command center" << std::endl;
		rx = GetRandomScalar();
		ry = GetRandomScalar();
		build_location = Point2D(staging_location_.x + rx * 15, staging_location_.y + ry * 15);

		return build_location;
	}

	for (auto resource : resources) {
		Point2D closest_base;
		float min_distance;

		closest_base.x = command_centers.front()->pos.x;
		closest_base.y = command_centers.front()->pos.y;

		min_distance = Distance2D(closest_base, Point2D(resource->pos.x, resource->pos.y));

		std::cerr << "min_distance: " << min_distance << std::endl;
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
		if (min_distance < 30.0f) {
			std::pair<Point2D, Point2D> pair;

			pair.first = Point2D(resource->pos.x, resource->pos.y);
			pair.second = closest_base;
			resource_base_pair.push_back(pair);
		}
	}

	if (resource_base_pair.empty()) {
		rx = GetRandomScalar();
		ry = GetRandomScalar();
		build_location = Point2D(staging_location_.x + rx * 15, staging_location_.y + ry * 15);

		return build_location;
	}


	while (!IsGoodLocation) {
		bool IsBad = false;
		rx = GetRandomScalar();
		ry = GetRandomScalar();
		build_location = Point2D(staging_location_.x + rx * 15, staging_location_.y + ry * 15);

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

void Feeder::GetAllEnemyBaseLocation(std::vector<Point2D> &rtv) {
	const ObservationInterface* observation = Observation();

	Point2D my_start_pos, min_p, max_p;
	Point3D temp_pos = observation->GetStartLocation();
	GameInfo game_info;

	my_start_pos.x = temp_pos.x;
	my_start_pos.y = temp_pos.y;
	game_info = observation->GetGameInfo();
	min_p = game_info.playable_min;
	max_p = game_info.playable_max;

	rtv.push_back(Point2D(my_start_pos.x, max_p.y - my_start_pos.y + min_p.y));
	rtv.push_back(Point2D(max_p.x - my_start_pos.x + min_p.x, max_p.y - my_start_pos.y + min_p.y));
	rtv.push_back(Point2D(max_p.x - my_start_pos.x + min_p.x, my_start_pos.y));
	rtv.push_back(Point2D(my_start_pos.x, my_start_pos.y));

}


void Feeder::GetNeareastBaseLocation(Point2D &point) {
	std::vector<Point2D> locations;
	float min_distance;
	Point2D min_point;

	GetAllEnemyBaseLocation(locations);
	min_point = locations.front();
	min_distance = Distance2D(point, min_point);
	for (const Point2D &loca : locations) {
		float temp = Distance2D(loca, loca); // TODO what happened here? it looks wrong

		if (temp < min_distance) {
			min_distance = temp;
			min_point = loca;
		}
	}

	enemy_base_point = min_point;
}


void Feeder::GetRallyPointOnRocks() {
	// find closest rocks by gameinfo.startlocation & getallunits // RALLY_UNITS, TERRAN_BARRACKS, 
	const ObservationInterface* observation = Observation();
	float min_distance;
	Point2D min_point, temp_p;
	Point3D start_pos = observation->GetStartLocation();
	float x_, y_;
	float temp_d;

	// TODO getUnits bug, unknown types for neutral rocks
	Units rocks = observation->GetUnits(
		[](const Unit& unit) {
		return unit.unit_type == 472;
	}
	);

	if (rocks.empty()) {
		std::cerr << "Error finding rocks" << std::endl;
		return;
	}

	// init before loop
	min_point = Point2D(rocks.front()->pos.x, rocks.front()->pos.y);
	min_distance = Distance2D(min_point, start_pos);

	// TODO try replacing auto
	for (auto rock : rocks) {
		temp_p.x = rock->pos.x;
		temp_p.y = rock->pos.y;
		temp_d = Distance2D(temp_p, start_pos);

		if (temp_d < min_distance) {
			min_point = temp_p;
			min_distance = temp_d;
		}
	}

	x_ = (min_point.x - start_pos.x) / 4.5f;
	y_ = (min_point.y - start_pos.y) / 4.5f;

	if (y_ > 0 && (abs(x_) < abs(y_))) { // bottom left
		x_ = min_point.x - x_ - 2;
		y_ = min_point.y - y_;
	}
	else if (y_ < 0 && (abs(x_) < abs(y_))) { // top right
		x_ = min_point.x - x_ + 2;
		y_ = min_point.y - y_;
	}
	else if (x_ < 0 && (abs(x_) > abs(y_))) { // top left
		x_ = min_point.x - x_;
		y_ = min_point.y - y_ - 2;
	}
	else if (x_ > 0 && (abs(x_) > abs(y_))) { // bottom right
		x_ = min_point.x - x_;
		y_ = min_point.y - y_ + 2;
	}


	rally_point = Point2D(x_, y_);

}
