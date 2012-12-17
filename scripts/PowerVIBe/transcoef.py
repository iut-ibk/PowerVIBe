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

from numpy import genfromtxt
""" Class to load the get transfer coefficients from a csv file.
    Strucutre of the CSV file:
    before	location	type	building_standard	ceiling_cellar	ceiling_floor	wall_outside	ceiling_roof	window	g	front_door
    1600	Tirol       0       0                   1.25                0.75            1.55            1.3            2.5	0.67	2.5
    """
class HeatCoefficients:
    def __init__(self, filename):
        self.db = genfromtxt(filename, dtype=None, delimiter=',',names=True)
        self.typeList = ["ceiling_cellar", "ceiling_floor", "wall_outside", "ceiling_roof", "front_door", "window"]
    def buildingPeriode(self, periods, year_built):
        #print periods
        #print year_built
        prev_p = periods[0]
        for p in periods:
            if p > year_built:
                return prev_p
            prev_p = p
        return periods[len(periods)-1]
        

    def searchDB(self, periode, type, location,  building_standard):
        
        ress = [{"ceiling_cellar": item["ceiling_cellar"],
                "ceiling_floor": item["ceiling_floor"],
                "wall_outside": item["wall_outside"],
                "ceiling_roof": item["ceiling_roof"],
                "front_door": item["front_door"],
                "window": item["window"]} for item in self.db if \
            item['before'] == periode and item['type'] == type and item['building_standard'] == building_standard and item['location'] == location]
        return ress
    
    """ Returns the heat ransfer coefficient in W/m2K for:
        - ceiling_cellar
        - ceiling_floor
        - wall_outside
        - ceiling_roof
        - window
        - front_door
        """
    def getCoefficients(self, part ,year_built, type="", location='Tirol',  building_standard=""):
        try:
            self.typeList.index(part)
        except ValueError:
            #print part
            return 0
        periode = self.buildingPeriode(self.db['before'],year_built)
        
        ress = self.searchDB(periode, type, location,  building_standard)
        if len(ress) > 0:
            #print ress
            #print part
            return ress[0][part]
    
        ress = self.searchDB(periode, type, location,  "")
        if len(ress) > 0:
            #print ress
            #print part
            return ress[0][part]
    
        ress = self.searchDB(periode, "", location,  "")
        if len(ress) > 0:
            #print ress
            #print part
            return ress[0][part]
        return ress


HeatCoefficients("/Users/christianurich/Documents/DynaMind-ToolBox/PowerVIBe/data/TransmissionCoefficient/default_transmission_coefficients_tirol.csv")

