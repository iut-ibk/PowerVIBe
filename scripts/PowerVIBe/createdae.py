#!/usr/bin/python
# -*- coding: utf8 -*-
from pydynamind import *
from pydmtoolbox import *
from collada import *
import matplotlib.delaunay as triang
import numpy as np
import math
from pykml.factory import KML_ElementMaker as KML
from pykml.factory import GX_ElementMaker as GX
from lxml import etree
from datetime import date
from datetime import timedelta

import osgeo.ogr as ogr
import osgeo.osr as osr

import sys
#
#    KML.TimeStamp(
#       KML.when((date.today()- timedelta(days=d)).strftime('%Y-%m-%dT%H:%MZ')),
#                  ),
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
        #self.models = View("Model_Building", FACE, READ)
        datastream = []
        datastream.append(self.buildings)
        self.addData("City", datastream)
            
    def createPlacemark(self, uuid, filetoDAE, center):
        node_transformed = self.transformCoorindate(center)
        pm1 = KML.Placemark(
            KML.name(uuid),
            KML.description(uuid),
            GX.balloonVisibility(1),
            KML.ExtendedData(
                            KML.Data(
                             KML.name("huh"),
                             KML.value(1),
                             ),
            ),
            KML.Model(
                KML.styleUrl("#transYellowPoly"),
                KML.altitudeMode('relativeToGround'),
                KML.Location(
                    KML.longitude(node_transformed.getX()),
                    KML.latitude(node_transformed.getY()),
                ),
                KML.Link(
                    KML.href(filetoDAE),
                ),
            ),

        )
        return pm1
    def hasCoordinate(self, x1, X):
        for x in X:
            if abs(x1 - x) > 0.0001:
                return True
        return False
        
    def createPlacemarkAsLineString(self, city, objects, fld):
        for obj in objects:
                f = city.getFace(obj)
                nodes = f.getNodes()
                p_nodes = []
                for n_uuid in nodes:
                    n = city.getNode(n_uuid)
                    p_nodes.append(n)
                p_nodes.reverse()
                nodes_transformed = []
                for n in p_nodes:
                        nodes_transformed.append(self.transformCoorindate(n))
                coordinates =  ''
                for n in nodes_transformed:
                    coordinates+="{0},{1},{2}".format(n.getX(), n.getY(), n.getZ())+"\n"
                print coordinates
                                                       
                pm = KML.Placemark(
                            KML.name(obj),
                            KML.styleUrl("#transYellowPoly"),
                            
                            KML.Polygon(
                            KML.outerBoundaryIs(
                                    KML.LinearRing(
                                           KML.altitudeMode("clampToGround"),                                           
                                           KML.coordinates(coordinates),
                                           ),
                                         ),
                            ),
                
                )
                fld.append(pm)


    def transformCoorindate(self, n):
        x = n.getX()
        y = n.getY()
        wkt = 'POINT(%s %s)' % (x, y)


        # CREATE PROJECTION OBJECTS
        wgs84 = osr.SpatialReference()
        wgs84.ImportFromEPSG(4326)

        # CREATE OGR POINT OBJECT, ASSIGN PROJECTION, REPROJECT
        point = ogr.CreateGeometryFromWkt(wkt)
        point.AssignSpatialReference(self.origin)
    
        point.TransformTo(wgs84)
    
        return Node(point.GetX(), point.GetY(), n.getZ())

    def tessilate(self, X,Y,Z):
        x1 = X[0]
        y1 = Y[0]
        z1 = Z[0]
        
        hasX = self.hasCoordinate(x1,X)
        hasY = self.hasCoordinate(y1,Y)
        hasZ = self.hasCoordinate(z1,Z)
        n = []
        if hasX:
            n.append(X)
        if hasY:
            n.append(Y)
        if hasZ:
            n.append(Z)
        if len(n) < 2:
               print "something is woring"
               tri = []
               return tri
        cens,edg,tri,neig = triang.delaunay(n[0], n[1])
        return tri
            
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
        mat = material.Material("material"+str(id), "mymaterial"+str(id), effect)
        mesh.effects.append(effect)
        mesh.materials.append(mat)
        return mat


    def createDAE_KML(self, city,uuid,  objects, center, fld):
        print "create "  + str(uuid)
        mesh = Collada()
        scenenodes = []
        objind = 0
        for obj in objects:
            f = city.getFace(obj)
            
            print center.getX()
            print center.getY()
            nodes = f.getNodes()
            vert_floats = []
            normal_floats = []
            x = []
            y = []
            z = []
            #Triangulate Objects
            p_nodes = []
            for n_uuid in nodes:
                n = city.getNode(n_uuid)
                x.append(n.getX() - center.getX())
                y.append(n.getY() - center.getY())
                z.append(n.getZ())
                vert_floats.append(n.getX()  - center.getX() )
                vert_floats.append(n.getZ())
                vert_floats.append(n.getY() - center.getY() )
                normal_floats.append(0)
                normal_floats.append(1)
                normal_floats.append(0)
                p_nodes.append(n)
            x.pop()
            y.pop()
            z.pop()
            tri= self.tessilate(x,y,z)
            #print "Triangulation " + str(len(tri))
            findex = []
            for i in range(len(tri)):
                index = i*2
                t = tri[i]
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
        fld.append(self.createPlacemark(uuid, "/tmp/exportdae"+ uuid + ".dae", center))
        mesh.write("/tmp/exportdae"+ uuid + ".dae")
        #objectid_total+=1

    def run(self):
        self.origin = osr.SpatialReference ()
        self.origin.ImportFromEPSG(31257)
        stylename = "export_dm"
        doc = KML.kml(
            KML.Document(
                KML.Name("DynaMind Export"),
                KML.Style(
                    KML.LineStyle(
                        KML.width(1.5),
                    ),
                    KML.PolyStyle(
                        KML.color("7d00ffff"),
                    ),
                    id="transYellowPoly",
                )
            )
        )
        fld = KML.Folder()
        
        city = self.getData("City")   
        uuids = city.getUUIDs(View(self.ViewName, COMPONENT, READ))
        objectid_total = 0
        
        
        
        for uuid in uuids:
            #getLinks
            building = city.getComponent(uuid)
            objects = []
            if self.Type == "COMPONENT":
                print "Component"
                center = Node(building.getAttribute("centroid_x").getDouble(), building.getAttribute("centroid_y").getDouble(),0.0)
                LinkAttributes = building.getAttribute("Model").getLinks()
                for attribute in LinkAttributes:                    
                    objects.append(attribute.uuid)
                self.createDAE_KML(city, uuid, objects, center,fld)
            if self.Type == "FACE":
                    objects.append(uuid)        
                    self.createPlacemarkAsLineString(city, objects, fld)
                
        doc.Document.append(fld)
        text_file = open(self.Filename+str(".kml"), "w")
        text_file.write(etree.tostring(doc, pretty_print=True))
        text_file.close()