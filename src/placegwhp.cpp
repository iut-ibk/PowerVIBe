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
#include <tbvectordata.h>
#include <cgalgeometry.h>
#include <QPolygonF>
#include <fstream>
#ifdef _OPENMP
#include <omp.h>
#endif

#include "alglib/src/spline1d.h"
DM_DECLARE_NODE_NAME(PlaceGWHP, PowerVIBe)

PlaceGWHP::PlaceGWHP():
    pi(3.14159265358979323846)
{
    buildings = DM::View("BUILDING", DM::COMPONENT, DM::READ);
    buildings.getAttribute("PARCEL");
    buildings.getAttribute("monthly_peak_heating_demand");
    buildings.getAttribute("anual_heating_demand");
    buildings.addAttribute("geothermal_energy");
    buildings.getAttribute("place_gwhp");


    parcels = DM::View("PARCEL", DM::FACE, DM::READ);
    parcels.getAttribute("I_ground_water");
    parcels.getAttribute("kf");
    parcels.getAttribute("kfhTokfh");
    parcels.getAttribute("BUILDING");

    ghwps = DM::View("GWHP", DM::COMPONENT, DM::WRITE);
    ghwps.addAttribute("Q");

    ghwps.addLinks("BUILDING", buildings);
    buildings.addLinks("GWHP", ghwps);

    thermal_effected_area = DM::View("thermal_effected_area", DM::FACE, DM::WRITE);
    thermal_effected_area.addAttribute("T");

    city_view = DM::View("CITY", DM::COMPONENT, DM::READ);
    city_view.addAttribute("total_anual_heating_demand");
    city_view.addAttribute("total_anual_heating_demand_gwhp");
    //thermal_effected_area.addAttribute("b_id");
    //thermal_effected_area.addAttribute("counter");

    this->reportFile = "";

    std::vector<DM::View> data;
    data.push_back(buildings);
    data.push_back(parcels);
    data.push_back(ghwps);
    data.push_back(thermal_effected_area);
    data.push_back(city_view);
    database_location = "";
    this->deepWell = false;
    this->withCenterBuilding = false;
    this->addParameter("ReportFile", DM::STRING, &this->reportFile);

    this->addParameter("GWHP_Database", DM::FILENAME, &this->database_location);

    this->addParameter("DeepWell", DM::BOOL, &this->deepWell);

    this->addParameter("CenterBuilding", DM::BOOL, &this->withCenterBuilding);


    this->addData("City", data);

}

void PlaceGWHP::createReport(DM::System * city) {
    double heating_tot = 0;
    double gwhp_tot = 0;
    int gwhp = 0;
    std::vector<std::string> building_uuids = city->getUUIDs(buildings);
    foreach (std::string building_uuid, building_uuids) {
        DM::Component * building = city->getComponent(building_uuid);
        heating_tot+=building->getAttribute("anual_heating_demand")->getDouble();
        std::vector<LinkAttribute> links_ghwp = building->getAttribute("GHWP")->getLinks();
        foreach (LinkAttribute l,links_ghwp ) {
            gwhp++;
            gwhp_tot += building->getAttribute("anual_heating_demand")->getDouble();
        }
    }


    std::vector<std::string> uuids = city->getUUIDs(DM::View("CITY", DM::COMPONENT, DM::READ));
    double year = 0;
    DM::Component * c;
    if (uuids.size() != 0) {
        year = city->getComponent(uuids[0])->getAttribute("year")->getDouble();
        c = city->getComponent(uuids[0]);
    }
    c->addAttribute("total_anual_heating_demand", heating_tot);
    c->addAttribute("total_anual_heating_demand_gwhp", gwhp_tot);


    if (this->reportFile.empty())
        return;
    ofstream rf;
    rf.open (this->reportFile.c_str(),ios::app );






    rf << year << "\t";
    rf << building_uuids.size() << "\t";
    rf <<heating_tot << "\t";
    rf <<gwhp << "\t";
    rf <<gwhp_tot << "\t";
    rf <<gwhp_tot/heating_tot << "\t";
    rf << "\n";



    rf.close();
}

