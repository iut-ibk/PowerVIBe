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
        self.buildings.addAttribute("monthly_peak_heating_demand")
        self.buildings.addAttribute("dayly_peak_heating_demand")        
        
        self.geometry = View("Geomtry", FACE, READ)
        self.geometry.getAttribute("solar_radiaton_dayly")
        
        self.createParameter("withSolarRadiation", BOOL, "withSolarRadiation")
        self.withSolarRadiation = False
        
        
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
            solarEnergy = {}
            if self.withSolarRadiation == True:
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

        return solarRadiationTotal 
        
    def monthlyValues(self,building, V, L, dates, temperatures, solarEnergy, T_heating):
        firstDay = datetime.strptime(dates[0], '%Y-%m-%d')
        lastDay = datetime.strptime(dates[len(dates)-1], '%Y-%m-%d')
        currentmonth = firstDay.month
        
        counter = -1
        monthlySolarEnergy = doublevector()   
        monthlyHeatingLoss = doublevector()
        monthlyHeatingDemand = doublevector()
        monthlyDates = stringvector()
        sum_qs = 0.
        sum_qe = 0.
        sum_t = 0
        days = 0  
        sum_tot = 0.
        sum_sun_tot = 0.
        max_daily_peak = 0.
        monthly_peak = 0.
        max_monthly_peak = 0.
        
        for day in dates:
            date = datetime.strptime(day, '%Y-%m-%d') 
            counter += 1
            days +=1
            qs = 0.
            if self.withSolarRadiation == True:
                qs = solarEnergy[date]
                      
            daily_peak =  L * (T_heating - temperatures[counter])
            qe = daily_peak * 24.
            
            if daily_peak > max_daily_peak:
                max_daily_peak = daily_peak
                
            if daily_peak > 0:
                monthly_peak+=daily_peak
                
            sum_qs+= qs
            sum_qe+= qe
            
            sum_t += temperatures[counter]                                           
            if currentmonth != date.month or date == lastDay: 
                monthlyDates.append(datetime(date.year, currentmonth, 15).strftime('%Y-%m-%d'))                          
                monthlyHeatingLoss.append(sum_qe)
                monthlySolarEnergy.append(sum_qs)
                
                etha = self.efficencyForHeatProduction(L, V, sum_qe/days, sum_qs/days) # for average month               
                lastInsert = len(monthlyHeatingLoss) -1
                monthlyHeatingDemand.append((sum_qe/days - etha*sum_qs/days)*days)
                sum_sun_tot += sum_qs
                if monthlyHeatingDemand[lastInsert] > 0:
                    sum_tot += monthlyHeatingDemand[lastInsert]
                #print str(L) + " " + str("Heating ") + str(currentmonth) + " " + str(monthlyHeatingLoss[lastInsert])+" " +str(monthlySolarEnergy[lastInsert]) +" " +str(monthlyHeatingDemand[lastInsert])
                
                if monthly_peak > max_monthly_peak/days:
                    max_monthly_peak = monthly_peak/days
                currentmonth = date.month
                sum_qs = 0
                sum_qe = 0
                sum_t = 0
                days = 0  
                monthly_peak = 0
        #print  sum_sun_tot
        
        building.getAttribute("heating_loss_monthly").addTimeSeries(monthlyDates, monthlyHeatingLoss)     
        building.getAttribute("solar_energy_montlhy").addTimeSeries(monthlyDates, monthlySolarEnergy)
        building.getAttribute("heating_demand_monthly").addTimeSeries(monthlyDates, monthlyHeatingDemand)
        building.getAttribute("anual_heating_demand").setDouble(sum_tot)
        building.getAttribute("monthly_peak_heating_demand").setDouble(max_monthly_peak)
        building.getAttribute("dayly_peak_heating_demand").setDouble(max_daily_peak)        
        
        
    """ Based on ÖNORM B 8110-6:2009 Part 9.4 (page 45)
        
    The calculation is based on monthly values 
    
    """
    def efficencyForHeatProduction(self, L_t, V, Q_heating, Q_prod):
        if L_t == 0:
            return 0
        if Q_prod == 0:
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

