/**
 * @file
 * @author  Chrisitan Urich <christian.urich@gmail.com>
 * @version 1.0
 * @section LICENSE
 *
 * This file is part of DynaMind
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


#ifndef MODERNBUILDING_H
#define MODERNBUILDING_H

#include <dm.h>
using namespace DM;

class DM_HELPER_DLL_EXPORT ModernBuilding: public Module
{
     DM_DECLARE_NODE(ModernBuilding)
     private:
             DM::View buildings;
             DM::View geometry;
             DM::View footprints;
             double heatingT;
             double coolingT;
             int buildyear;
             int stories;
             double alpha;
             double l;
             double b;

             double overhang;
             double parapet;
             double w_height;
             double w_width;
             double w_distance;


public:
    ModernBuilding();
    void run();
};

#endif // MODERNBUILDING_H
