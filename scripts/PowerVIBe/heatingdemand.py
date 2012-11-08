#!/usr/bin/python
# -*- coding: utf8 -*-

from pydynamind import *
from pydmtoolbox import *
import pydmtoolbox
from datetime import *

class MonthlyHeatingAndCooling(Module):
    def __init__(self):        
        Module.__init__(self)
        self.buildings = View("BUILDING", COMPONENT, READ)
        self.buildings.getAttribute("transmission_coefficient_heating")
        self.buildings.getAttribute("transmission_coefficient_cooling")
        self.buildings.getAttribute("T_cooling")
        self.buildings.getAttribute("T_heating")
        self.buildings.getAttribute("Geometry")
        
        self.buildings.addAttribute("heating_days_monthly")
        self.buildings.addAttribute("cooling_days_monthly")
        
        self.geometry = View("Geomtry", FACE, READ)
        self.geometry.getAttribute("solar_radiaton_dayly")
        
        
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
            print L_heating/1000.
            T_heating = building.getAttribute("T_heating").getDouble()
            T_cooling = building.getAttribute("T_cooling").getDouble()  
            solarEnergy = self.calculateSolarRadiation(city, building)
            firstDay = datetime.strptime(dates[0], '%Y-%m-%d')
            print firstDay.month
            currentmonth = firstDay.month
            heating_days = 0
            counter = -1
            for day in dates:
                date = datetime.strptime(day, '%Y-%m-%d') 
                counter += 1
                sum_qs = 0
                sum_qe = 0
                sum_t = 0
                days = 0
                days +=1
                qs = solarEnergy[date]
                qe =  L_heating * (T_heating - temperatures[counter]) * 24.
                sum_qs+= qs/1000.
                sum_qe+= qe/1000.
                sum_t += temperatures[counter]
                    
                if qs < qe:
                    heating_days += 1                
                           
                if currentmonth != date.month:                                    
                    print  str("Heating ") + str(currentmonth) + " " + str(heating_days)+" " + str(L_cooling/1000)+ " " + str(sum_t/days)+" " +str(sum_qe/days) +" " +str(sum_qs/days)
                    currentmonth = date.month
                    heating_days = 0

                    
    def calculateSolarRadiation(self, city, building):
        links = building.getAttribute("Geometry").getLinks()
        solarRadiationTotal = {}
        first = True
        for link in links:
            geom = city.getFace(link.uuid)
            if geom.getAttribute("type").getString() == "window":
                area = pydmtoolbox.TBVectorData_CalculateArea(city, geom)
                solarRadiation = geom.getAttribute("solar_radiation_dayly").getDoubleVector()
                dates = geom.getAttribute("solar_radiation_dayly").getStringVector()
                for i in range(len(dates)):
                    day = dates[i]                    
                    date = datetime.strptime(day, '%Y-%m-%d')  
                    if first == True:
                        solarRadiationTotal[date] = solarRadiation[i] * area
                    else:
                        solarRadiationTotal[date] = solarRadiationTotal[date]  + solarRadiation[i] * area
                first = False
        #print solarRadiationTotal
        return solarRadiationTotal 
            
        
    def getHelpUrl(self):        
        return "https://docs.google.com/document/pub?id=1s6rJ9mSbTrNU2ZUaF-zALxsdZ0kJZc8RywA-LVtWHgE"

