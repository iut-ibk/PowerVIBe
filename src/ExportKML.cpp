//
//  exportkml.cpp
//  DynaMind-ToolBox
//
//  Created by Christian Urich on 10/19/12.
//
//

#include "exportkml.h"
#include <iostream>
#include <string>
#include "kml/dom.h"
#include "kml/engine.h"

using kmldom::CoordinatesPtr;
using kmldom::KmlPtr;
using kmldom::KmlPtr;
using kmldom::KmlFactory;
using kmldom::PlacemarkPtr;
using kmldom::PointPtr;
using kmldom::LinearRingPtr;
using kmldom::OuterBoundaryIsPtr;
using kmldom::PolygonPtr;
using kmldom::FolderPtr;

#include <ogr_spatialref.h>
#include <ogrsf_frmts.h>

DM_DECLARE_NODE_NAME(ExportKML, Export)


ExportKML::ExportKML()
{
    
}

void ExportKML::concertCoordinates(double &x, double &y)
{
    OGRSpatialReference oSourceSRS, oTargetSRS;
    OGRCoordinateTransformation *poCT;
    
    oTargetSRS.importFromEPSG( 4326  );
    oSourceSRS.importFromEPSG( 31257 );
    
    poCT = OGRCreateCoordinateTransformation( &oSourceSRS,
                                             &oTargetSRS );
    
    
    if( poCT == NULL || !poCT->Transform( 1, &x, &y ) )
        DM::Logger(DM::Warning) <<  "Transformation failed";
}
void ExportKML::run()
{
    
    double xref_1 = 230884;
    double yref_1 = 235291;
    
    double xref_2 = 230894;
    double yref_2 = 235295;
    
    concertCoordinates(xref_1, yref_1);
    concertCoordinates(xref_2, yref_2);
    // Get the factory singleton to create KML elements.
    KmlFactory* factory = KmlFactory::GetFactory();
    
    CoordinatesPtr coordinates = factory->CreateCoordinates();
    coordinates->add_latlngalt(yref_1,xref_1, 1);
    coordinates->add_latlngalt(yref_1, xref_2, 1);
    coordinates->add_latlngalt(yref_2, xref_2, 1);
    coordinates->add_latlngalt(yref_2, xref_1, 1);
    
    
    LinearRingPtr linearring = factory->CreateLinearRing();
    linearring->set_coordinates(coordinates);
    
    OuterBoundaryIsPtr outerboundaryis = factory->CreateOuterBoundaryIs();
    outerboundaryis->set_linearring(linearring);
    
    PolygonPtr polygon = factory->CreatePolygon();
    polygon->set_tessellate(true);
    polygon->set_altitudemode(kmldom::ALTITUDEMODE_RELATIVETOGROUND);
    polygon->set_outerboundaryis(outerboundaryis);
    
    FolderPtr folder = factory->CreateFolder();
    
    
    PlacemarkPtr placemark = factory->CreatePlacemark();
    placemark->set_name("Simple Polygon");
    placemark->set_geometry(polygon);
    
    
    folder->add_feature(placemark);
    
    /*coordinates = factory->CreateCoordinates();
    coordinates->add_latlngalt(yref_1,xref_1, 5);
    coordinates->add_latlngalt(yref_1, xref_2, 5);
    coordinates->add_latlngalt(yref_2, xref_2, 5);
    coordinates->add_latlngalt(yref_2, xref_1, 5);
    
    
    linearring = factory->CreateLinearRing();
    linearring->set_coordinates(coordinates);
    
    outerboundaryis = factory->CreateOuterBoundaryIs();
    outerboundaryis->set_linearring(linearring);
    
    polygon = factory->CreatePolygon();
    polygon->set_tessellate(true);
    polygon->set_altitudemode(kmldom::ALTITUDEMODE_RELATIVETOGROUND);
    polygon->set_outerboundaryis(outerboundaryis);
    
    PlacemarkPtr placemark1 = factory->CreatePlacemark();
    placemark1->set_name("Simple Polygon 1");
    placemark1->set_geometry(polygon);*/
    
    
    
    
    KmlPtr kml = factory->CreateKml();
    kml->set_feature(folder);  // kml takes ownership.
    
    //kml->set_feature(placemark1);  // kml takes ownership.
    // Serialize to XML
    std::string xml = kmldom::SerializePretty(kml);
    // Print to stdout
    std::cout << xml;
    

    kmlengine::KmzFile::WriteKmz("/Users/christianurich/Documents/test.kmz", xml);


    




    
    
}