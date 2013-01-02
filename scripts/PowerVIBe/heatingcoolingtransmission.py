#!/usr/bin/python
# -*- coding: utf8 -*-

"""
@file
@author  Chrisitan Urich <christian.urich@gmail.com>
@version 1.0
@section LICENSE

This file is part of PowerVIBe
Copyright (C) 2012  Christian Urich

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
"""

from pydynamind import *
from pydmtoolbox import *
from transcoef import *


class HeatingCoolingTransmission(Module):
    def __init__(self):        
        Module.__init__(self)
        self.createParameter("FileName", FILENAME, "FileName")
        self.FileName = ""
        self.Type = "COMPONENT"
        self.buildings = View("BUILDING", COMPONENT, READ)
        self.buildings.getAttribute("built_year")
        self.buildings.addAttribute("transmission_coefficient_heating")
        self.buildings.addAttribute("transmission_coefficient_cooling")
        #self.buildings.addAttribute("transmission_coefficient_ventilation")
        datastream = []
        datastream.append(self.buildings)
        self.addData("City", datastream)    


    def run(self):
        #init Database
        coefDB = HeatCoefficients(self.FileName)
        city = self.getData("City")
        
        
        uuids = city.getUUIDs(self.buildings)
        for uuid in uuids:
            building = city.getComponent(uuid)
            Le = 0
            #print uuid
            parts = building.getAttribute("Geometry").getLinks()
            for lpart in parts:
                    part = city.getFace(lpart.uuid)
                    area = TBVectorData_CalculateArea(city, part)
                    
                    U = coefDB.getCoefficients(part.getAttribute("type").getString(), building.getAttribute("built_year").getDouble() , building.getAttribute("type").getString())                    
                    l = area*U
                    if part.getAttribute("type").getString() == "ceiling_cellar":
                        l = l*0.7
                    Le += l
            Lv = building.getAttribute("gross_floor_area").getDouble() * 0.7072 * 0.4
            Lt = Le + Lv
            building.addAttribute("transmission_coefficient_heating", Lt)
            #building.addAttribute("transmission_coefficient_ventilation", Lv)
            building.addAttribute("transmission_coefficient_cooling", Lt)

            
    def getHelpUrl(self):        
        return "https://docs.google.com/document/pub?id=1s6rJ9mSbTrNU2ZUaF-zALxsdZ0kJZc8RywA-LVtWHgE"

