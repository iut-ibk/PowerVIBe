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
#include "cgalgeometry.h"
#include "tbvectordata.h"
#include <dmgeometry.h>

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
	tolerance = 0.01;
	snapping = false;
	this->addParameter("edgeLayerName", DM::STRING, &edgeLayerName);
	this->addParameter("newFaceName", DM::STRING, &newFaceName);
	this->addParameter("tolerance", DM::DOUBLE, &tolerance);
	this->addParameter("snapping", DM::BOOL, &snapping);
}



void EdgeToFaces::run()
{
	DM::System * sys = this->getData("Data");
	DM::System s = CGALGeometry::ShapeFinder(sys,edgeLayer, newFace, this->snapping, this->tolerance);

	std::vector<std::string> uuids = s.getUUIDs(newFace);


	foreach(std::string uuid, uuids) {
		SpatialNodeHashMap spnh(sys, 1,false);
		DM::Face * f = s.getFace(uuid);
		std::vector<DM::Node*> nl_old = TBVectorData::getNodeListFromFace(&s, f);
		std::vector<DM::Node*> nl;
		for  (unsigned int i = 0; i < nl_old.size(); i++) {
			if (spnh.findNode(nl_old[i]->getX(), nl_old[i]->getY(), 0.001)) {
				Logger(Debug) << "Node exists already, not added";
				continue;
			}
			DM::Node * n = spnh.addNode(nl_old[i]->getX(), nl_old[i]->getY(),nl_old[i]->getZ(), 0.001);
			nl.push_back(n);
		}
		sys->addFace(nl, newFace);
	}


}
