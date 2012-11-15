/**
 * @file
 * @author  Chrisitan Urich <christian.urich@gmail.com>
 * @version 1.0
 * @section LICENSE
 *
 * This file is part of PowerVIBe
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


#include "modernbuilding.h"

#include <tbvectordata.h>
#include <littlegeometryhelpers.h>

DM_DECLARE_NODE_NAME(ModernBuilding, Buildings)

ModernBuilding::ModernBuilding()
{
    buildings = DM::View("BUILDING", DM::COMPONENT, DM::WRITE);
    buildings.addAttribute("built_year");
    buildings.addAttribute("stories");
    buildings.addAttribute("stories_below"); //cellar counts as story
    buildings.addAttribute("stories_height");

    buildings.addAttribute("floor_area");
    buildings.addAttribute("gross_floor_area");

    buildings.addAttribute("area_walls_outside");
    buildings.addAttribute("area_windows");

    buildings.addAttribute("centroid_x");
    buildings.addAttribute("centroid_y");

    buildings.addAttribute("l_bounding");
    buildings.addAttribute("b_bounding");
    buildings.addAttribute("h_bounding");

    buildings.addAttribute("alpha_bounding");

    buildings.addAttribute("alpha_roof");

    buildings.addAttribute("cellar_used");
    buildings.addAttribute("roof_used");

    buildings.addAttribute("T_heating");
    buildings.addAttribute("T_cooling");

    buildings.addAttribute("Geometry");
    buildings.addAttribute("V_living");

    geometry = DM::View("Geometry", DM::FACE, DM::WRITE);
    geometry.addAttribute("type");

    footprints = DM::View("Footprint", DM::FACE, DM::WRITE);

    std::vector<DM::View> data;
    data.push_back(buildings);
    data.push_back(geometry);
    data.push_back(footprints);
    data.push_back(DM::View("dummy", DM::SUBSYSTEM, DM::MODIFY));

    heatingT    = 20;
    coolingT    = 26;

    buildyear   = 1985;
    stories     = 2;
    l           = 16;
    b           = 10;
    alpha       = 30;

    overhang    = 2;
    parapet     = 0.2;
    w_height    = 2.1;
    w_width     = 1.1;
    w_distance  = 3.0;

    this->addParameter("built_year", DM::INT, &buildyear);
    this->addParameter("l",          DM::DOUBLE, &l);
    this->addParameter("b",          DM::DOUBLE, &b);
    this->addParameter("stories",    DM::INT, &stories);
    this->addParameter("alhpa",      DM::DOUBLE, &alpha);
    this->addParameter("overhang",   DM::DOUBLE, &overhang);

    this->addParameter("parapet",    DM::DOUBLE, &parapet);
    this->addParameter("w_height",   DM::DOUBLE, &w_height);
    this->addParameter("w_width",    DM::DOUBLE, &w_width);
    this->addParameter("w_distance", DM::DOUBLE, &w_distance);


    this->addParameter("T_heating",  DM::DOUBLE, &heatingT);
    this->addParameter("T_cooling",  DM::DOUBLE, &coolingT);

    this->addData("city", data);

}

void ModernBuilding::run()
{
    std::vector<double> roofColor;
    roofColor.push_back(0.66);
    roofColor.push_back(0.66);
    roofColor.push_back(0.66);
    std::vector<double> wallColor;
    wallColor.push_back(196./255.);
    wallColor.push_back(196./255.);
    wallColor.push_back(196./255.);
    std::vector<double> windowColor;
    windowColor.push_back(0.53);
    windowColor.push_back(0.81);
    windowColor.push_back(1);

    //Get simulation datastream
    DM::System * city = this->getData("city");

    //Create building container
    DM::Component * building = city->addComponent(new Component(), buildings);

    //Set building parameters
    building->addAttribute("type", 1001);
    building->addAttribute("built_year", buildyear);
    building->addAttribute("stories", stories);
    building->addAttribute("stories_below", 0); //cellar counts as story
    building->addAttribute("stories_height",3 );

    building->addAttribute("floor_area", l*b);
    building->addAttribute("gross_floor_area", l * b * stories);
    building->addAttribute("V_living", l*b*stories * 3);

    building->addAttribute("l_bounding", l);
    building->addAttribute("b_bounding", b);
    building->addAttribute("h_bounding", stories * 3);

    building->addAttribute("alpha_bounding", 0);

    building->addAttribute("alpha_roof", 0);

    building->addAttribute("cellar_used", 1);
    building->addAttribute("roof_used", 0);

    building->addAttribute("T_heating", heatingT);
    building->addAttribute("T_cooling", 26);


    //Building Footprint. Orientation matters. Check if the normal vector of the face is in your direction
    std::vector<DM::Node * > nodes_footprint;

    DM::Node * n1  = city->addNode(-l/2.,  -b/2., 0.);
    DM::Node * n2  = city->addNode( l/2.,  -b/2., 0.);
    DM::Node * n3  = city->addNode( l/2.,   b/2., 0.);
    DM::Node * n4  = city->addNode(-l/2.,   b/2., 0.);

    nodes_footprint.push_back(n1);
    nodes_footprint.push_back(n2);
    nodes_footprint.push_back(n3);
    nodes_footprint.push_back(n4);
    nodes_footprint.push_back(n1); //To a closed polygon last is first

    //Overhang
    DM::Node n1_c (-l/2. - overhang,  -b/2. - overhang, 0.);
    DM::Node n2_c ( l/2. + overhang,  -b/2. - overhang, 0.);
    DM::Node n3_c ( l/2. + overhang,   b/2. + overhang, 0.);
    DM::Node n4_c (-l/2. - overhang,   b/2. + overhang, 0.);

    std::vector<DM::Node> overhang_nodes;
    overhang_nodes.push_back(n1_c);
    overhang_nodes.push_back(n2_c);
    overhang_nodes.push_back(n3_c);
    overhang_nodes.push_back(n4_c);


    //Add Footprint
    DM::Face * footprint = city->addFace(nodes_footprint, footprints);

    Node  n = TBVectorData::CaclulateCentroid(city, footprint);
    building->addAttribute("centroid_x", n.getX());
    building->addAttribute("centroid_y", n.getY());

    //Set footprint as floor
    DM::Face * base_plate = city->addFace(nodes_footprint, geometry);
    building->getAttribute("Geometry")->setLink("Geometry", base_plate->getUUID());

    //Define baseplate as cellar
    base_plate->addAttribute("type", "ceiling_cellar");

    //Extrude Footprint

    for (int story = 0; story < stories; story++) {
        std::vector<DM::Face*> extruded_faces = TBVectorData::ExtrudeFace(city, geometry, nodes_footprint, 3);
        //last face in vector is ceiling
        int unsigned lastID = extruded_faces.size()-1;
        for (unsigned int i = 0; i < extruded_faces.size(); i++) {
            DM::Face * f = extruded_faces[i];
            //if wall
            if (i != lastID) {
                //Add type
                f->addAttribute("type", "wall_outside");
                //Add color
                f->getAttribute("color")->setDoubleVector(wallColor);
                //Create Windows every 5m with = 1.5m height = 1.0m
                std::vector<DM::Face* > windows = LittleGeometryHelpers::CreateHolesInAWall(city, f, w_distance, w_width, w_height, parapet);
                foreach (DM::Face * w, windows) {
                    city->addComponentToView(w, this->geometry);
                    w->addAttribute("type", "window");
                    w->getAttribute("color")->setDoubleVector(windowColor);
                    building->getAttribute("Geometry")->setLink("Geometry", w->getUUID());
                }
            }
            //if ceiling
            else if (story != stories -1){
                f->addAttribute("type", "ceiling");
                f->getAttribute("color")->setDoubleVector(wallColor);
                //Start for the next level
                nodes_footprint = TBVectorData::getNodeListFromFace(city, f);
            } else {
                f->addAttribute("type", "ceiling_roof");
                f->getAttribute("color")->setDoubleVector(roofColor);
            }
            //link face to building
            building->getAttribute("Geometry")->setLink("Geometry", f->getUUID());

        }
        //create hangover
        std::vector<DM::Node * > nodelist_ho;
        foreach (DM::Node n, overhang_nodes) {
            nodelist_ho.push_back(city->addNode(n.getX(), n.getY(), (story+1) * 3));
        }
        DM::Face * ceiling = extruded_faces[lastID];
        nodelist_ho.push_back(nodelist_ho[0]); //Last node ist start node to close face
        DM::Face * f = city->addFace(nodelist_ho, geometry);
        f->addHole(TBVectorData::getNodeListFromFace(city, ceiling));
        f->getAttribute("color")->setDoubleVector(wallColor);
        building->getAttribute("Geometry")->setLink("Geometry", f->getUUID());
    }

    //Calculate areas
    std::vector<DM::LinkAttribute> links = building->getAttribute("Geometry")->getLinks();

    double windows_a = 0;
    double wall_outside_a = 0;
    foreach (LinkAttribute la, links) {
        DM::Face * f = city->getFace(la.uuid);
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
