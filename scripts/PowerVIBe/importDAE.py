# -*- coding: utf-8 -*-
"""
Created on Thu Nov 15 10:00:56 2012

@author: christianurich
"""

import collada

from pydynamind import *

class ImportDAE(Module):
    def __init__(self):
            Module.__init__(self)
            self.createParameter("FileName", FILENAME, "filename")
            self.FileName = ""
            self.dummy = View("dummy", SUBSYSTEM, MODIFY)
            self.object = View("Objects", COMPONENT, WRITE)
            self.geometry = View("Geometry", FACE, WRITE) 
            self.datastream = []
            self.datastream.append(self.object)
            self.datastream.append(self.geometry)
            self.datastream.append(self.dummy)
            self.createParameter("OffsetX", DOUBLE, "offsetx")
            self.OffsetX = 0.0   
            self.createParameter("OffsetY", DOUBLE, "offsety")
            self.OffsetY = 0.0
            self.createParameter("ScaleX", DOUBLE, "scalesetx")
            self.ScaleX = 1.0   
            self.createParameter("ScaleY", DOUBLE, "scalesety")
            self.ScaleY = 1.0
            self.createParameter("ScaleZ", DOUBLE, "scalesetz")
            self.ScaleZ = 1.0   

            self.addData("sys", self.datastream)
            
            
    def run(self):
        sys = self.getData("sys")
        cmp = Component()
        cmp = sys.addComponent(cmp, self.object)
        
        print self.FileName
        col = collada.Collada(self.FileName, ignore=[collada.DaeUnsupportedError, collada.DaeBrokenRefError])
        for geom in col.geometries:
            for triset in geom.primitives:
                trilist = list(triset)
                elements = len(trilist)
                for i in range(elements):
                    nl = nodevector()
                    for j in range(3):                   
                        node = triset.vertex[triset.vertex_index][i][j]
                        n = sys.addNode(float(node[0]* self.ScaleX + self.OffsetX), float(node[1]* self.ScaleY+self.OffsetY), float(node[2])* self.ScaleZ)
                        nl.append(n)
                    nl.append(nl[0])
                    f = sys.addFace(nl, self.geometry)
                    cmp.getAttribute("Geometry").setLink("Geometry", f.getUUID())
                        
                        

    
     