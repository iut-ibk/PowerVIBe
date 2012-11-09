#!/usr/bin/python
# -*- coding: utf8 -*-
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

