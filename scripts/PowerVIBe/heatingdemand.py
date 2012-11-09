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
        self.buildings.getAttribute("V_living")        
        
        self.buildings.addAttribute("heating_loss_monthly")
        self.buildings.addAttribute("solar_energy_montlhy")
        self.buildings.addAttribute("heating_demand_monthly")
        self.buildings.addAttribute("anual_heating_demand")
        
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

            T_heating = building.getAttribute("T_heating").getDouble()
            T_cooling = building.getAttribute("T_cooling").getDouble()  
            V_living = building.getAttribute("V_living").getDouble()  
            solarEnergy = self.calculateSolarRadiation(city, building)
            self.monthlyValues(building, V_living, L_heating, dates, temperatures, solarEnergy, T_heating)

                    
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
        
    def monthlyValues(self,building, V, L, dates, temperatures, solarEnergy, T_heating):
        firstDay = datetime.strptime(dates[0], '%Y-%m-%d')
        lastDay = datetime.strptime(dates[len(dates)-1], '%Y-%m-%d')
        currentmonth = firstDay.month
        counter = -1
        montlySolarEnergy = doublevector()   
        montlyHeatingLoss = doublevector()
        montlyHeatingDemand = doublevector()
        sum_qs = 0.
        sum_qe = 0.
        sum_t = 0
        days = 0  
        sum_tot = 0.
        for day in dates:
            date = datetime.strptime(day, '%Y-%m-%d') 
            counter += 1
            days +=1
            qs = solarEnergy[date]
            qe =  L * (T_heating - temperatures[counter]) * 24.
            sum_qs+= qs/1000.
            sum_qe+= qe/1000.
            sum_t += temperatures[counter]                                           
            if currentmonth != date.month or date == lastDay:                                    
                montlyHeatingLoss.append(sum_qe)
                montlySolarEnergy.append(sum_qs)
                etha = self.efficencyForHeatProduction(L, V, sum_qe/days, sum_qs/days) # for average month               
                lastInsert = len(montlyHeatingLoss) -1
                montlyHeatingDemand.append((sum_qe/days - etha*sum_qs/days)*days)
                if montlyHeatingDemand[lastInsert] > 0:
                    sum_tot += montlyHeatingDemand[lastInsert] 
                print str(L) + " " + str("Heating ") + str(currentmonth) + " " + str(montlyHeatingLoss[lastInsert])+" " +str(montlySolarEnergy[lastInsert]) +" " +str(montlyHeatingDemand[lastInsert])
                currentmonth = date.month
                sum_qs = 0
                sum_qe = 0
                sum_t = 0
                days = 0  

        building.getAttribute("heating_loss_monthly").setDoubleVector(montlyHeatingLoss)     
        building.getAttribute("solar_energy_montlhy").setDoubleVector(montlySolarEnergy)
        building.getAttribute("heating_demand_monthly").setDoubleVector(montlyHeatingDemand)
        building.getAttribute("anual_heating_demand").setDouble(sum_tot)
        #return montlyHeatingLoss, montlySolarEnergy, montlyHeatingDemand
    """ Based on ÖNORM B 8110-6:2009 Part 9.4 (page 45)
        
    The calculation is baed on monthly values 
    
    """
    def efficencyForHeatProduction(self, L_t, V, Q_heating, Q_prod):
        if L_t == 0:
            return 0
        
        #See page 41
        f_bw = 30. # form solid buildings
        
        C = f_bw * V
        tau = C / L_t
                
        a_0 = 1 # for heating
        tau_0 = 16 #for heating
        a = a_0 + tau/tau_0
        
        gamma = Q_heating / Q_prod
        
        if gamma != 1:
            return (1-pow(gamma,a)) / (1-pow(gamma,(a+1)))           
        return a / (a+1)
        
    def getHelpUrl(self):        
        return "https://docs.google.com/document/pub?id=1s6rJ9mSbTrNU2ZUaF-zALxsdZ0kJZc8RywA-LVtWHgE"

