# -*- coding: utf-8 -*-
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


from pydmtoolbox import *
import pydmextensions
from pydynamind import *
from random import randrange
from math import *


class GroundSourceHeatPumpSystems(Module):
    def __init__(self):
        Module.__init__(self)
        self.view_parcel = View("PARCEL", FACE, READ)
        self.view_parcel.getAttribute("conductivity")
        self.view_parcel.addAttribute("gshp_factor_extern")
        self.view_parcel.addAttribute("gshp_factor_intern")
        self.view_parcel.addAttribute("reduction_ground_temperatur")
        self.view_building = View("BUILDING", COMPONENT, READ)
        self.view_building.getAttribute("dayly_peak_heating_demand")
        self.view_building.getAttribute("anual_heating_demand") 
        self.view_building.addAttribute("GSHP")
        self.view_building.getAttribute("PARCEL")
        
        
        self.view_technology = View("GSHP", COMPONENT, WRITE)
        self.view_technology.addAttribute("type")
        self.view_technology.addAttribute("configuration")
        self.view_technology.addAttribute("Borehole")
        
        self.createParameter("reportfile",STRING, "reportfile")
        self.reportfile = ""

        self.createParameter("min_distance",DOUBLE, "min_distance")
        self.min_distance = 20
        
        self.view_borehole = View("Borehole", NODE, WRITE)
        self.view_borehole.addAttribute("heat_extraction")     
        self.view_borehole.addAttribute("anual_average_heat_extraction")
        self.view_borehole.addAttribute("length")
        self.view_borehole.addAttribute("surface_temperature")
        
        datastream = []
        datastream.append(self.view_building)
        datastream.append(self.view_parcel)
        datastream.append(self.view_technology)
        datastream.append(self.view_borehole)
        self.addData("city", datastream)
        self.singleRefT = 7.67
        self.singleRefW = 6.7
        self.singelDeltaT = [(0.0, 3.53), 
                 (0.5,2.29),
                 (1.0,2.01),
                 (1.5,1.80),
                 (2.0,1.65),
                 (2.5, 1.53),
                 (3.0, 1.44),
                 (3.5, 1.36),
                 (4.0, 1.28),
                 (4.5, 1.22),
                 (5.0, 1.17),
                 (7.5, 0.96),
                 (12.5, 0.70),
                 (20.0, 0.48),
                 (30.0, 0.30),
                 (42.5, 0.17),
                 (57.5, 0.09),
                 (75.0, 0.04),
                 (95.0, 0.01),
                 (117.5, 0.00)]
                 
    def interpolationSonde(self, x):
        if x > 95:
            return 0
        sondenDeltaT= self.singelDeltaT
        s_0 = (0,0)
        s_1 = (0,0)
        

        for i in range(1, len(sondenDeltaT) ):
            if sondenDeltaT[i][0] > x:
                s_0 = sondenDeltaT[i-1]
                s_1 = sondenDeltaT[i]
                break
        y = (s_1[1] - s_0[1])/(s_1[0] - s_0[0]) * (x-s_0[0]) + s_0[1]
        return y/6.7 
    def transformNodes(self, nodes, center, alpha):
        new_nodes = []
        for n in nodes:
            n_x = n[0] * cos(alpha) - n[1] * sin(alpha) + center.getX()
            n_y = n[0] * sin(alpha) + n[1] * cos(alpha) + center.getY()
            new_nodes.append([n_x, n_y])
        return new_nodes
    def checkNeigbours(self, sonden_tot, sonden):
        for n_e in sonden_tot:
            for s in sonden:
                d = self.caclulateDistance(n_e,s)
                if d < self.min_distance:
                    return False
        return True
    def sonden_in_reach(self, sonden_tot,  center, l, b, alpha):
        X1 =  (l) * cos(alpha) - (b) * sin(alpha) + center.getX() - 100       
        Y1 =  (l) * sin(alpha) + (b) * cos(alpha) + center.getY() - 100
        
        X2 =  (l) * cos(alpha) - (b) * sin(alpha) + center.getX() + 100       
        Y2 =  (l) * sin(alpha) + (b) * cos(alpha) + center.getY() + 100 

        sinr = []
        for s in sonden_tot:
            if X1 > s.getX():
                continue
            if Y1 > s.getY():
                continue
            if X2 < s.getX():
                continue
            if Y2 < s.getY():
                continue
            sinr.append(s)
        return sinr

    def poosibleConfigurations(self, sonden_tot, length, T_mean_ground, center, l, b, alpha, average_w_m):
        from_parcel = 4
        distances = [5,7.5, 10,20,30]
        results = []
        results_recs = []
        sir = self.sonden_in_reach( sonden_tot, center, l, b,alpha )
        print len(sir)

     
        for d in distances:
            
            X = int((l-2*from_parcel)/d+1)
            Y = int((b-2*from_parcel)/d+1)
            
            if X > 10:
                X = 10
            if Y > 10:
                Y = 10
            print "check configuratins for distance " ,d, X, Y
            if X < 0 or Y < 0:
                print "failed"
                return [],[]
            #single
            rec_s = self.transformNodes(self.rectangular(1,1,10), center, alpha)
            if self.checkNeigbours(sir, rec_s) == False:
                continue
            results_recs.append(rec_s)
            results.append(self.calulcateAverageDeltaT(sir, rec_s, average_w_m))
            #lines
            for x in range(1,X+1):                
                rec_s = self.transformNodes(self.rectangular(x,1,d), center, alpha)
                if self.checkNeigbours(sir, rec_s) == False:
                    continue
                results_recs.append(rec_s)
                results.append(self.calulcateAverageDeltaT(sir, rec_s, average_w_m))

            for y in range(1,Y+1):
                rec_s = self.transformNodes(self.rectangular(1,y,d), center, alpha)
                if self.checkNeigbours(sir, rec_s) == False:
                    continue
                results_recs.append(rec_s)
                results.append(self.calulcateAverageDeltaT(sir, rec_s, average_w_m))
            #Squares
            for x in range(1,X+1):
                for y in range(1,Y+1):     
                    rec_s = self.transformNodes(self.rectangular(x,y,d), center, alpha)
                    if self.checkNeigbours(sir, rec_s) == False:
                        continue
                    results_recs.append(rec_s)
                    results.append(self.calulcateAverageDeltaT(sir, rec_s, average_w_m))
                    

        con = -1
        l_tot_min = -1
        for c in range(len(results)):
            n_s = len(results_recs[c])
            
            factor_total = results[c][0] + results[c][2]

            l_tot = length * factor_total
            
   
            
            #PossibleConfiguration
            if (l_tot/n_s < 200 and l_tot/n_s > 50 and factor_total < 5):
                if l_tot_min < 0 or l_tot_min > l_tot:
                    l_tot_min = l_tot
                    con = c
                    
                
        if con == -1:
            print "failed"
            return [],[]
        
        #print "Picked ", results_recs[con], results[con]
        return results_recs[con], results[con]
                
    def rectangular(self, X,Y,a):
        sonden = []

        for x in range(X):
            for y in range(Y):
                sonden.append((x*a - (X-1)*a/2., y*a-(Y-1)*a/2.))
        return sonden
    def caclulateDistance(self, n, n1):
            dx = n.getX() - n1[0]
            dy = n.getY() - n1[1]            
            d = sqrt(dx*dx+dy*dy)
            return d
    """Return internal factos and external temperatur reduction at borehole"""
    def calulcateAverageDeltaT(self, sonden_tot, sonden, w_m):
        external_influence = []
        internal_influence = []
        deltaT = 0
        refT = self.interpolationSonde(0)
        refT_w_m = refT*w_m
        for s in sonden:
            external_influence.append(self.calculateTtoPositions(sonden_tot, s)/refT_w_m)

        for s_0 in sonden:        
            ls = []
            for s_1 in sonden:
                s = (s_0[0] - s_1[0], s_0[1] - s_1[1])
                ls.append(sqrt(s[0]*s[0]+s[1]*s[1]))        
            deltaT = 0            
            for l in ls:
                deltaT+=self.interpolationSonde(l)/refT
                
            internal_influence.append(deltaT)
        #print "internal ",internal_influence
        mean_in= 0
        for a in internal_influence:
            mean_in+=a
        mean_in/=len(internal_influence)
        
        mean_ex= 0
        #print "external ",external_influence
        for a in external_influence:
            mean_ex+=a
        mean_ex/=len(external_influence)
        
        return mean_in, internal_influence, mean_ex, external_influence
            
        return 0        
    """delta T
    """    
    def calculateTtoPositions(self, sonden_tot, n):
        #self.interpolationSonde(0)
        deltaSourceT = 0
        for n_e in sonden_tot:
            d = self.caclulateDistance(n_e,n)
            delta = self.interpolationSonde(d) * n_e.getAttribute("anual_average_heat_extraction").getDouble()/n_e.getAttribute("length").getDouble()
            deltaSourceT+=delta
        return deltaSourceT
            
    def run(self):
        city = self.getData("city")
        conductivity = 2.55 
        building_uuids = city.getUUIDs(self.view_building)
        distanceToNeighbour = 20.
        
        ex_borehole_uuids = city.getUUIDs(self.view_borehole)
        ex_boreholes = []
        #for borehole_uuid in ex_borehole_uuids:
            #ex_boreholes.append(city.getNode(borehole_uuid))
        #mix vector
        for r in range(len(building_uuids)):
            sw_1 = randrange(len(building_uuids))
            sw_2 = randrange(len(building_uuids)) 
            uuid_sw1 = building_uuids[sw_1]
            uuid_sw2 = building_uuids[sw_2]
            building_uuids[sw_1] = uuid_sw2
            building_uuids[sw_2] = uuid_sw1
        
        counter_done = 0
        for building_uuid in building_uuids:
            counter_done+=1
            print counter_done, len(building_uuids) 
            building = city.getComponent(building_uuid)
            hd_peak = building.getAttribute("dayly_peak_heating_demand").getDouble()
            hd_anual = building.getAttribute("anual_heating_demand").getDouble()
            surfaceT = self.groundSurfaceT(500)
            op_h = self.operatingHours(hd_peak, hd_anual)
            w_m = self.wattperMeter(conductivity)
            
            #Erhöhung des Peak demandes laut ÖWAV Regelblatt 
            op_penalty = op_h/1800.
            
            l_10_1 = hd_peak / w_m       
            
            #op_penalty =   self.opertionalHourPenalty(op_h, 5)
            #print  op_penalty
            #print w_m
            l_1 = l_10_1 * self.temperaturPenalty(l_10_1, surfaceT) * op_penalty#op_penalty[0]
            #print l_1
            
            
            #Thermanl Effected Area
            #Check Options
            parcel = city.getFace(building.getAttribute("PARCEL").getLink().uuid)
            nodes_parcel = TBVectorData_getNodeListFromFace(city, parcel)
            node_boundingbox =  pydmextensions.nodevector_obj()
            b_size = doublevector()
            angle = pydmextensions.CGALGeometry_CalculateMinBoundingBox(nodes_parcel, node_boundingbox, b_size) /180 * pi
            #center = TBVectorData_CaclulateCentroid(city, parcel)
            center = Node(building.getAttribute("centroid_x").getDouble(), building.getAttribute("centroid_y").getDouble(),0.0)
            #print str(b_size[0]) + " " +  str(b_size[1])

            #alpha = angle/180.*pi
            #print "angle", angle
            #print "alpha", alpha
            #We want to stay on our parcel. Min distance up from the parcel is 2.5 m
            l = b_size[0]
            b = b_size[1]
            
            w_h_anual = self.temperaturPenalty(l_10_1, surfaceT) * op_h * w_m / (24.*365.) 
            #print "w_m", w_h_anual
            sonden, leistung = self.poosibleConfigurations(ex_boreholes, l_1, 7.67, center, l, b, angle, w_h_anual)
            #print "Leistung", leistung
            if len(sonden) == 0:
                continue
            new_boreholes = self.place_sonde(city, l_1, sonden, leistung, hd_anual,parcel)
            for b in new_boreholes:
                ex_boreholes.append(b)
        #self.updateParcelFactors(city)
        self.calculatePlacement(city)

            
            
    def place_sonde(self, city, length, sonden, leistung, anual_heat_ex, parcel):
        """self.view_technology = View("GSHP", COMPONENT, WRITE)
        self.view_technology.addAttribute("type")
        self.view_technology.addAttribute("configuration")
        self.view_technology.addAttribute("Borehole")
        
        #self.view_geometry= View("GSHP", FACE, WRITE)
        #self.view_geometry.addAttribute("type")

        self.view_borehole = View("Borehole", NODE, WRITE)
        self.view_borehole.addAttribute("heat_extraction")     
        self.view_borehole.addAttribute("anual_average_heat_extraction")
        self.view_borehole.addAttribute("length")
        self.view_borehole.addAttribute("surface_temperature")"""
        length = length * leistung[0]
        boreholes = []
        gshp = Component()
        gshp = city.addComponent(gshp, self.view_technology)
        for i in range(len(sonden)):
            borehole = city.addNode(sonden[i][0], sonden[i][1], 0, self.view_borehole)
            #print leistung[i]
            #print anual_heat_ex
            heatextraction = anual_heat_ex*(leistung[0] + leistung[2])/(leistung[1][i]  + leistung[3][i])/len(sonden)
            borehole.addAttribute("anual_average_heat_extraction", heatextraction/(365.*24.))
            borehole.addAttribute("length", length/len(sonden))
            borehole.addAttribute("fi_intern", leistung[0])
            borehole.addAttribute("fi_extern", leistung[2])
         
            boreholes.append(borehole)


            gshp.getAttribute("Borehole").setLink("Borehole", borehole.getUUID())
        parcel.addAttribute("gshp_factor_extern",leistung[2])
        parcel.addAttribute("gshp_factor_intern",leistung[0])
        parcel.getAttribute("GSHP").setLink("GSHP", gshp.getUUID())
        return boreholes      
            
    def calculatePlacement(self, city):
    

        building_uuids = city.getUUIDs(self.view_building)

        
        ex_borehole_uuids = city.getUUIDs(self.view_borehole)

        anual_heating_demand = 0
        extraction = 0
        fi_intern = 0
        fi_extern = 0
        length = 0
        
        for building_uuid in building_uuids:
            building = city.getComponent(building_uuid)
            hd_anual = building.getAttribute("anual_heating_demand").getDouble()
            anual_heating_demand+=hd_anual
        
        for b_uuid in ex_borehole_uuids:
            b = city.getComponent(b_uuid)
            extraction+=b.getAttribute("anual_average_heat_extraction").getDouble()*365*24.
            b.getAttribute("length").getDouble()
            fi_intern+=b.getAttribute("fi_intern").getDouble()/len(ex_borehole_uuids)
            fi_extern+=b.getAttribute("fi_extern").getDouble()/len(ex_borehole_uuids)
        print "tot_fix",     fi_intern, fi_extern
        print "anual_heating_demand", anual_heating_demand, extraction
        print length
        
        if self.reportfile == "":
            return
        with open(self.reportfile, 'a') as f:
            f.write(str(anual_heating_demand) + "\t")
            f.write(str(extraction) + "\t")
            f.write(str(fi_intern) + "\t")
            f.write(str(fi_extern) + "\t")      
            f.write("\n")
                       
    def updateParcelFactors(self, city):
        parcel_uuids = city.getUUIDs(self.view_parcel)
        
        for p_uuid in parcel_uuids:
            current_p = city.getFace(p_uuid)
            center_p = TBVectorData_CaclulateCentroid(city, current_p)
            sonden_n = []
            for p_b_uuid in parcel_uuids:
                if p_uuid == p_b_uuid:
                    continue
                b_p = city.getComponent(p_b_uuid)
                l_gshp = b_p.getAttribute("GSHP").getLinks()
                for l_g in l_gshp:
                    l_boreholes = city.getComponent(l_g.uuid).getAttribute("Borehole").getLinks()
                    for l_b in l_boreholes:
                        sonden_n.append(city.getNode(l_b.uuid))
            s = [center_p.getX(), center_p.getY()]            
            tred = self.calculateTtoPositions(sonden_n, s)
            print tred
            current_p.addAttribute("reduction_ground_temperatur", tred)
        
    """ SIA page 33 H in m
    """
    def groundSurfaceT(self, H):
        return 1.373e-6 * H * H - 7.11e-3 * H + 13.2
    def wattperMeter(self, conductivity):
        	return -0.7143*conductivity*2 + 14.5*conductivity + 6.2143
  
    def createGeometry(self, city, cmp, e_nodes):
        for e in e_nodes:                                     
            nodes_circle = TBVectorData_CreateCircle(e, 2.5, 8)
            nodes = nodevector()
            for n in nodes_circle:
                nodes.push_back(city.addNode(n.getX(), n.getY(), n.getZ()))
            nodes.push_back(nodes[0])
            #print "circle " + str(e.getX())
            f = city.addFace(nodes,self.view_geometry)
            cmp.getAttribute("Geometry").setLink("Geometry", f.getUUID())        
                
    def operatingHours(self, monthly_h_peak, anual_h):
        return anual_h/monthly_h_peak
    """Excerpted from SIA """

    def opertionalHourPenalty(self, op_h, distance):
        op_h2 = op_h * op_h
        if distance == 5:
            gshp_1 = 0
            if op_h >= 2100:
                gshp_1 = -4e-7*op_h2 + 0.0171*op_h - 32.581                
            gshp_2 = -6e-6*op_h2 + 0.0537*op_h - 77.269  
            gshp_3 = -6e-6*op_h2 + 0.06*op_h - 77.275  
            gshp_4 = -6e-6*op_h2 + 0.0612*op_h - 77.432
            gshp_2x2 = 7e-7*op_h2 + 0.0319*op_h - 38.763
        
        penalties = []
        penalties.append(1+gshp_1/100.)
        penalties.append(1+gshp_2/100.)
        penalties.append(1+gshp_3/100.)
        penalties.append(1+gshp_4/100.)
        penalties.append(1+gshp_2x2/100.)
        return penalties
    """ SIA 1.6 """
    def temperaturPenalty(self, length_10, surfaceT):
        deltaT_design = -1.5
        deltaT_g = 0.03
        deltaT_10_100 = 10 + (100 * deltaT_g) / 2. - deltaT_design
        deltaT = surfaceT + (length_10 * deltaT_g) / 2. - deltaT_design
        
        return deltaT_10_100 / deltaT

    def calculateSIA():
            w_m = self.wattperMeter(conductivity)
            
            
            l_10_1 = hd_peak / w_m       
            l_10_2 = l_10_1 / 2.
            l_10_3 = l_10_1 / 3.
            l_10_4 = l_10_1 / 4.

            op_penalty =   self.opertionalHourPenalty(op_h, 5)

            
            l_1 = l_10_1 * self.temperaturPenalty(l_10_1, surfaceT) * op_penalty[0]
            l_2 = l_10_2 * self.temperaturPenalty(l_10_2, surfaceT) * op_penalty[1]
            l_3 = l_10_3 * self.temperaturPenalty(l_10_3, surfaceT) * op_penalty[2]
            l_4 = l_10_4 * self.temperaturPenalty(l_10_4, surfaceT) * op_penalty[3]
            l_2x2 = l_10_4 * self.temperaturPenalty(l_10_4, surfaceT) * op_penalty[4]
            
            #Select possible options
            if l < 0 or b < 0:
                print "no option place able"
            if l >= 5 and b >= 5:
                if l_2x2 > 50 and l_2x2 < 300:
                    print "can place 2x2"
                    cmp = Component()
                    to = city.addComponent(cmp, self.view_technology)
                    to.addAttribute("type", "gshp")
                    to.addAttribute("probes_x", 2)
                    to.addAttribute("probes_y", 2)
                    to.addAttribute("probes_l", l_2x2)
                    building.getAttribute("Option_Heating_Cooling").setLink(self.view_technology.getName(), to.getUUID())    
                    
                    #Create the thermal effected areas
                    n1 = Node(2.5*cos(angle) + center.getX(), 2.5*cos(angle) + center.getY(),0)
                    n2 = Node(-2.5*cos(angle) + center.getX(), 2.5*cos(angle) + center.getY(),0)
                    n3 = Node(-2.5*cos(angle) + center.getX(), -2.5*cos(angle) + center.getY(),0)
                    n4 = Node(2.5*cos(angle) + center.getX(), -2.5*cos(angle) + center.getY(),0)
                    
                    e_nodes = [n1, n2, n3, n4]
                    print n1.getX()
                    print n2.getX()
                    print n3.getX()
                    print n4.getX()
                    
                    print n1.getY()
                    print n2.getY()
                    print n3.getY()
                    print n4.getY()
                    self.createGeometry(city, to, e_nodes)

                        
            if l >= 15:
                if l_4 > 50 and l_4 < 300:
                    print "can place 4" 
                    cmp = Component()
                    to = city.addComponent( cmp, self.view_technology)
                    to.addAttribute("type", "gshp")
                    to.addAttribute("probes_x", 4)
                    to.addAttribute("probes_l", l_4)
                    building.getAttribute("Option_Heating_Cooling").setLink(self.view_technology.getName(), to.getUUID()) 
                    #Create the thermal effected areas
                    n1 = Node(7.5*cos(angle) + center.getX(), center.getY(),0)
                    n2 = Node(2.5*cos(angle) + center.getX(), center.getY(),0)
                    n3 = Node(-2.5*cos(angle) + center.getX(), center.getY(),0)
                    n4 = Node(-7.5*cos(angle) + center.getX(), center.getY(),0)                  
                    e_nodes = [n1, n2, n3, n4]
                    self.createGeometry(city,to, e_nodes)                    
                    
                    
                    
            if l >= 10:
                if l_3 > 50 and l_3 < 300:
                    print "can place 3"     
                    cmp = Component()
                    to = city.addComponent(cmp, self.view_technology)
                    to.addAttribute("type", "gshp")
                    to.addAttribute("probes_x", 3)
                    to.addAttribute("probes_l", l_3)
                    building.getAttribute("Option_Heating_Cooling").setLink(self.view_technology.getName(), to.getUUID()) 
                    #Create the thermal effected areas
                    n1 = Node(5.0*cos(angle) + center.getX(), center.getY(),0)
                    n2 = Node(0*cos(angle) + center.getX(), center.getY(),0)
                    n3 = Node(-5.0*cos(angle) + center.getX(), center.getY(),0)            
                    e_nodes = [n1, n2, n3]
                    self.createGeometry(city,to, e_nodes)                        
                    
            if l >= 5:
                if l_2 > 50 and l_2 < 300:
                    print "can place 2"   
                    cmp = Component()
                    to = city.addComponent(cmp, self.view_technology)
                    to.addAttribute("type", "gshp")
                    to.addAttribute("probes_x", 2)
                    to.addAttribute("probes_l", l_2)
                    building.getAttribute("Option_Heating_Cooling").setLink(self.view_technology.getName(), to.getUUID()) 
                    #Create the thermal effected areas
                    n1 = Node(2.5*cos(angle) + center.getX(), center.getY(),0)
                    n2 = Node(-2.5*cos(angle) + center.getX(), center.getY(),0)            
                    e_nodes = [n1, n2]
                    self.createGeometry(city,to, e_nodes)     
            if l >= 1:
                if l_1 > 50 and l_1 < 300:
                    print "can place 1" 
                    cmp = Component()
                    to = city.addComponent(cmp, self.view_technology)
                    to.addAttribute("type", "gshp")
                    to.addAttribute("probes_x", 1)
                    to.addAttribute("probes_l", l_1)
                    building.getAttribute("Option_Heating_Cooling").setLink(self.view_technology.getName(), to.getUUID())             
                    
                    #Create the thermal effected areas
                    n1 = Node(0*cos(angle) + center.getX(), center.getY(),0)        
                    e_nodes = [n1]
                    self.createGeometry(city,to, e_nodes)                        
    
    
