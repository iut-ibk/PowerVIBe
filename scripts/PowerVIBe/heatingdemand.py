#!/usr/bin/python
# -*- coding: utf8 -*-
from pydynamind import *
from pydmtoolbox import *
from datetime import *

class MonthlyHeatingAndCooling(Module):
    def __init__(self):        
        Module.__init__(self)
        self.Type = "COMPONENT"
        self.buildings = View("BUILDING", COMPONENT, READ)
        self.buildings.getAttribute("transmission_coefficient_heating")
        self.buildings.getAttribute("transmission_coefficient_cooling")
        self.buildings.getAttribute("T_cooling")
        self.buildings.getAttribute("T_heating")
        
        self.buildings.addAttribute("heating_days_monthly")
        self.buildings.addAttribute("cooling_days_monthly")

        
        self.Type = "COMPONENT"
        self.region = View("CITY", COMPONENT, READ)
        self.region.getAttribute("temperature_dayly")
        datastream = []
        datastream.append(self.buildings)
        datastream.append(self.region)
        self.addData("City", datastream)    
        
        

    def run(self):
        #init Database
        city = self.getData("City")
        
        climate_uuid = city.getUUIDs(self.region)[0]
        
        region = city.getComponent(climate_uuid)
        dates = region.getAttribute("temperature_dayly").getStringVector()
        temperatures = region.getAttribute("temperature_dayly").getDoubleVector()
        uuids = city.getUUIDs(self.buildings)
        for uuid in uuids:
            building = city.getComponent(uuid)
            L_heating = building.getAttribute("transmission_coefficient_heating").getDouble()
            L_cooling = building.getAttribute("transmission_coefficient_cooling").getDouble()
            print L_heating
            T_heating = building.getAttribute("T_heating").getDouble()
            T_cooling = building.getAttribute("T_cooling").getDouble()  
            firstDay = datetime.strptime(dates[0], '%Y-%m-%d')
            currentmonth = firstDay.month
            heating_days = 0
            counter = -1
            for day in dates:
                counter += 1
                if T_heating > temperatures[counter]:
                    heating_days += 1
                
                date = datetime.strptime(day, '%Y-%m-%d')            
                if currentmonth != date.month:
                    currentmonth = date.month
                    print heating_days
                    heating_days = 0
                    
                
            
            
            
    def getHelpUrl(self):        
        return "https://docs.google.com/document/pub?id=1s6rJ9mSbTrNU2ZUaF-zALxsdZ0kJZc8RywA-LVtWHgE"

