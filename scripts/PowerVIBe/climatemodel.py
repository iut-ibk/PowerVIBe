# -*- coding: utf-8 -*-
"""
Spyder Editor

This temporary script file is located here:
/Users/christianurich/.spyder2/.temp.py
"""

"""
mean mohtly temperatur 
coefficients a and b from ÖNORM B8110-5
h in m
"""
from datetime import *
from math import *
import calendar
from pydynamind import *

class ClimateModel(Module):
    def __init__(self):
        Module.__init__(self)
        self.daylyAmplitude = [7.4, 6.6, 5.5, 7.7, 6.2, 6.9, 6.1, 6.2, 5.2, 5.9, 8.2, 7.8]
        self.monthlyAmplitude = [3.2, 6.3, 6.7, 6.1, 6.4, 5.8, 6.1, 6.0, 6.6, 5.4, 4.8, 3.8]

        h = 500
        self.a = []
        self.b = []
        if h <= 750:
            self.a = [-1.129, 1.265, 5.527, 10.619, 15.235, 18.21, 19.901, 19.515, 15.822, 10.179, 4.789, 0.762]
            self.b = [-0.423, -0.458, -0.521, -0.613, -0.602, -0.61, -0.597, -0.607, -0.495, -0.413, -0.486, -0.566]  
        self.region = View("CITY", COMPONENT, READ)
        self.region.addAttribute("temperature_hourly")
        self.region.addAttribute("temperature_dayly")
        datastream = []
        datastream.append(self.region)
        self.addData("City", datastream)
    def meanTemperatur(self, month, h):
        return self.a[month-1] + self.b[month-1]*h/100.
        
    def run(self):
        city = self.getData("City")
        
        h = 500
        date = datetime(2012, 1,1)
        ahour = timedelta(hours=1)
        currentyear = date.year
        currentmonth = date.month
        currentday = date.day
        daysInCurrentMonth = calendar.monthrange(currentyear, currentmonth)[1]
        Tm = self.meanTemperatur(currentmonth, h)
        dates = stringvector()
        temperature = doublevector()
        dates_dayly = stringvector()
        temperatur_dalyly = doublevector()
        sum_temp_day = 0
        while currentyear == date.year:
            monthly= self.monthlyAmplitude[currentmonth - 1] * sin( float(date.day) / float(daysInCurrentMonth) * (2. * pi))
            dayly = self.daylyAmplitude[currentmonth - 1] * cos(date.hour / 24. * (2 * pi))
            #print monthly
            Thm = Tm + monthly - dayly
            dates.append(str(date))
            temperature.append(Thm)
            date = date + ahour
            sum_temp_day += Thm
            #calucate dayly
            if (date.day != currentday):
                temperatur_dalyly.append(sum_temp_day / 24.)
                dates_dayly.append(datetime(currentyear, currentmonth,currentday).strftime('%Y-%m-%d'))
                currentday = date.day
                sum_temp_day = 0.
                
            
            #New months new tm
            if (date.month != currentmonth):
                currentmonth = date.month
                daysInCurrentMonth = calendar.monthrange(currentyear, currentmonth)[1]
                Tm = self.meanTemperatur(currentmonth, h)
                print "Monthly mean " + str(Tm)
        uuids = city.getUUIDs(self.region)
        face = city.getComponent(uuids[0])

        
        face.getAttribute("temperature_hourly").addTimeSeries(dates, temperature)
        face.getAttribute("temperature_dayly").addTimeSeries(dates_dayly, temperatur_dalyly)

        


    