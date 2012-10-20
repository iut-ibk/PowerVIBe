#!/usr/bin/python
# -*- coding: utf8 -*-
from pydynamind import *
from collada import *
import matplotlib.delaunay as triang
import numpy as np
import math
from pykml.factory import KML_ElementMaker as KML
from lxml import etree
from datetime import date
from datetime import timedelta

class CreateDAE(Module):        
    def __init__(self):
            Module.__init__(self)
            
            self.buildings = View("CITYBLOCK", COMPONENT, READ)
            
            datastream = []
            datastream.append(self.buildings)
            self.addData("City", datastream)
            
    def createPlacemark(self, uuid, filetoDAE, d):
        print d
        print (date.today()- timedelta(days=d))
        pm1 = KML.Placemark(
            KML.name(uuid),
            KML.Model(
                KML.altitudeMode('relativeToGround'),
                KML.Location(
                    KML.longitude(146.806),
                    KML.latitude(12.219),
                ),
                KML.Link(
                    KML.href(filetoDAE),
                ),
            ),
            KML.TimeStamp(                    
                KML.when((date.today()- timedelta(days=d)).strftime('%Y-%m-%dT%H:%MZ')),
            ),
        )
        return pm1
                  
    def run(self):
        stylename = "export_dm"
        doc = KML.kml(
            KML.Document(
                KML.Name("DynaMind Export"),
                KML.Style(
                    KML.IconStyle(
                        KML.scale(1.2),
                        KML.Icon(
                            KML.href("http://maps.google.com/mapfiles/kml/shapes/shaded_dot.png")
                        ),
                    ),
                    id=stylename,
                )
            )
        )
        fld = KML.Folder()
        
        city = self.getData("City")   
        uuids = city.getUUIDs(self.buildings)
        objectid_total = 0
        for uuid in uuids:
            
            mesh = Collada()
            effect = material.Effect("effect0", [], "phong", diffuse=(0,1,0), specular=(0,1,0))
            mat = material.Material("material0", "mymaterial", effect)
            mesh.effects.append(effect)
            mesh.materials.append(mat)            
            scenenodes = []
            objind = 0
            f = city.getFace(uuid)
            nodes = f.getNodes()
            vert_floats = []
            normal_floats = []
            x =[]
            y = []
            #Triangulate Objects
            
            for n_uuid in nodes:
                n = city.getNode(n_uuid)
                x.append(n.getX())
                y.append(n.getY())
                vert_floats.append(n.getX())
                vert_floats.append(n.getZ())
                vert_floats.append(n.getY())                
                normal_floats.append(0)
                normal_floats.append(1)
                normal_floats.append(0)
            x.pop()
            y.pop()
            cens,edg,tri,neig = triang.delaunay(x, y)
            #print "Triangulation " + str(len(tri))
            findex = []    
            for i in range(len(tri)):
                t = tri[i]
                findex.extend([t[2], i, t[1], i, t[0], i])
            try:
                vert_src = source.FloatSource("cubeverts-array"+str(objind), np.array(vert_floats), ('X', 'Y', 'Z'))
                normal_src = source.FloatSource("cubenormals-array"+str(objind),np.array(normal_floats), ('X', 'Y', 'Z'))
                geom = geometry.Geometry(mesh, "geometry"+str(objind), uuid, [vert_src, normal_src])

                input_list = source.InputList()
                input_list.addInput(0, 'VERTEX', "#cubeverts-array"+str(objind))
                input_list.addInput(1, 'NORMAL', "#cubenormals-array"+str(objind))
                
                triset = geom.createTriangleSet(np.array(findex), input_list, "materialref")
                geom.primitives.append(triset)
                mesh.geometries.append(geom)
                
                matnode = scene.MaterialNode("materialref", mat, inputs=[])
                geomnode = scene.GeometryNode(geom, [matnode])
                node = scene.Node("node"+str(objind), children=[geomnode])
                scenenodes.append(node)
                objind += 1
            except:
                print "Error in Traingulation"
                
            #print "Adding Scene"
            myscene = scene.Scene("myscene"+str(objind), scenenodes)
            mesh.scenes.append(myscene)
            mesh.scene = myscene
            #print "Start Writing File"
            fld.append(self.createPlacemark(uuid, "/tmp/exportdae"+ uuid + ".dae", objectid_total))
            mesh.write("/tmp/exportdae"+ uuid + ".dae")
            objectid_total+=1
        doc.Document.append(fld)
        text_file = open("test_dynamind.kml", "w")
        text_file.write(etree.tostring(doc, pretty_print=True))
        text_file.close()