#!/usr/bin/python
# -*- coding: utf8 -*-
from pydynamind import *
from collada import *
import numpy

class CreateDAE(Module):        
    def __init__(self):
            Module.__init__(self)
            
            self.buildings = View("CITYBLOCK", COMPONENT, READ)
            
            datastream = []
            datastream.append(self.buildings)
            self.addData("City", datastream)
            
    def run(self):
        city = self.getData("City")   
        mesh = Collada()
        effect = material.Effect("effect0", [], "phong", diffuse=(1,0,0), specular=(0,1,0))
        mat = material.Material("material0", "mymaterial", effect)
        mesh.effects.append(effect)
        mesh.materials.append(mat)
        uuids = city.getUUIDs(self.buildings)
        scenenodes = []
        objind = 0
        for uuid in uuids:
            f = city.getFace(uuid)
            nodes = f.getNodes()
            vert_floats = []
            normal_floats = []
            for n_uuid in nodes:
                n = city.getNode(n_uuid)
                print str(n.getX())
                vert_floats.append(n.getX())
                vert_floats.append(n.getZ())
                vert_floats.append(n.getY())                
                normal_floats.append(0)
                normal_floats.append(-1)
                normal_floats.append(0)
            vert_src = source.FloatSource("cubeverts-array"+str(objind), numpy.array(vert_floats), ('X', 'Y', 'Z'))
            normal_src = source.FloatSource("cubenormals-array"+str(objind), numpy.array(normal_floats), ('X', 'Y', 'Z'))
            geom = geometry.Geometry(mesh, "geometry"+str(objind), uuid, [vert_src, normal_src])

            input_list = source.InputList()
            input_list.addInput(0, 'VERTEX', "#cubeverts-array"+str(objind))
            input_list.addInput(1, 'NORMAL', "#cubenormals-array"+str(objind))
            indices = numpy.array([0,0,1,1,2,2,3,3])
            triset = geom.createPolylist(indices, [4], input_list, "materialref")
            geom.primitives.append(triset)
            mesh.geometries.append(geom)
            
            matnode = scene.MaterialNode("materialref", mat, inputs=[])
            geomnode = scene.GeometryNode(geom, [matnode])
            node = scene.Node("node"+str(objind), children=[geomnode])
            scenenodes.append(node)
            objind += 1
            myscene = scene.Scene("myscene"+str(objind), scenenodes)
            mesh.scenes.append(myscene)
        mesh.scene = myscene
        mesh.write('test1.dae')
