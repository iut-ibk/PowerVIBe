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

#ifndef PLACEHOUSEHOLDS_H
#define PLACEHOUSEHOLDS_H

#include <dmmodule.h>
#include <dm.h>

using namespace DM;

class DM_HELPER_DLL_EXPORT PlaceHouseholds : public Module
{
	DM_DECLARE_NODE(PlaceHouseholds)
	private:
		DM::View grids;
	DM::View households;
	DM::View buildings;
	DM::View persons;
	int sumHouseholds(int (&hh)[5] );
	int chooseHousehold( int (&hh)[5] );
	int chooseBuilding(std::vector<int>& buildings);
public:
	PlaceHouseholds();
	void run();
	std::string getHelpUrl();
};

#endif // PLACEHOUSEHOLDS_H
