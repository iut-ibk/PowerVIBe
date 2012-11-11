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

from math import *


class GroundSourceHeatPumpSystems(Module):
    def __init__(self):
        Module.__init__(self)
        self.view_parcel = View("PARCEL", FACE, READ)
        
        self.view_building = View("BUILDING", COMPONENT, READ)
        self.view_building.getAttribute("monthly_peak_heating_demand")
        self.view_building.getAttribute("anual_heating_demand") 
        self.view_building.addAttribute("Option_Heating_Cooling")
        self.view_building.getAttribute("PARCEL")
        
        self.view_technology = View("Option_Heating_Cooling", COMPONENT, WRITE)
        self.view_technology.addAttribute("type")
        self.view_technology.addAttribute("Geometry")
        
        self.view_geometry= View("GWHP", FACE, WRITE)
        self.view_geometry.addAttribute("type")

        
        datastream = []
        datastream.append(self.view_building)
        datastream.append(self.view_parcel)
        datastream.append(self.view_technology)
        datastream.append(self.view_geometry)
        self.addData("city", datastream)
        
        
    def run(self):
        city = self.getData("city")
        conductivity = 2.55 
        building_uuids = city.getUUIDs(self.view_building)
        for building_uuid in building_uuids:
            building = city.getComponent(building_uuid)
            hd_peak = building.getAttribute("monthly_peak_heating_demand").getDouble()
            hd_anual = building.getAttribute("anual_heating_demand").getDouble()
            surfaceT = self.groundSurfaceT(500)
            op_h = self.operatingHours(hd_peak, hd_anual)
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
            
            #print "Calculation Parameter " +  "peak " + str(hd_peak/1000) + "w/m " + str(w_m) + "op_h " + str(op_h) + "l1 " + str(l_10_1) +" " + str(self.temperaturPenalty(l_10_1, surfaceT)) +" " + str( surfaceT)
            #print "GSHP Length " + "1 " + str(l_1)  + "2 " + str(l_2)  + "3 " + str(l_3)  + "4 " + str(l_4) + "2x2 " + str(l_2x2) 
            
            #Thermanl Effected Area
            #Check Options
            parcel = city.getFace(building.getAttribute("PARCEL").getLink().uuid)
            nodes_parcel = TBVectorData_getNodeListFromFace(city, parcel)
            node_boundingbox =  pydmextensions.nodevector_obj()
            b_size = doublevector()
            angle = pydmextensions.CGALGeometry_CalculateMinBoundingBox(nodes_parcel, node_boundingbox, b_size) /180 * pi
            center = TBVectorData_CaclulateCentroid(city, parcel)
            print str(b_size[0]) + " " +  str(b_size[1])
            #We want to stay on our parcel. Min distance up from the parcel is 2.5 m
            l = b_size[0] - 5.
            b = b_size[0] - 5.
            
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
            print "circle " + str(e.getX())
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
            
    
    
