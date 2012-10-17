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
#include "alglib/src/spline1d.h"
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
    
    ghwps = DM::View("GWHP", DM::COMPONENT, DM::WRITE);
    ghwps.addAttribute("Q");
    
    ghwps.addLinks("BUILDING", buildings);
    buildings.addLinks("GWHP", ghwps);
 
    regzones = DM::View("REGZONES", DM::FACE, DM::WRITE);
    regzones.addAttribute("T");

    std::vector<DM::View> data;
    data.push_back(buildings);
    data.push_back(parcels);
    data.push_back(ghwps);
    data.push_back(regzones);
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
                                                                   7,
                                                                   parcel->getAttribute("kf")->getDouble(),
                                                                   parcel->getAttribute("kfhTokfh")->getDouble(),
                                                                   Q,
                                                                   building->getAttribute("heating_duration")->getDouble(),
                                                                   10);
        DM::Component * g = city->addComponent(new Component(ghwp), ghwps);
        
        g->getAttribute("BUILDING")->setLink("BUILDING", building->getUUID());
        building->getAttribute("GHWP")->setLink("GHWP", g->getUUID());
        DM::Node n = *city->getNode(parcel->getNodes()[0]);
        drawTemperaturAnomaly(n, 5, ghwp.getAttribute("L")->getDouble(), ghwp.getAttribute("B")->getDouble(), 3, city, regzones);
        break;
        
        
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

void PlaceGWHP::drawTemperaturAnomaly(DM::Node p, double l1, double l2, double b, double T, DM::System * sys, DM::View v) {
    
    if (l1 < 1 || l2 < 1 || b < 1 ) {
        
        std::cout << "Warning l1 or l2 or b is zero" << std::endl;
    }
    double l = l1+l2;
    double spliter = 40;
    std::vector<Node*> p_above;
    std::vector<Node*> p_above_side;
    
    for (double i = 1; i <= T; i++) {
        ap::real_1d_array x_x;
        ap::real_1d_array x_y;
        spline1dinterpolant x;
        int n = 5;
        x_x.setlength(n);
        x_y.setlength(n);
        x_x(0) = 0;
        x_y(0) = 0;
        x_x(1) = (i/T)*l/50;
        x_y(1) = (i/T)*b/4.;
        x_x(2) = (i/T)*l/15.;
        x_y(2) = (i/T)*b/2.4;
        x_x(3) = (i/T)*l/3.5;
        x_y(3) = (i/T)*b/2.;
        x_x(4) = (i/T)*l;
        x_y(4) = 0;
        
        spline1dbuildcubic(x_x, x_y, n, 0, double(1), 0, double(0), x);
        for (double j =0; j < (i/T)*l+0.00001; j +=(i/T)*l/spliter) {
            double l_y = spline1dcalc(x, j);
            Node * n1 = sys->addNode(j+p.getX()-(i/T)*l2, l_y+p.getY(), 0);
            p_above.push_back(n1);
            
            Node * n2 = sys->addNode(j+p.getX()-(i/T)*l2, p.getY()-l_y, 0);
            p_above_side.push_back(n2);

        }
        
    }
    
    Node * n1 = sys->addNode(p.getX(), p.getY(), 0);
    p_above.push_back(n1);
    
    Node * n2 = sys->addNode(p.getX(), p.getY(), 0);
    p_above_side.push_back(n2);

    spliter++;
    std::vector<Face> f_above;
    std::vector<Face> f_above_side;
    for (double i = 0; i < T-1; i++) {
        for ( int j = 0; j < spliter-1;  j++) {
            std::vector<DM::Node*> f_above;
            f_above.push_back(p_above[j+(i+1)*spliter]);
            f_above.push_back(p_above[j+(i+1)*spliter+1]);
            f_above.push_back(p_above[j+i*spliter+1]);
            f_above.push_back(p_above[j+i*spliter]);
            f_above.push_back(p_above[j+(i+1)*spliter]);
  
            std::vector<DM::Node*> f_above_side;
            f_above_side.push_back(p_above_side[j+(i+1)*spliter]);
            f_above_side.push_back(p_above_side[j+(i+1)*spliter+1]);
            f_above_side.push_back(p_above_side[j+i*spliter+1]);
            f_above_side.push_back(p_above_side[j+i*spliter]);
            f_above_side.push_back(p_above_side[j+(i+1)*spliter]);
            
            Face * f1 = sys->addFace(f_above,v);
            f1->addAttribute("T", T-i);
            Face * f2 = sys->addFace(f_above_side,v);
            f2->addAttribute("T", T-i);
        }
    }
    
    for ( int j = 0; j < spliter-1;  j++) {
        int i = 0;
        std::vector<DM::Node*> f_above;
        f_above.push_back(p_above[j+i*spliter+1]);
        f_above.push_back(p_above[j+i*spliter]);
        f_above.push_back(p_above[p_above.size()-1]);
        f_above.push_back(p_above[j+i*spliter+1]);

        std::vector<DM::Node*> f_above_side;
        f_above_side.push_back(p_above_side[j+i*spliter+1]);
        f_above_side.push_back(p_above_side[j+i*spliter]);
        f_above_side.push_back(p_above_side[p_above.size()-1]);
        f_above_side.push_back(p_above_side[j+i*spliter+1]);
                                            
        Face * f1 = sys->addFace(f_above,v);
        f1->addAttribute("T", T-i);
        Face * f2 = sys->addFace(f_above_side,v);
        f2->addAttribute("T", T-i);
    }
    
}
