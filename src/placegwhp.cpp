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

#include <math.h>
#include "placegwhp.h"
#include "thermalregenerationDB.h"

DM_DECLARE_NODE_NAME(PlaceGWHP, PowerVIBe)

PlaceGWHP::PlaceGWHP()
{
    buildings = DM::View("BUILDING", DM::COMPONENT, DM::READ);
    buildings.getAttribute("PARCEL");
    buildings.getAttribute("heating_demand");
    buildings.getAttribute("heating_duration");
    
        
    parcels = DM::View("PARCEL", DM::FACE, DM::READ);
    parcels.getAttribute("I");
    parcels.getAttribute("kf");
    parcels.getAttribute("kfhTokfh");
    parcels.getAttribute("BUILDING");
    
    ghwps = DM::View("COMPONENT", DM::COMPONENT, DM::MODIFY);
    ghwps.addAttribute("Q");
    
    ghwps.addLinks("BUILDING", buildings);
    buildings.addLinks("GWHP", ghwps);
    
    std::vector<DM::View> data;
    data.push_back(buildings);
    data.push_back(parcels);
    data.push_back(ghwps);
    database_location = "";
    this->addParameter("GWHP_Database", DM::FILENAME, &this->database_location);
    
    this->addData("City", data);
    
}

void PlaceGWHP::run()
{
    
    ThermalRegenerationDB TRDatabase(database_location);
    
    DM::System * city = this->getData("City");
    
    //Caluclate Water Demand For every building
    std::vector<std::string> building_uuids = city->getUUIDs(buildings);
    
    foreach (std::string building_uuid, building_uuids) {        
        DM::Component * building = city->getComponent(building_uuid);
        
        //GetParcel
        LinkAttribute lp = building->getAttribute("PARCEL")->getLink();
        if (lp.uuid.empty()) {
            Logger(Warning) << "Building not linked can't create GWHP";
            continue;
        }
        
        Face * parcel = city->getFace(lp.uuid);
        double heatingDemand = building->getAttribute("heating_demand")->getDouble();        
        double Q = calculateWaterAmount(heatingDemand, 3);
        //building->addAttribute("gwhp_q", Q);
        DM::Component ghwp = TRDatabase.getThermalRegernationField(
                                                                   parcel->getAttribute("I")->getDouble(),
                                                                   parcel->getAttribute("kf")->getDouble(),
                                                                   parcel->getAttribute("kfhTokfh")->getDouble(),
                                                                   Q,
                                                                   building->getAttribute("heating_duration")->getDouble());
        
    }
    
    
    
}
double PlaceGWHP::calcuateHydraulicEffectedArea(double Q, double kf, double IG, double kfhTokfh)
{
    return 2+3 * pow(Q /(kf*IG*pi) * sqrt( kfhTokfh ) , (1./3.));
}

double PlaceGWHP::calculateWaterAmount(double demandHeating, double deltaT)
{
    double cvw = 4190000.;
    double A = demandHeating;
    return A/(cvw*deltaT);
}
