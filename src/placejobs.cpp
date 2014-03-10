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


#include "placejobs.h"

DM_DECLARE_NODE_NAME(PlaceJobs, PowerVIBe)

PlaceJobs::PlaceJobs()
{
	grids = DM::View("GRID", DM::FACE, DM::READ);
	grids.addAttribute("jobs", "BUILDING", DM::READ);
	grids.addAttribute("BUILDING", "BUILDING", DM::READ);

	buildings = DM::View("BUILDING",  DM::FACE, DM::READ);
	buildings.addAttribute("non_residential_sqft", DM::Attribute::DOUBLE, DM::WRITE);

	households = DM::View("HOUSEHOLD", DM::COMPONENT, DM::READ);
	households.addAttribute("workers", DM::Attribute::DOUBLE, DM::WRITE);
	households.addAttribute("PERSON", "PERSON", DM::READ);

	persons = DM::View("PERSON", DM::COMPONENT, DM::READ);
	persons.addAttribute("id", DM::Attribute::DOUBLE, DM::READ);

	jobs = DM::View("JOB",  DM::COMPONENT ,DM::WRITE);
	jobs.addAttribute("id", DM::Attribute::DOUBLE, DM::WRITE);
	jobs.addAttribute("home_based_status", DM::Attribute::DOUBLE, DM::WRITE);
	jobs.addAttribute("sector_id", DM::Attribute::DOUBLE, DM::WRITE);
	jobs.addAttribute("building_type", DM::Attribute::DOUBLE, DM::WRITE);

	jobs.addAttribute("BUILDING", buildings.getName(), DM::WRITE);
	buildings.addAttribute("JOB", jobs.getName(), DM::WRITE);

	std::vector<DM::View> dataset;
	dataset.push_back(grids);
	dataset.push_back(buildings);
	dataset.push_back(jobs);
	dataset.push_back(households);
	dataset.push_back(persons);

	this->addData("City", dataset);
}

void PlaceJobs::run() {
	DM::System * city = this->getData("City");

	std::vector<Component*> personCmps = city->getAllComponentsInView(persons);
	std::vector<int> person_rand_int_id;
	for (unsigned int i = 0; i < personCmps.size(); i++) {
		person_rand_int_id.push_back(i);
	}
	for (unsigned int i = 0; i < personCmps.size(); i++) {
		int index1 =  rand() % person_rand_int_id.size();
		int index2 =  rand() % person_rand_int_id.size();
		int p_id_tmp = person_rand_int_id[index1];
		person_rand_int_id[index1] = person_rand_int_id[index2];
		person_rand_int_id[index2] = p_id_tmp;

	}



	std::vector<Component*> gridCmps = city->getAllComponentsInView(grids);


	int job_id = 0;
	foreach(DM::Component * grid, gridCmps) 
	{
		double jobsN = grid->getAttribute("jobs")->getDouble();

		if (jobsN < 0.01)
			continue;

		//Calcualte Total Area
		Attribute  * attrBuildings = grid->getAttribute("BUILDING");
		double building_footprints_tot = 0;

		std::vector<Component*> linkedBuildings = attrBuildings->getLinkedComponents();
		foreach(DM::Component* b, linkedBuildings)
			building_footprints_tot += b->getAttribute("area")->getDouble();

		if (building_footprints_tot < 0.01)
			continue;

		//Create Jobs
		double JobperSqft = jobsN/building_footprints_tot;

		foreach(DM::Component* b, linkedBuildings)
		{
			double area = b->getAttribute("area")->getDouble();

			int jobsInBuilding = (int) (area*JobperSqft + 0.5);
			for (int i = 0; i < jobsInBuilding; i++) 
			{
				job_id++;
				DM::Component * job = new DM::Component();

				//Person ID
				DM::Component * person = personCmps[person_rand_int_id[job_id]];
				person->addAttribute("job_id", job_id);

				DM::Component * households = person->getAttribute("HOUSEHOLD")->getLinkedComponents()[0];
				households->addAttribute("workers", households->getAttribute("workers")->getDouble()+1);
				job->addAttribute("id", job_id);
				job->addAttribute("home_based_status", 0);
				job->addAttribute("sector_id", 1);
				job->addAttribute("building_type", 1);

				Attribute lAttr("BUILDING");
				lAttr.addLink(b, "BUILDING");
				job->addAttribute(lAttr);
				city->addComponent(job, jobs);
				Attribute * lBAttr = b->getAttribute("JOB");
				lBAttr->addLink(job, "JOB");

			}
			b->addAttribute("non_residential_sqft", 20*jobsInBuilding*1.1);
		}
	}
}

std::string PlaceJobs::getHelpUrl() {
	return "https://docs.google.com/document/pub?id=1I_XAdzd0bMZJRbRfth59EYVU0OO8j61K3CRrjRyzg54";
}
