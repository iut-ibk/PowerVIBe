# -*- coding: utf-8 -*-
"""
Created on Mon Dec 17 16:31:05 2012

@author: christianurich
"""

"""self.view_building = View("BUILDING", COMPONENT, READ)
        self.view_building.getAttribute("dayly_peak_heating_demand")
        self.view_building.getAttribute("anual_heating_demand")"""
        
from pydmtoolbox import *
from pydynamind import *

from math import *


class SimpleHeatingDemand(Module):
    def __init__(self):
        Module.__init__(self)
        self.buildings = View("BUILDING", COMPONENT, READ)
        self.buildings.getAttribute("built_year")
        self.buildings.getAttribute("gross_floor_area")
        self.buildings.addAttribute("anual_heating_demand")
        self.buildings.addAttribute("monthly_peak_heating_demand")
        self.buildings.addAttribute("dayly_peak_heating_demand") 
        
        datastream = []
        datastream.append(self.buildings)
        self.addData("city", datastream)
    
    def run(self):
        print "HUHHU"
        city = self.getData("city")
        heating_demands = [(1980., 75.),
                           (1990., 50.),
                           (2000., 40.),
                           (2010., 15.),]
        building_uuids = city.getUUIDs(self.buildings)
        print building_uuids
        
        for building_uuid in building_uuids:
            
            building = city.getComponent(building_uuid)
            built_year= building.getAttribute("built_year").getDouble()
            area = building.getAttribute("gross_floor_area").getDouble()
            peak_demand = heating_demands[0][1]
            for hds in heating_demands:
                if hds < built_year:
                    break
                peak_demand = hds[1]
            anual =  peak_demand * area*1800.
            peak =  peak_demand * area
            building.addAttribute("dayly_peak_heating_demand", peak)
            building.addAttribute("monthly_peak_heating_demand", peak)
            building.addAttribute("anual_heating_demand", anual)  
        
        
        
        
        