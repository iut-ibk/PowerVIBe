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


from pydmtoolbox import *
import pydmextensions
from pydynamind import *

from collada import *
import matplotlib.delaunay as triang
import numpy as np
import math


import sys

class CreateDAE(Module):        
    def __init__(self):
        Module.__init__(self)
        self.createParameter("ViewName", STRING, "ViewName")
        self.ViewName = ""
        self.createParameter("Filename", STRING, "Filename")
        self.Filename = ""
        self.createParameter("Type", STRING, "Type")
        self.Type = "COMPONENT"
        self.buildings = View("dummy", SUBSYSTEM, READ)
        datastream = []
        datastream.append(self.buildings)
        self.addData("City", datastream)
            
    def createMaterial(self, mesh, face,id):
        r = 0.0
        b = 1.0
        g = 0.0
        rgbVector = face.getAttribute("color").getDoubleVector()
        if len(rgbVector) == 3:
          r = rgbVector[0]
          b = rgbVector[1]
          g = rgbVector[2]
        effect = material.Effect("effect"+str(id), [], "phong", diffuse=(r,g,b), specular=(r,g,b))
        #effect = material.Effect("effect"+str(id), [], "phong",  ambient=(r,g,b,1))
        mat = material.Material("material"+str(id), "mymaterial"+str(id), effect)
        mesh.effects.append(effect)
        mesh.materials.append(mat)
        return mat


    def createDAE_KML(self, city, uuid,  objects):
        print "create "  + str(uuid)
        mesh = Collada()
        scenenodes = []
        objind = 0
        for obj in objects:
            f = city.getFace(obj)

            #trangulate face
            triangles = pydmextensions.CGALGeometry_FaceTriangulation(city, f)

            vert_floats = []
            normal_floats = []
            x = []
            y = []
            z = []
            print len(triangles)
            for n in triangles:
                x.append(n.getX())
                y.append(n.getY())
                z.append(n.getZ())
                vert_floats.append(n.getX())
                vert_floats.append(n.getZ())
                vert_floats.append(n.getY())
                normal_floats.append(0)
                normal_floats.append(1)
                normal_floats.append(0)

            findex = []
            for i in range(len(triangles)/3):
                index = i*2
                t = [i*3+0, i*3+1, i*3+2]
                findex.extend([t[2], index, t[1], index, t[0], index, t[0], index+1, t[1], index+1, t[2], index+1])
            try:
                vert_src = source.FloatSource("cubeverts-array"+str(objind), np.array(vert_floats), ('X', 'Y', 'Z'))
                normal_src = source.FloatSource("cubenormals-array"+str(objind),np.array(normal_floats), ('X', 'Y', 'Z'))
                geom = geometry.Geometry(mesh, "geometry"+str(objind), uuid, [vert_src, normal_src])
                
                input_list = source.InputList()
                input_list.addInput(0, 'VERTEX', "#cubeverts-array"+str(objind))
                input_list.addInput(1, 'NORMAL', "#cubenormals-array"+str(objind))
                
                triset = geom.createTriangleSet(np.array(findex), input_list, "materialref"+str(objind))
                geom.primitives.append(triset)
                mesh.geometries.append(geom)
                
                #Create Material
                mat = self.createMaterial(mesh, f, objind)
                matnode = scene.MaterialNode("materialref"+str(objind), mat, inputs=[])
                geomnode = scene.GeometryNode(geom, [matnode])
                node = scene.Node("node"+str(objind), children=[geomnode])
                scenenodes.append(node)
                objind += 1
            except:
                print "Unexpected error:", sys.exc_info()[0]
                print "Error in Traingulation"

        
        myscene = scene.Scene("myscene"+str(objind), scenenodes)
        mesh.scenes.append(myscene)
        mesh.scene = myscene

        mesh.write(str(self.Filename) + ".dae")

    def run(self):
        city = self.getData("City")   
        data_type = -1
        if self.Type == "COMPONENT":
            data_type = COMPONENT
        if self.Type == "FACE":
            data_type = FACE   
        if data_type == -1:
            print "Unknown Data Type"
            return
        uuids = city.getUUIDs(View(self.ViewName, data_type, READ))
        
        if len(uuids) == 0:
            print "no objects found"
            return
        objects = []   
        for uuid in uuids:

            if data_type == COMPONENT:
                building = city.getComponent(uuid)
                LinkAttributes = building.getAttribute("Geometry").getLinks()
                for attribute in LinkAttributes:
                    objects.append(attribute.uuid)
            if data_type == FACE:
                print "add"
                objects.append(uuid)
        
        self.createDAE_KML(city, "", objects)

