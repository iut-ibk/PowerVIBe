/**
 * @file
 * @author  Chrisitan Urich <christian.urich@gmail.com>
 * @version 1.0
 * @section LICENSE
 *
 * This file is part of DynaMind
 *
 * Copyright (C) 2012  Christian Urich

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

#include "placehouseholds.h"

DM_DECLARE_NODE_NAME(PlaceHouseholds, PowerVIBe)

int PlaceHouseholds::sumHouseholds(int (&hh)[5])
{
	int sum = 0;
	for(int i = 0; i < 5; i++)
		sum+=hh[i];
	return sum;
}

int PlaceHouseholds::chooseHousehold(int (&hh)[5])
{
	std::vector<int> vChoose;
	for (int i = 0; i < 5; i++) {
		if (hh > 0)
			vChoose.push_back(i);
	}

	if (vChoose.size() == 0)
		return -1;

	int chooser= rand() % vChoose.size();

	return vChoose[chooser];
}

int PlaceHouseholds::chooseBuilding(std::vector<int> &buildings)
{
	std::vector<int> vChoose;
	for (unsigned int i = 0; i < buildings.size(); i++){
		if (buildings[i] > 0)
			vChoose.push_back(i);
	}
	if (vChoose.size() == 0)
		return -1;
	int chooser= rand() % vChoose.size();
	return vChoose[chooser];
}

PlaceHouseholds::PlaceHouseholds()
{
	grids = DM::View("GRID", DM::FACE, DM::READ);
	grids.getAttribute("hh01");
	grids.getAttribute("hh02");
	grids.getAttribute("hh03");
	grids.getAttribute("hh04");
	grids.getAttribute("hh05");
	grids.getAttribute("BUILDING");

	buildings = DM::View("BUILDING",  DM::FACE, DM::READ);
	buildings.getAttribute("residential_units");



	households = DM::View("HOUSEHOLD",  DM::COMPONENT ,DM::WRITE);
	households.addAttribute("id");
	households.addAttribute("income");
	households.addAttribute("age_of_head");
	households.addAttribute("race_id");
	households.addAttribute("children");
	households.addAttribute("cars");




	households.addLinks("BUILDING", buildings);
	buildings.addLinks("HOUSEHOLD", households);


	persons = DM::View("PERSON", DM::COMPONENT ,DM::WRITE);
	persons.addAttribute("id");
	persons.addLinks("HOUSEHOLD", households);
	households.addLinks("PERSON", persons);

	std::vector<DM::View> dataset;
	dataset.push_back(grids);
	dataset.push_back(buildings);
	dataset.push_back(households);
	dataset.push_back(persons);

	this->addData("City", dataset);
}

void PlaceHouseholds::run()
{
	DM::System * city = this->getData("City");

	std::vector<std::string> gridUuids = city->getUUIDsOfComponentsInView(grids);


	int household_id = 0;
	int person_id = 0;
	foreach (std::string gUuid, gridUuids) {
		DM::Component * grid = city->getComponent(gUuid);

		int hh[5];
		hh[0] = (int) grid->getAttribute("hh01")->getDouble();
		hh[1] = (int) grid->getAttribute("hh02")->getDouble();
		hh[2] = (int) grid->getAttribute("hh03")->getDouble();
		hh[3] = (int) grid->getAttribute("hh04")->getDouble();
		hh[4] = (int) grid->getAttribute("hh05")->getDouble();

		//Get Buildings
		Attribute  * attrBuildings = grid->getAttribute("BUILDING");
		std::vector<LinkAttribute> lBuildings = attrBuildings->getLinks();
		std::vector<int> avalibleUnits;
		std::vector<std::string> buildingUUIDs;
		foreach (LinkAttribute lB, lBuildings) {
			DM::Component * b = city->getComponent(lB.uuid);
			int runits = (int) b->getAttribute("residential_units")->getDouble();
			if (runits>0) {
				avalibleUnits.push_back(runits);
				buildingUUIDs.push_back(lB.uuid);
			}
		}


		while (this->sumHouseholds(hh) > 0) {
			household_id++;
			int chooser = this->chooseHousehold(hh);
			int chooser_building = this->chooseBuilding(avalibleUnits);

			if (chooser == -1 || chooser_building == -1)
				break;
			//Create Household
			Component * h = new Component();
			h->addAttribute("id", household_id);
			h->addAttribute("income", 1000);
			h->addAttribute("age_of_head", 50);
			h->addAttribute("race_id", 1);
			int children = 0;
			if (chooser+1 > 2)
				children = 1;
			h->addAttribute("children", children);
			h->addAttribute("cars", children);

			//Link Household - Building
			Attribute linkB("BUILDING");
			linkB.setLink(buildings.getName(), buildingUUIDs[chooser_building]);
			h->addAttribute(linkB);

			//Link Building Household
			Component * building = city->getComponent( buildingUUIDs[chooser_building]);
			Attribute * bh = building->getAttribute("HOUSEHOLD");
			bh->setLink(households.getName(), h->getUUID());

			city->addComponent(h, households);

			//Create Persons
			for(int i = 0; i < chooser+1; i++) {
				person_id++;
				Component * p = new Component();
				p->addAttribute("id", person_id);

				//Link Person - Houshold
				Attribute l("HOUSEHOLD");
				l.setLink(households.getName(), h->getUUID());
				p->addAttribute(l);

				//Link Household - Person
				Attribute * hp = h->getAttribute("PERSON");
				hp->setLink(persons.getName(), p->getUUID());

				city->addComponent(p, persons);
			}

			//Remove Reidential Unit
			avalibleUnits[chooser_building] = avalibleUnits[chooser_building]-1;

			//Remove Household
			hh[chooser] = hh[chooser]-1;
		}

	}
	Logger(Debug) << "Created Households " << household_id;
	Logger(Debug) << "Created Persons " << person_id;
}

string PlaceHouseholds::getHelpUrl()
{
	return "https://docs.google.com/document/pub?id=1xEXR-Jd82a496RVrfw5nNz6ZnFN-2EwZ-u0U3FCY7ns";
}