void PlaceGWHP::run()
{

    ThermalRegenerationDB TRDatabase(database_location);

    DM::System * city = this->getData("City");

    //Caluclate Water Demand For every building
    std::vector<std::string> building_uuids = city->getUUIDs(buildings);

    int TotalNumberOfBuildings = building_uuids.size();
    int counterDone = 0;
    foreach (std::string building_uuid, building_uuids) {
        counterDone++;
        Logger(Debug) << "Start" << counterDone << "Total" << TotalNumberOfBuildings;
        DM::Component * building = city->getComponent(building_uuid);
        if (building->getAttribute("place_gwhp")->getDouble() < 0.01)
            continue;
        DM::Node bcenter (building->getAttribute("centroid_x")->getDouble(), building->getAttribute("centroid_y")->getDouble(), 0);

        //GetParcel
        LinkAttribute lp = building->getAttribute("PARCEL")->getLink();
        if (lp.uuid.empty()) {
            Logger(Warning) << "Building not linked can't create GWHP";
            continue;
        }

        Face * parcel = city->getFace(lp.uuid);

        double heatingDemand = building->getAttribute("monthly_peak_heating_demand")->getDouble();
        double heatingDuration = building->getAttribute("anual_heating_demand")->getDouble() / heatingDemand;
        double Q = calculateWaterAmount(heatingDemand, 3);
        DM::Component ghwp = TRDatabase.getThermalRegernationField(
                    parcel->getAttribute("I_ground_water")->getDouble(),
                    7,
                    parcel->getAttribute("kf")->getDouble(),
                    parcel->getAttribute("kfhTokfh")->getDouble(),
                    Q,
                    heatingDuration,
                    10);

        //Create Random Node List to Check

        double d = calcuateHydraulicEffectedArea(Q,
                                                 parcel->getAttribute("kf")->getDouble(),
                                                 parcel->getAttribute("I_ground_water")->getDouble(),
                                                 parcel->getAttribute("kfhTokfh")->getDouble());

        //Logger(Debug) << d;
        std::vector<DM::Node> offsetNodes = CGALGeometry::OffsetPolygon(TBVectorData::getNodeListFromFace(city, parcel), d);

        DM::System sys_tmp;

        DM::Face* parcel_offset = TBVectorData::AddFaceToSystem(&sys_tmp, offsetNodes);

        if (!parcel_offset)
            continue;

        std::vector<DM::Node> possibleNodes_tmp = TBVectorData::CreateRaster(&sys_tmp, parcel_offset,5.0);


        //Remove possibel nodes within distance
        std::vector<DM::Node> possibleNodes;
        if (!withCenterBuilding) {

            foreach (DM::Node n , possibleNodes_tmp) {
                if (TBVectorData::calculateDistance(&bcenter, &n)>20)
                    continue;
                possibleNodes.push_back(n);
            }
        }
        else {
            for (int i = -10; i < 11; i++) {
                for (int j = -10; j < 11; j++){
                    possibleNodes.push_back(Node(bcenter.getX()+2.5*j, bcenter.getY()+2.5*i, 0));
                }
            }
        }

        //Randomize Nodes
        for (unsigned int i = 0; i < possibleNodes.size(); i++) {
            int sw1 = rand() % possibleNodes.size();
            int sw2 = rand() % possibleNodes.size();

            DM::Node tmp_Node(possibleNodes[sw1]);
            possibleNodes[sw1] = possibleNodes[sw2];
            possibleNodes[sw2] = tmp_Node;
        }

        //Create Temperatur Anamoly

        std::vector<DM::Node> thermalNodes = drawTemperaturAnomalySimple(DM::Node(0,0,0), 5, ghwp.getAttribute("L")->getDouble(), ghwp.getAttribute("B")->getDouble(), 3, city, thermal_effected_area);
        if (thermalNodes.size() == 0)
            continue;


        DM::Node ressNode;
        if (!withCenterBuilding) {
            if (!this->checkThermalEffectedAreas(city, thermalNodes, possibleNodes, ressNode, parcel, 5, ghwp.getAttribute("L")->getDouble(), ghwp.getAttribute("B")->getDouble(), d))
                continue;
        } else {
            if (!this->checkThermalEffectedAreas(city, thermalNodes, possibleNodes, ressNode, city->getFace(building->getAttribute("Footprint")->getLink().uuid), 5, ghwp.getAttribute("L")->getDouble(), ghwp.getAttribute("B")->getDouble(), d))
                continue;
        }
        DM::Component * g = city->addComponent(new Component(ghwp), ghwps);

        g->getAttribute("BUILDING")->setLink("BUILDING", building->getUUID());
        building->getAttribute("GHWP")->setLink("GHWP", g->getUUID());


        //Check Thermal Effected Areas
        std::vector<DM::Node*> nodes_t;
        foreach (DM::Node n, thermalNodes) {
            nodes_t.push_back(city->addNode(n.getX() + ressNode.getX(), n.getY() +  ressNode.getY(), n.getZ()));
        }
        nodes_t.push_back(nodes_t[0]);

        int thermnodessize = thermalNodes.size();

        DM::Face * tf = city->addFace(nodes_t, this->thermal_effected_area);

        QPolygonF p = TBVectorData::FaceAsQPolgonF(city, tf);

        QRectF p_rect = p.boundingRect();

        double X1 = p_rect.bottomLeft().x();
        double X2 = p_rect.bottomRight().x();

        double Y2 = p_rect.bottomLeft().y() ;
        double Y1 = p_rect.topRight().y();

        tf->addAttribute("X1", X1);
        tf->addAttribute("Y1", Y1);
        tf->addAttribute("X2", X2);
        tf->addAttribute("Y2", Y2);

        Logger(Debug ) << "PLACE " << counterDone;
        Logger(Debug ) << X1 << " " << Y1 << " " << X2 << " " << Y2;

        std::vector<double> Color;
        Color.push_back(0);
        Color.push_back(0);
        Color.push_back(1);
        tf->getAttribute("color")->setDoubleVector(Color);

        thermalFields.push_back(tf);
        building->addAttribute("geothermal_energy", building->getAttribute("anual_heating_demand")->getDouble());
        building->addAttribute("place_gwhp", 0);
    }
    createReport(city);

}


