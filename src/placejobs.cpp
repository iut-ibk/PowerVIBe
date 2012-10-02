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
    grids.getAttribute("jobs");
    grids.getAttribute("BUILDING");

    buildings = DM::View("BUILDING",  DM::FACE, DM::READ);
    buildings.addAttribute("non_residential_sqft");



    jobs = DM::View("JOB",  DM::COMPONENT ,DM::WRITE);
    jobs.addAttribute("id");
    jobs.addAttribute("home_based_status");
    jobs.addAttribute("sector_id");


    jobs.addLinks("BUILDING", buildings);
    buildings.addLinks("JOB", jobs);


    std::vector<DM::View> dataset;
    dataset.push_back(grids);
    dataset.push_back(buildings);
    dataset.push_back(jobs);

    this->addData("City", dataset);
}

void PlaceJobs::run() {
    DM::System * city = this->getData("City");

    std::vector<std::string> gridUuids = city->getUUIDsOfComponentsInView(grids);


    int job_id = 0;
    foreach (std::string gUuid, gridUuids) {
        DM::Component * grid = city->getComponent(gUuid);
        double jobsN = grid->getAttribute("jobs")->getDouble();

        if (jobsN < 0.01)
            continue;

        //Calcualte Total Area
        Attribute  * attrBuildings = grid->getAttribute("BUILDING");
        std::vector<LinkAttribute> lBuildings = attrBuildings->getLinks();
        double building_footprints_tot = 0;
        foreach (LinkAttribute lB, lBuildings) {
            DM::Component * b = city->getComponent(lB.uuid);
            building_footprints_tot += b->getAttribute("area")->getDouble();

        }

        if (building_footprints_tot < 0.01)
            continue;

        //Create Jobs
        double JobperSqft = jobsN/building_footprints_tot;


        foreach (LinkAttribute lB, lBuildings) {
            DM::Component * b = city->getComponent(lB.uuid);
            double area = b->getAttribute("area")->getDouble();

            int jobsInBuilding = (int) (area*JobperSqft + 0.5);
            for (int i = 0; i < jobsInBuilding; i++) {
                job_id++;
                DM::Component * job = new DM::Component();

                job->addAttribute("id", job_id);
                job->addAttribute("home_based_status", 0);
                job->addAttribute("sector_id", 1);
                Attribute lAttr("BUILDING");
                lAttr.setLink("BUILDING", b->getUUID());
                job->addAttribute(lAttr);
                city->addComponent(job, jobs);
                Attribute * lBAttr = b->getAttribute("JOB");
                lBAttr->setLink("JOB",job->getUUID());

            }
            b->addAttribute("non_residential_sqft", 25*jobsInBuilding*1.1);
        }
    }
}

std::string PlaceJobs::getHelpUrl() {
    return "https://docs.google.com/document/pub?id=1I_XAdzd0bMZJRbRfth59EYVU0OO8j61K3CRrjRyzg54";
}
