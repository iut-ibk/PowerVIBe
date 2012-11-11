# -*- coding: utf-8 -*-
"""
Created on Sun Nov 11 19:43:39 2012

@author: christianurich
"""


from pydmtoolbox import *
import pydmextensions
from pydynamind import *

from collada import *
import matplotlib.delaunay as triang
import numpy as np
import math


import sys

class TestComponent(Module):  
    def __init__(self):        
        Module.__init__(self)
        self.ops = View("TEST", COMPONENT, WRITE)
        self.ops.addAttribute("Geometry")
        self.geometry = View("Geomtry", FACE, WRITE)
        self.geometry.addAttribute("type")

        datastream = []
        datastream.append(self.ops)
        datastream.append(self.geometry)

        self.addData("City", datastream)    
    def run(self):
        city = self.getData("City")
        center = Node(0,0,0)
        nodes_circle = TBVectorData_CreateCircle(center, 2.5, 8)
        nodes = nodevector()
        for n in nodes_circle:
            nodes.push_back(city.addNode(n.getX(), n.getY(), n.getZ()))
        nodes.push_back(nodes[0])
        f = city.addFace(nodes,self.geometry)
        cmp = Component()
        cmp = city.addComponent(cmp, self.ops)
        print "Component UUID"
        print cmp.getUUID()
        print "Center UUID"
        print center.getUUID()
        cmp.getAttribute("Geometry").setLink("Geometry", f.getUUID())
        
        
        #to.getAttribute("Geometry").setLink("Geometry", f.getUUID())