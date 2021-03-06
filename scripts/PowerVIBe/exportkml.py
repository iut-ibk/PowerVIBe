#!/usr/bin/python
# -*- coding: utf8 -*-
from pydynamind import *
from pydmtoolbox import *
import pydmextensions
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
class ExportKML(Module):        
    def __init__(self):
        Module.__init__(self)
        self.createParameter("ViewName", STRING, "ViewName")
        self.ViewName = ""
        self.createParameter("Filename", STRING, "Filename")
        self.Filename = ""
        self.createParameter("Type", STRING, "Type")
        self.Type = "COMPONENT"
        self.createParameter("ExtrudAttr", STRING, "")
        self.ExtrudAttr = ""
        self.buildings = View("dummy", SUBSYSTEM, MODIFY)
        self.createParameter("EPSG",INT, "EPSG")
        self.EPSG = 31257
        self.counter = 0;
        #self.models = View("Model_Building", FACE, READ)
        datastream = []

        datastream.append(self.buildings)
        self.addData("City", datastream)

    """convert rgb to kml color"""
    def rgb_to_hex(self, r,g,b,a):
        return '#%02x%02x%02x%02x' % (a,b,g,r)
    

    def rotatePolygon(self, nodes, deg):
        nodes_rotated = []
        rad = deg /180. * math.pi
        for n in nodes:
            x = n.getX()
            y = n.getY()
            x1 = math.cos(rad) * x - math.sin(rad) * y
            y2 = math.sin(rad) * x + math.cos(rad) * y
            nodes_rotated.append(Node(x1,y2,n.getZ()))

        return nodes_rotated

    def translatePolygon(self, nodes, center):
        nodes_translate = []
        for n in nodes:
                x = n.getX()
                y = n.getY()
                x1 = x + center.getX()
                y2 = y + center.getY()
                nodes_translate.append(Node(x1,y2,n.getZ()))
        return nodes_translate
    
    def createPlacemarkForSelection(self, cmp, uuid, center, fld):
        
        #geomtryLinks =  cmp.getAttribute("Geometry").getLinks()
        
        #for link in geomtryLinks:
        #    link.
        l = cmp.getAttribute("l_bounding").getDouble()
        b = cmp.getAttribute("b_bounding").getDouble()
        h = cmp.getAttribute("h_bounding").getDouble()
        alpha = cmp.getAttribute("alpha_bounding").getDouble()
 
        nodes = []
        nodes.append(Node(l/2, -b/2, h))
        nodes.append(Node(l/2, b/2, h))
        nodes.append(Node(-l/2, b/2, h))
        nodes.append(Node(-l/2, -b/2, h))
        nodes.append(Node(l/2, -b/2, h))
        
        nodes = self.rotatePolygon(nodes, alpha)
        nodes = self.translatePolygon(nodes, center)
        
        nodes_transformed = []
        exdata = KML.ExtendedData()
        attributes = cmp.getAllAttributes()
        for k in attributes:
            attr = attributes[k]
            #print attr.getType()
            if attr.getType() == Attribute.DOUBLE:
                #print str(k) + " " + str(attr.getDouble())
                d = KML.Data(
                         KML.name(k),
                         KML.value(attr.getDouble()),
                    )
                exdata.append(d)
                    
        
        for n in nodes:
            nodes_transformed.append(self.transformCoorindate(n))
        coordinates =  ''
        for n in nodes_transformed:
            coordinates+="{0},{1},{2}".format(n.getX(), n.getY(), n.getZ())+"\n"
    
        
        y = int(cmp.getAttribute("built_year").getDouble())
        if y < 1:
                y = 1900
        m = 1
        d = 1
        pm = KML.Placemark(
                           KML.name(uuid+"_Data"),
                           exdata,
                           KML.styleUrl("#transRedPoly"),
                           KML.TimeStamp(
                               KML.when((date(y,m,d)).strftime('%Y-%m-%dT%H:%MZ')),
                            ),
                           KML.Polygon(
                                       KML.extrude(1),
                                       KML.altitudeMode("relativeToGround"),
                                       KML.outerBoundaryIs(
                                                            KML.LinearRing(

                                                                            KML.coordinates(coordinates),
                                                                           ),
                                                            ),
                                        ),
                               
                            )
        fld.append(pm)

    
    def createPlacemark(self, uuid, filetoDAE, center):
        node_transformed = self.transformCoorindate(center)
        pm1 = KML.Placemark(
            KML.name(uuid),
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
            e = city.getEdge(obj)
            n_uuids = []
            n_uuids.append(e.getStartpointName())
            n_uuids.append(e.getEndpointName())
            p_nodes = []
            for n_uuid in n_uuids:
                n = city.getNode(n_uuid)
                p_nodes.append(n)
            p_nodes.reverse()
            nodes_transformed = []
            for n in p_nodes:
                nodes_transformed.append(self.transformCoorindate(n))
            coordinates =  ''
            for n in nodes_transformed:
                coordinates+="{0},{1},{2}".format(n.getX(), n.getY(), n.getZ())+"\n"
            y = int(e.getAttribute("built_year").getDouble())
            if y < 1:
                y = 1900
            m = 1
            d = 1      
            if y < 0.01:
                    continue
            pm = KML.Placemark(
                               KML.name(obj),
                               KML.styleUrl("#SWMM"),
                               KML.TimeStamp(
                                   KML.when((date(y,m,d)).strftime('%Y-%m-%dT%H:%MZ')),
                                    ),
                                        KML.LineString(
                                            KML.altitudeMode("relativeToGround"),
                                            KML.coordinates(coordinates),
                                        ),
                               )
            fld.append(pm)

    def createPlacemarkAsLineRing(self, city, objects, fld):
        for obj in objects:
                f = city.getFace(obj)
                nodes = f.getNodes()
                p_nodes = []
                for n_uuid in nodes:
                    n = city.getNode(n_uuid)
                    p_nodes.append(n)
                if pydmextensions.CGALGeometry_CheckOrientation(f.getNodePointers()) == False:
                    p_nodes.reverse()
                p_nodes.append(p_nodes[0])
             
                nodes_transformed = []
                for n in p_nodes:
                        nodes_transformed.append(self.transformCoorindate(n))
                coordinates =  ''
                z = 0
                if self.ExtrudAttr != "":
                    z = f.getAttribute(self.ExtrudAttr).getDouble()
                for n in nodes_transformed:
                    coordinates+="{0},{1},{2}".format(n.getX(), n.getY(), z)+"\n"
                y = int(f.getAttribute("built_year").getDouble())
                #print y
                m = 1
                d = 1
                if y < 1:
                    y = 1900
                if self.ExtrudAttr == "":
                    pm = KML.Placemark(
                                KML.name(obj),
                                KML.styleUrl(str("style_")+str(obj)),  
                                KML.TimeStamp(
                                   KML.when((date(y,m,d)).strftime('%Y-%m-%dT%H:%MZ')),
                                ),                          
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
                if self.ExtrudAttr != "":
                    pm = KML.Placemark(
                           KML.name(obj),
                           KML.styleUrl(str("style_")+str(obj)),
                           KML.TimeStamp(
                                   KML.when((date(y,m,d)).strftime('%Y-%m-%dT%H:%MZ')),
                                ),     
                           KML.Polygon(
                                       KML.extrude(1),
                                       KML.altitudeMode("relativeToGround"),
                                       KML.outerBoundaryIs(
                                                            KML.LinearRing(

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
               print "something is wrong"
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
    
    def create_syles(self,city, object):
        obj = city.getComponent(object)
        
        r = int( obj.getAttribute("color_r").getDouble() * 255.)
        #print r
        g = int( obj.getAttribute("color_g").getDouble() * 255.)
        b = int( obj.getAttribute("color_b").getDouble() * 255.)
        alpha = int( obj.getAttribute("color_alpha").getDouble() * 255.)
        sty = KML.Style(
            KML.LineStyle(
                KML.width(0.5),
            ),
            KML.PolyStyle(
                        KML.color(self.rgb_to_hex( r,g,b,alpha)),
                        ),
                        id="style_"+str(object),
            )
        return sty
            
    def run(self):
        self.counter = self.counter + 1
        self.origin = osr.SpatialReference ()
        self.origin.ImportFromEPSG(self.EPSG)
        stylename = "export_dm"
        
        doc = KML.kml()
        document = KML.Document()
        docname = KML.Name("DynaMind Export")
        document.append(docname)
        doc.append(document)
        document.append(
        KML.Style(
            KML.LineStyle(
                KML.width(3),
                KML.color(self.rgb_to_hex( 0,255,255,255)),
            ),
            KML.PolyStyle(
                        KML.color(self.rgb_to_hex( 0,255,255,255)),
                        ),
                        id="#SWMM",
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
                center = Node(building.getAttribute("centroid_x").getDouble(), building.getAttribute("centroid_y").getDouble(),0.0)
                LinkAttributes = building.getAttribute("Geometry").getLinks()
                for attribute in LinkAttributes:
                    objects.append(attribute.uuid)
                self.createPlacemarkForSelection(city.getComponent(uuid), uuid, center, fld)
            if self.Type == "FACE":
                    objects.append(uuid)        
                    self.createPlacemarkAsLineRing(city, objects, fld)                    
                    document.append(self.create_syles(city, uuid))
            if self.Type == "EDGE":
                    objects.append(uuid)
                    self.createPlacemarkAsLineString(city, objects, fld)
                
        doc.Document.append(fld)
        text_file = open(self.Filename+"_"+str(self.counter)+str(".kml"), "w")
        text_file.write(etree.tostring(doc, pretty_print=True))
        text_file.close()