double PlaceGWHP::calcuateHydraulicEffectedArea(double Q, double kf, double IG, double kfhTokfh)
{
    double d = 2.+3. * pow(Q /(kf*IG*1000.*pi) * sqrt( kfhTokfh ) , (1./3.));
    return d;
}

//demand heating in W
double PlaceGWHP::calculateWaterAmount(double demandHeating, double deltaT)
{
    double cvw = 4190000.;
    double A = demandHeating;
    return A/(cvw*deltaT);
}

bool PlaceGWHP::checkThermalEffectedAreas(System *sys, const std::vector<DM::Node > & nodes, const std::vector<DM::Node > & possible_nodes, DM::Node & ressNode, DM::Face * parcel, double l1, double l2, double b, double hydrauliceffected)
{

    //Find Poosible Intersections
    QPolygonF p = TBVectorData::FaceAsQPolgonF(sys, parcel);

    QRectF p_rect = p.boundingRect();

    double x1 = p_rect.bottomLeft().x() - l1 - hydrauliceffected-50;
    double x2 = p_rect.bottomRight().x() + l2+l1+50;

    double y1 = p_rect.topRight().y() -b-50;
    double y2 = p_rect.bottomLeft().y() + b+50;


    std::vector<std::vector<DM::Node* > > nodelists;


    for (unsigned int i = 0; i < thermalFields.size(); i++) {

        DM::Face * f = thermalFields[i];
        //Logger(Debug) << f->getAttribute("counter")->getDouble();
        if (f->getAttribute("X2")->getDouble() < x1) {
            //Logger(Debug) << "X2" <<f->getAttribute("X2")->getDouble() << " " << x1;
            continue;
        }
        if (f->getAttribute("Y2")->getDouble() < y1){
            //Logger(Debug) << "Y2" <<f->getAttribute("Y2")->getDouble() << " " << y1;

            continue;
        }
        if (f->getAttribute("X1")->getDouble() > x2){
            //Logger(Debug) << "X1" <<f->getAttribute("X1")->getDouble() << " " << x2;
            continue;
        }
        if (f->getAttribute("Y1")->getDouble() > y2){
            //Logger(Debug) << "Y1" <<f->getAttribute("Y1")->getDouble() << " " << y2;
            continue;
        }
        std::vector<DM::Node* > nodelist = TBVectorData::getNodeListFromFace(sys, f);
        //Logger(Debug) << f->getAttribute("counter")->getDouble();
        nodelists.push_back(nodelist);

    }
    //Logger(Debug) << "Total " << thermalFields.size() << "check " << nodelists.size();
    std::vector<DM::Node*> nodesToCheck;

    foreach (DM::Node n, nodes) {
        nodesToCheck.push_back(new DM::Node(n.getX(), n.getY(), n.getZ()));
    }
    nodesToCheck.push_back(nodesToCheck[0]);


    std::vector<DM::Node> hydraulicEffected_ref;
    hydraulicEffected_ref.push_back(DM::Node(-hydrauliceffected/2.-hydrauliceffected, -hydrauliceffected/2,0));
    hydraulicEffected_ref.push_back(DM::Node(hydrauliceffected/2.-hydrauliceffected, -hydrauliceffected/2,0));
    hydraulicEffected_ref.push_back(DM::Node(hydrauliceffected/2.-hydrauliceffected, hydrauliceffected/2,0));
    hydraulicEffected_ref.push_back(DM::Node(-hydrauliceffected/2.-hydrauliceffected, hydrauliceffected/2,0));

    std::vector<DM::Node*> hydraulicEffected;
    for (int i = 0; i < 4; i++)
        hydraulicEffected.push_back(new DM::Node(0, 0, 0));
    hydraulicEffected.push_back(hydraulicEffected[0]);


    int nodeID = -1;


    for (unsigned int j = 0; j < possible_nodes.size(); j++) {



        if (nodeID != -1) continue;

        DM::Node n_ref = possible_nodes[j];
        for (unsigned int i = 0; i < nodes.size()-1; i++) {
            DM::Node * n = nodesToCheck[i];
            n->setX( nodes[i].getX() +  n_ref.getX() );
            n->setY( nodes[i].getY() +  n_ref.getY() );
        }
        for (unsigned int i = 0; i < hydraulicEffected_ref.size(); i++) {
            hydraulicEffected[i]->setX(hydraulicEffected_ref[i].getX() +  n_ref.getX());
            hydraulicEffected[i]->setY(hydraulicEffected_ref[i].getY() +  n_ref.getY());
        }

        bool tmp_intersect = false;
        for (unsigned int i = 0; i < nodelists.size(); i++) {
            //Logger(Debug) <<  "Regfield";
            if ( CGALGeometry::DoFacesInterect(nodelists[i],nodesToCheck ))  {
                tmp_intersect = true;
                break;
            }
            if (deepWell)
                continue;
            //Logger(Debug) << " Hydraulic Effected";
            if ( CGALGeometry::DoFacesInterect(nodelists[i],hydraulicEffected ) ) {
                tmp_intersect = true;
                break;
            }


        }
        if (tmp_intersect)
            continue;

        if (nodeID == -1) {
            nodeID = j;
            break;
        }

    }
    Logger(Debug) << "Place at " << nodeID;

    for (unsigned int i = 0; i < nodesToCheck.size()-1; i++)
        delete nodesToCheck[i];
    for (unsigned int i = 0; i < hydraulicEffected.size()-1; i++)
        delete hydraulicEffected[i];
    if (nodeID != -1) {
        ressNode.setX(possible_nodes[nodeID].getX());
        ressNode.setY(possible_nodes[nodeID].getY());
        return true;
    }



    return false;

}

