/**
 * @file
 * @author  Chrisitan Urich <christian.urich@gmail.com>
 * @version 1.0
 * @section LICENSE
 *
 * This file is part of PowerVIBe
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
#include "heatingdemand.h"

DM_DECLARE_NODE_NAME(HeatingDemand, PowerVIBe)

HeatingDemand::HeatingDemand()
{
    buildings = DM::View("BUILDING", DM::COMPONENT, DM::READ);
    buildings.getAttribute("floor_area");
    buildings.getAttribute("built_year");
    buildings.addAttribute("heating_demand");
    
    std::vector<DM::View> data;
    data.push_back(buildings);
    
    this->addData("City", data);
}

void HeatingDemand::run()
{
    DM::System * city = this->getData("City");
    std::vector<std::string> buildingUUIDs = city->getUUIDs(buildings);
    foreach (std::string uuid, buildingUUIDs){
        DM::Component * building = city->getComponent(uuid);
        double area = city->getAttribute("floor_area")->getDouble();
        int year = (int) city->getAttribute("heating_demand")->getDouble();
        building->addAttribute("heating_demand", calculateHeatingDemand(area, year));
    }
}

double HeatingDemand::calculateHeatingDemand(double area, int year) {
    double fw = 75;
    if (year > 1995)
        fw = 50;
    if (year > 2000)
        fw = 15;
    return area * fw;
}