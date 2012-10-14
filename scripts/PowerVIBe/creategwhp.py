#!/usr/bin/python
# -*- coding: utf8 -*-
from pydynamind import *
import Polygon
import vibehelper
import numpy as np
from math import pi, sqrt
#import sympy as sym
import random
import Polygon
from Polygon.Utils import pointList
from ThermalRegenerationField import *
from collections import *


class CreateGWHP(Module):        

    def __init__(self):
            Module.__init__(self)
    def init(self, params):
        self.
        self.paramComplete = True   
        self.usable = 0
        self.addParameter(self, "usable", VIBe2.DOUBLEDATA_OUT)        
        if params.has_key("VectorDataHouses") == True:        
            self.VectorDataHouses = params["VectorDataHouses"].split()
        if params.has_key("Anomalies") == True:        
            self.Anomalies = params["Anomalies"].split()[0]
        if params.has_key("IDHouses") == True:        
            self.IDHouses = params["IDHouses"].split()
        if params.has_key("Database") == True:                
            self.Database = params["Database"].split()
        if params.has_key("ResultFile") == True:
            filename = params["ResultFile"].split()   
            self.FileName =  filename[0]
        if params.has_key("Year") == True:
            tmp = params["Year"].split()   
            self.year =  int(tmp[0])
        if params.has_key("I") == True:
            tmp = params["I"].split()   
            self.I =  float(tmp[0])
        if params.has_key("kf") == True:
            tmp = params["kf"].split()   
            self.kf =  float(tmp[0])
        self.PlotResults = False    
        if params.has_key("PlotResults") == True:
            tmp = params["PlotResults"].split()  
            if str(tmp[0]) == "true" or str(tmp[0]) == "True" :
               self.PlotResults = True
        self.Grouping = True    
        if params.has_key("Grouping") == True:
            tmp = params["Grouping"].split()  
            if str(tmp[0]) == "false" or str(tmp[0]) == "False" :
               self.Grouping = False              
        FILE = open(self.FileName ,"w")
        FILE.close() # this is icing, you can just exit and this will be
        self.t = ThermalRegenerationField(self.Database[0])
    
    def run(self):
        GP = random.randint(0,100)
        print "Run CreateGHWP"
        vec = VectorData()
        totalDemand = 0
        totalQ = 0
        palcedDemand = 0
        palcedQ = 0
        totalArea = 0
        v = self.getVectorData(self.VectorDataHouses[0])
        counter = int(v.getDoubleAttributes("numberOf"+self.IDHouses[0])[0])
        NAHouses = counter
        Sites = {}
        #link buildings with sites
        for houseID in range(counter):  
            name = self.IDHouses[0]+str(houseID)  
            if len(v.getLinkNames()) > 0:
                links = v.getLinks(name)
                for l in range(len(links)):   
                    vectordataname =  links[l].getVectorDataName() 
                    if ( str(vectordataname) == "Faces"): 
                        site = str(links[l].getID())
                        if Sites.has_key(site) == True:
                            Sites[site].append(str(name))
                        else: 
                            Sites[site] = []
                            Sites[site].append(str(name))
            else:
                Sites[str(name)] = [str(name)]
                
        #create GHWP Systems
        PriorityMap = defaultdict(list)
        GWHPs = []
        for site in Sites.keys():  
            GWHP = {}
            if ( random.randint(0,100) < GP):
                grouping = False
            else:
                grouping = True
            if self.Grouping == False:
                grouping =False
            houses = Sites[site]
            pxmin =  -1
            pymin =  -1
            pxmax =  -1
            pymax =  -1
            Q = 0
            demand = 0
            id  = 1
            for house in houses:                
                grf = v.getAttributes(house).getAttribute("GSF")
                area = v.getAttributes(house).getAttribute("BGF")
                priority = v.getAttributes(house).getAttribute("GWR")
                #year = v.getAttributes(house).getAttribute("CA")
                demand += self.calculateHeatingDemand(area, self.year)                
                totalArea += area

                #Find parcel 
                points = []
                if len(v.getLinkNames()) > 0:
                    links = v.getLinks(house)                
                    
                    for l in range(len(links)):                    
                            vectordataname =  links[l].getVectorDataName()
                            layerid = links[l].getID()
    
                            if ( str(vectordataname) == "Houses"):
                                points = self.getVectorData(vectordataname).getVectorData(layerid)
                                poly_tmp = []

                if len(points) == 0:
                        points = v.getVectorData(house)
                poly_tmp = []        
                for i in range(len(points)):
                    poly_tmp.append((points[i].getX(),points[i].getY()))
                poly = Polygon.Polygon(poly_tmp)
                
                #Most Right Point    
                for p in pointList(poly):
                        if p[0] > pxmin or pxmin == -1:
                            pxmin = p[0]
                            pymin = p[1]
                        elif p[0] ==  pxmin and pymin >  p[1]:
                            pxmin = p[0]
                            pymin = p[1]                           
                 #Most Left Point    
                for p in pointList(poly):
                        if p[0] < pxmax or pxmax == -1:                                
                            pxmax = p[0]
                            pymax = p[1]  
                        elif p[0] == pxmax and pymax <  p[1]:
                            pxmax = p[0]
                            pymax = p[1]  
                            
                        
                GWHP["Priority"] = priority
                GWHP["pxmin"] = pxmin
                GWHP["pymin"] = pymin
                GWHP["pxmax"] = pxmax
                GWHP["pymax"] = pymax   
                GWHP["id"] = id
                GWHP["Demand"] = demand  
                if grouping == False:
                    Q = self.calculateWaterAmount(demand, 3)
                    GWHP["Q"] = Q
                    GWHPs.append(GWHP)
                    PriorityMap[GWHP["Priority"]].append(GWHP)
                    totalQ += Q
                    totalDemand += demand
                    
                    Q = 0
                    demand = 0
                    GWHP = {}
                    pxmin =  -1
                    pymin =  -1
                    pxmax =  -1
                    pymax =  -1
                    id +=1
                    
                    
            if grouping == True:
                Q = self.calculateWaterAmount(demand, 3)
                GWHP["Q"] = Q
                totalQ += Q
                totalDemand += demand
                GWHPs.append(GWHP)
                Q = 0
                demand = 0
        #Random Sites
        random.seed()

        GHWPsnew = []
        l1 = len(PriorityMap)
        for p in range(0,l1,1):
        #for p in range(l1-1,0, -1):
            nv = []
            i = 0
            for GWHP in PriorityMap[p]:            
                nv.append(i)
                i += 1
            for GWHP in PriorityMap[p]:
                c = len(nv)-1            
                n = nv[random.randint(0,c)]
                GHWPsnew.append(PriorityMap[p][n])
                nv.remove(n)
        counter = 0
        Fields = []
        GWHPs = GHWPsnew
        palcedDemand = 0
        palcedQ = 0
        totalQTest = 0
        for GWHP in GWHPs:   
            totalQTest +=      GWHP["Q"]
        for GWHP in GWHPs:       
            I = self.I
            kf = self.kf
            kfHzuV = 2
            TempR = 5 
            BD = 2000
            Dist = 10                 
            
            l , b = self.t.calculate(I, kf, kfHzuV, GWHP["Q"], TempR, BD, Dist) 
            if (l < 1 or b < 1):
                break
            h = self.calcuateHydraulicEffectedArea(GWHP["Q"], kf, I, kfHzuV)
            #Create PossiblePointsList
            PList = []
            dx = (GWHP["pxmax"]-GWHP["pxmin"]) / 10
            dy = (GWHP["pymax"]-GWHP["pymin"]) / 10
            for i in range( 10 ):
                for j in range( 10 ):
                    PList.append([GWHP["pxmin"]+dx*i, GWHP["pymin"]+dy*j])
            i = 0
            nv = []
            for P in PList:            
                nv.append(i)
                i += 1                   
            PListnew = []
            for P in PList:
                c = len(nv)-1            
                n = nv[random.randint(0,c)]
                PListnew.append(PList[n])
                nv.remove(n)   
                
            PList = PListnew
            
            intersects = True
            controller = 0
            while intersects == True and controller < len(PList):
                intersects = False
                #print PList[controller]
                pxmin = PList[controller][0]
                pymin = PList[controller][1]
                #Place Injection Well    
                controller +=1
            
    
                RegField = Polygon.Polygon(((pxmin,pymin - b/2. ),  (pxmin,pymin + b/2.) ,  (pxmin+l,pymin + b/2.),  (pxmin+l,pymin - b/2.)))
                            
                pl = PointList()
                el = EdgeList()    
                for p in pointList(RegField):
                    pl.append(Point(p[0], p[1], 0))
                for i in range(1, len(pl)):
                    el.append(Edge(i-1,i))
                el.append(Edge(len(pl)-1, 0))       
                
                plReg = pl
                elReg = el   
                
                #HydraulicField    
                pxmin =  pxmin - h                
                HydraulicField1 = Polygon.Polygon(((pxmin,pymin - h ),  (pxmin,pymin + h) ,  (pxmin+h,pymin +h),  (pxmin+h,pymin - h)))
                            
                pl = PointList()
                el = EdgeList()    
                for p in pointList(HydraulicField1):
                    pl.append(Point(p[0], p[1], 0))
                for i in range(1, len(pl)):
                    el.append(Edge(i-1,i))
                el.append(Edge(len(pl)-1, 0))     
                        
                elP1 = pl
                elH1 = el
    
                HydraulicField2 = Polygon.Polygon(((pxmin-h/2,pymin - h/2 ),  (pxmin-h/2,pymin + h/2) ,  (pxmin+h/2,pymin +h/2),  (pxmin+h/2,pymin - h/2)))
                            
                pl = PointList()
                el = EdgeList()    
                for p in pointList(HydraulicField2):
                    pl.append(Point(p[0], p[1], 0))
                for i in range(1, len(pl)):
                    el.append(Edge(i-1,i))
                el.append(Edge(len(pl)-1, 0))
                
                elP2 = pl
                elH2 = el
                        
                #Check If Placable
    
                for f in Fields:
                        intersects = False
                        if (f.overlaps(RegField) == True):
                                intersects = True
                                break
                        if (f.overlaps(HydraulicField1) == True):
                                intersects = True
                                break                           
                        if (f.overlaps(HydraulicField2) == True):
                                intersects = True
                                break
            
                
                
            if  intersects == False:   
                #print "place"
                #print l 
                #print b
                Fields.append(RegField)
                Fields.append(HydraulicField1)
                Fields.append(HydraulicField2)
                if self.PlotResults == True:
                    p = Point(pxmin+h, pymin, 0)
                    
                    t = vibehelper.DrawTemperaturAnomaly(p, l, h, b, 5)
                    #vec.setVectorData("GHWPRegField"+house, plReg)
                    #vec.setEdges("GHWPRegField"+house, elReg)
                    #vec.setVectorData("GHWPHydraulicField1"+str(counter), elP1)
                    #vec.setEdges("GHWPHydraulicField1"+str(counter), elH1)                    
                    #vec.setVectorData("GHWPHydraulicField2"+str(counter), elP2)
                    #vec.setEdges("GHWPHydraulicField2"+str(counter), elH2)

                    vec.setVectorData("WF"+str(counter), t.getVectorData("WF_above"))
                    vec.setFaces("WF" + str(counter), t.getFaces("WF_above"))
                    vec.setVectorData("COLOR_WF" + str(counter), t.getVectorData("COLOR_WF_above"))
                    vec.setVectorData("WF_side"+str(counter), t.getVectorData("WF_above_side"))
                    vec.setFaces("WF_side" + str(counter), t.getFaces("WF_above_side"))
                    vec.setVectorData("COLOR_WF_side" + str(counter), t.getVectorData("COLOR_WF_above_side"))
                    #vec.setVectorData("WF3D"+str(counter), t.getVectorData("WF"))
                    #vec.setFaces("WF3D" +str(counter), t.getFaces("WF"))
                palcedDemand += GWHP["Demand"]
                palcedQ += GWHP["Q"]
                counter = counter + 1
                    
            else:
                pass

        if self.PlotResults == True:        
            self.setVectorData(self.Anomalies, vec)                                

        FILE = open( self.FileName ,"a")
        
        FILE.write(str(GP) + "\t" + str(NAHouses) + "\t" + str(totalArea)+"\t" +str(totalDemand) + "\t" + str(palcedDemand) + "\t" + str(totalQ)  + "\t" +str(totalQTest)+"\t" + str(palcedQ)+ "\n" )
        FILE.close() 
        self.usable  = palcedDemand/totalDemand
        
    def calculateHeatingDemand(self, area, year):
        fw = 75.        #W/m
        flowT = 60.     #°C
            
        if (year > 1960):
            fw = 75.        #W/m
            flowT = 60.     #°C
        if (year > 1995):
            fw = 50.        #W/m
            flowT = 50.     #°C
        if (year > 2000):
            fw = 15;        #W/m
            flowT = 30;     #°C

                    
        return area * fw
            
            
    def calculateWaterAmount(self, demandHeating, deltaT):
            cvw = 4190000.
            A = demandHeating            
            return A/(cvw*deltaT)
            
    def calcuateHydraulicEffectedArea(self, Q, kf, IG, kfhTokfh):
            return 2+3 * pow(Q /(kf*IG*pi) * sqrt( kfhTokfh ) , (1./3.))
            
    def clone(self):
            return CreateGWHP()


