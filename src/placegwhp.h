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


#ifndef __DynaMind_ToolBox__placegwhp__
#define __DynaMind_ToolBox__placegwhp__

#include <iostream>

#include <dm.h>

using namespace DM;

class DM_HELPER_DLL_EXPORT PlaceGWHP : public Module
{
    DM_DECLARE_NODE(PlaceGWHP)
    
private:
    DM::View buildings;
    
public:
    PlaceGWHP();
    void run();
    
    double calcuateHydraulicEffectedArea(double Q, double kf, double IG, double kfhTokfh);
};

#endif /* defined(__DynaMind_ToolBox__placegwhp__) */
