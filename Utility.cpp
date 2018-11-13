#include <iostream>
#include <cstdlib>

#include "Feeder.h"
#include "sc2api/sc2_api.h"
#include "sc2api/sc2_types.h"

using namespace sc2;

Point2D Feeder::GetGoodBuildingLocation() {
	const ObservationInterface* observation = Observation();
	float rx, ry;
	Point2D build_location;
	bool IsGoodLocation = false;

	Units rocks = observation->GetUnits(
		[](const Unit& unit) {
		return unit.unit_type == 472;
	}
	);

	rx = GetRandomScalar();
	ry = GetRandomScalar();

	while (!IsGoodLocation) {
		build_location = Point2D(staging_location_.x + rx * 15, staging_location_.y + ry * 15);

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