std::vector<DM::Node> PlaceGWHP::drawTemperaturAnomalySimple(DM::Node p, double l1, double l2, double b, double T, DM::System * sys, DM::View v) {

    if (l1 < 1 || l2 < 1 || b < 1 ) {

        std::cout << "Warning l1 or l2 or b is zero" << std::endl;
        std::vector<Node> ress;
        return ress;
    }
    double l = l1+l2;
    double spliter = 4;
    std::vector<Node> p_above;
    std::vector<Node> p_above_side;

    ap::real_1d_array x_x;
    ap::real_1d_array x_y;
    spline1dinterpolant x;
    int n = 5;
    x_x.setlength(n);
    x_y.setlength(n);
    x_x(0) = 0;
    x_y(0) = 0;
    x_x(1) = l/50;
    x_y(1) = b/4.;
    x_x(2) = l/15.;
    x_y(2) = b/2.4;
    x_x(3) = l/3.5;
    x_y(3) = b/2.;
    x_x(4) = l;
    x_y(4) = 0;

    spline1dbuildcubic(x_x, x_y, n, 0, double(1), 0, double(0), x);

    for (double j =0; j < l+0.00001; j +=l/spliter) {
        double l_y = spline1dcalc(x, j);
        Node n1 (j+p.getX(), l_y+p.getY(), 0);
        p_above.push_back(n1);

        DM::Node n2(j+p.getX(), p.getY()-l_y, 0);
        if (n1.compare2d(n2, 0.0001))
            continue;

        p_above_side.push_back(n2);

    }

    std::reverse(p_above_side.begin(), p_above_side.end());

    foreach (DM::Node n, p_above_side)
        p_above.push_back(n);

    return p_above;


}



void PlaceGWHP::drawTemperaturAnomalyComplex(DM::Node p, double l1, double l2, double b, double T, DM::System * sys, DM::View v) {


    if (l1 < 1 || l2 < 1 || b < 1 ) {

        std::cout << "Warning l1 or l2 or b is zero" << std::endl;
    }
    double l = l1+l2;
    double spliter = 4;
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
