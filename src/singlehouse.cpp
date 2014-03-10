/**
 * @file
 * @author  Chrisitan Urich <christian.urich@gmail.com>
 * @version 1.0
 * @section LICENSE
 *
 * This file is part of DynaMind
 *
 * Copyright (C) 2011-2012   Christian Urich

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "singlehouse.h"
#include "dmgeometry.h"
#include "cgalgeometry.h"
#include "tbvectordata.h"
#include <QPolygonF>
#include <QTransform>
#include <dmhelper.h>
#include <tbvectordata.h>
#include <littlegeometryhelpers.h>

DM_DECLARE_NODE_NAME(SingleHouse, TestModules)
SingleHouse::SingleHouse()
{
	houses = DM::View("BUILDING", DM::COMPONENT, DM::WRITE);
	houses.addAttribute("built_year", DM::Attribute::DOUBLE, DM::WRITE);
	houses.addAttribute("stories", DM::Attribute::DOUBLE, DM::WRITE);
	houses.addAttribute("stories_below", DM::Attribute::DOUBLE, DM::WRITE); //cellar counts as story
	houses.addAttribute("stories_height", DM::Attribute::DOUBLE, DM::WRITE);

	houses.addAttribute("floor_area", DM::Attribute::DOUBLE, DM::WRITE);
	houses.addAttribute("gross_floor_area", DM::Attribute::DOUBLE, DM::WRITE);

	houses.addAttribute("area_walls_outside", DM::Attribute::DOUBLE, DM::WRITE);
	houses.addAttribute("area_windows", DM::Attribute::DOUBLE, DM::WRITE);

	houses.addAttribute("centroid_x", DM::Attribute::DOUBLE, DM::WRITE);
	houses.addAttribute("centroid_y", DM::Attribute::DOUBLE, DM::WRITE);

	houses.addAttribute("l_bounding", DM::Attribute::DOUBLE, DM::WRITE);
	houses.addAttribute("b_bounding", DM::Attribute::DOUBLE, DM::WRITE);
	houses.addAttribute("h_bounding", DM::Attribute::DOUBLE, DM::WRITE);

	houses.addAttribute("alpha_bounding", DM::Attribute::DOUBLE, DM::WRITE);

	houses.addAttribute("alpha_roof", DM::Attribute::DOUBLE, DM::WRITE);

	houses.addAttribute("cellar_used", DM::Attribute::DOUBLE, DM::WRITE);
	houses.addAttribute("roof_used", DM::Attribute::DOUBLE, DM::WRITE);

	houses.addAttribute("T_heating", DM::Attribute::DOUBLE, DM::WRITE);
	houses.addAttribute("T_cooling", DM::Attribute::DOUBLE, DM::WRITE);

	houses.addAttribute("Geometry", "Geometry", DM::WRITE);
	houses.addAttribute("V_living", DM::Attribute::DOUBLE, DM::WRITE);

	footprint = DM::View("Footprint", DM::FACE, DM::WRITE);

	building_model = DM::View("Geometry", DM::FACE, DM::WRITE);
	building_model.addAttribute("type", DM::Attribute::STRING, DM::WRITE);

	std::vector<DM::View> data;
	data.push_back(houses);
	data.push_back(footprint);
	data.push_back(building_model);
	data.push_back(DM::View("dummy", DM::SUBSYSTEM, READ));
	heatingT = 20;
	buildyear = 1985;
	stories = 1;
	l = 16;
	b = 10;
	alpha = 30;

	this->addParameter("l", DM::DOUBLE, &l);
	this->addParameter("b", DM::DOUBLE, &b);
	this->addParameter("stories", DM::INT, &stories);
	this->addParameter("alhpa", DM::DOUBLE, &alpha);
	this->addParameter("built_year", DM::INT, &buildyear);

	this->addParameter("T_heating", DM::DOUBLE, &heatingT);
	this->addParameter("T_cooling", DM::DOUBLE, &heatingT);
	this->addData("city", data);




}

void SingleHouse::run()
{

	DM::System * city = this->getData("city");


	QPointF f1 ( -l/2,  - b/2);
	QPointF f2 (l/2, -b/2);
	QPointF f3 (l/2,  b/2);
	QPointF f4 (-l/2, b/2);

	double angle = 0;
	QPolygonF original = QPolygonF() << f1 << f2 << f3 << f4;
	QTransform transform = QTransform().rotate(angle);
	QPolygonF rotated = transform.map(original);

	std::vector<DM::Node * > houseNodes;



	foreach (QPointF p, rotated) {
		houseNodes.push_back(city->addNode(p.x(), p.y(), 0));

	}
	if (houseNodes.size() < 2) {
		Logger(Error) << "Can't create House";

	}
	houseNodes.push_back(houseNodes[0]);

	DM::Component * building = city->addFace(houseNodes, houses);

	//Create Building and Footprints
	DM::Face * foot_print = city->addFace(houseNodes, footprint);
	//Link Building with Footprint
	DMHelper::LinkComponents(houses, building, footprint, foot_print);

	Node  n = DM::CGALGeometry::CalculateCentroid(city, foot_print);
	building->addAttribute("type", "single_family_house");
	building->addAttribute("built_year", buildyear);
	building->addAttribute("stories", stories);
	building->addAttribute("stories_below", 0); //cellar counts as story
	building->addAttribute("stories_height",3 );

	building->addAttribute("floor_area", l*b);
	building->addAttribute("gross_floor_area", l * b * stories);

	building->addAttribute("centroid_x", n.getX());
	building->addAttribute("centroid_y", n.getY());

	building->addAttribute("l_bounding", l);
	building->addAttribute("b_bounding", b);
	building->addAttribute("h_bounding", stories * 3);

	building->addAttribute("alpha_bounding", angle);

	building->addAttribute("alpha_roof", 0);

	building->addAttribute("cellar_used", 1);
	building->addAttribute("roof_used", 0);


	building->addAttribute("T_heating", heatingT);
	building->addAttribute("T_cooling", 26);

	building->addAttribute("V_living", l*b*stories * 3);

	LittleGeometryHelpers::CreateStandardBuilding(city, houses, building_model, building, houseNodes, stories);
	if (alpha > 10) {
		LittleGeometryHelpers::CreateRoofRectangle(city, houses, building_model, building, houseNodes, stories*3, alpha);
	}

	double windows_a = 0;
	double wall_outside_a = 0;
	foreach(DM::Component* c, building->getAttribute("Geometry")->getLinkedComponents()) {
		DM::Face * f = (DM::Face*)c;
		double area = TBVectorData::CalculateArea(city, f);
		if (f->getAttribute("type")->getString() == "window") {
			windows_a+=area;
		}
		if (f->getAttribute("type")->getString()== "wall_outside") {
			wall_outside_a+=area;
		}
	}

	building->addAttribute("area_walls_outside", wall_outside_a);
	building->addAttribute("area_windows", windows_a);


}
