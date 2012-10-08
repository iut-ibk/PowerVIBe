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

#include "edgetofaces.h"
#include "geometry.h"
#include "tbvectordata.h"

DM_DECLARE_NODE_NAME(EdgeToFaces, Geometry)

void EdgeToFaces::init()
{
    if (edgeLayerName.empty())
        return;
    if (newFaceName.empty())
        return;

    edgeLayer = DM::View(edgeLayerName, DM::EDGE, DM::READ);
    newFace = DM::View(newFaceName, DM::FACE, DM::WRITE);
    std::vector<DM::View> datastream;
    datastream.push_back(edgeLayer);
    datastream.push_back(newFace);

    this->addData("Data", datastream);


}

EdgeToFaces::EdgeToFaces()
{
    edgeLayerName = "";
    newFaceName = "";
    this->addParameter("edgeLayerName", DM::STRING, &edgeLayerName);
    this->addParameter("newFaceName", DM::STRING, &newFaceName);
}



void EdgeToFaces::run()
{
    DM::System * sys = this->getData("Data");
    DM::System s = DM::Geometry::ShapeFinder(sys,edgeLayer, newFace);

    std::vector<std::string> uuids = s.getUUIDs(newFace);

    foreach(std::string uuid, uuids) {
        DM::Face * f = s.getFace(uuid);
        std::vector<DM::Node*> nl_old = TBVectorData::getNodeListFromFace(&s, f);
        std::vector<DM::Node*> nl;
        for  (int i = 0; i < nl_old.size()-1; i++)
            nl.push_back(sys->addNode(new Node(*nl_old[i])));
        nl.push_back(nl[0]);
        sys->addFace(nl, newFace);
    }


}
