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

#include <dmmoduleregistry.h>
#include <dmnodefactory.h>
#include <placehouseholds.h>
#include <placejobs.h>
#include <annualemploymentcontrol.h>
#include <annualhouseholdcontroltotals.h>
#include <edgetofaces.h>
#include <householdplacementdtu.h>
#include <createsinglefamilyhouses.h>
#include <placegwhp.h>
#include <heatingdemand.h>
#include <exportkml.h>

using namespace DM;

extern "C" void DM_HELPER_DLL_EXPORT  registerModules(ModuleRegistry *registry) {
    registry->addNodeFactory(new NodeFactory<PlaceHouseholds>());
    registry->addNodeFactory(new NodeFactory<PlaceJobs>());
    registry->addNodeFactory(new NodeFactory<AnnualEmploymentControl>());
    registry->addNodeFactory(new NodeFactory<AnnualHouseholdControlTotals>());
    registry->addNodeFactory(new NodeFactory<EdgeToFaces>());
    registry->addNodeFactory(new NodeFactory<HouseholdPlacementDTU>());
    registry->addNodeFactory(new NodeFactory<CreateSingleFamilyHouses>());
    registry->addNodeFactory(new NodeFactory<HeatingDemand>());
    registry->addNodeFactory(new NodeFactory<PlaceGWHP>());
    registry->addNodeFactory(new NodeFactory<ExportKML>());
}
