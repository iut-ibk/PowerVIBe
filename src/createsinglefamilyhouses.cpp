#include "createsinglefamilyhouses.h"
#include "dmgeometry.h"
#include "cgalgeometry.h"
#include "tbvectordata.h"
#include <QPolygonF>
#include <QTransform>
#include <dmhelper.h>
#include <tbvectordata.h>

DM_DECLARE_NODE_NAME(CreateSingleFamilyHouses, BlockCity)

CreateSingleFamilyHouses::CreateSingleFamilyHouses()
{
    houses = DM::View("BUILDING", DM::COMPONENT, DM::WRITE);
    houses.addAttribute("footprint_area");
    parcels.addAttribute("centroid_x");
    parcels.addAttribute("centroid_y");

    parcels = DM::View("PARCEL", DM::FACE, DM::READ);
    parcels.getAttribute("centroid_x");
    parcels.getAttribute("centroid_y");
    

    building_model = DM::View("Model", DM::FACE, DM::WRITE);
    building_model.addAttribute("type");
    building_model.addAttribute("color");
    footprint = DM::View("Footprint", DM::FACE, DM::WRITE);


    parcels.addLinks("BUILDING", houses);
    houses.addLinks("PARCEL", parcels);

    std::vector<DM::View> data;
    data.push_back(houses);
    data.push_back(parcels);
    data.push_back(footprint);
    data.push_back(building_model);
    this->addData("City", data);
}

void CreateSingleFamilyHouses::run()
{
    DM::System * city = this->getData("City");
    DM::SpatialNodeHashMap spatialNodeMap(city, 100);

    std::vector<double> roofColor;
    roofColor.push_back(0.66);
    roofColor.push_back(0.66);
    roofColor.push_back(0.66);
    std::vector<double> wallColor;
    wallColor.push_back(0.96);
    wallColor.push_back(0.96);
    wallColor.push_back(0.86);

    std::vector<std::string> parcelUUIDs = city->getUUIDs(parcels);
    foreach (std::string parcelUUID, parcelUUIDs) {
        DM::Face * parcel = city->getFace(parcelUUID);
        std::vector<DM::Node * > nodes  = TBVectorData::getNodeListFromFace(city, parcel);
        
        std::vector<DM::Node> bB;        
        //Calcualte bounding minial bounding box
        
        double angle = CGALGeometry::CalculateMinBoundingBox(nodes, bB);
        Node centroid = DM::Node(parcel->getAttribute("centroid_x")->getDouble(),  parcel->getAttribute("centroid_y")->getDouble(), 0);
        
        double l = 16;
        double b = 10;
         
         QPointF f1 (centroid.getX() - l/2, centroid.getY() - b/2);
         QPointF f2 (centroid.getX() + l/2, centroid.getY() - b/2);
         QPointF f3 (centroid.getX() + l/2, centroid.getY() + b/2);
         QPointF f4 (centroid.getX() - l/2, centroid.getY() + b/2);
        
        
        QPolygonF original = QPolygonF() << f1 << f2 << f3 << f4;
        QTransform transform = QTransform().rotate(angle);
        QPolygonF rotated = transform.map(original);
        
        std::vector<DM::Node * > houseNodes;
        
        foreach (QPointF p, rotated) {            
            houseNodes.push_back(spatialNodeMap.addNode(p.x(), p.y(), 0, 0.01, DM::View()));
            
        }
        if (houseNodes.size() < 2) {
            Logger(Error) << "Can't create House";
            continue;
        }
        houseNodes.push_back(houseNodes[0]);

        DM::Component * building = city->addComponent(new DM::Component(), houses);
        building->addAttribute("footprint_area", l*b);

        //Create Building and Footprints
        DM::Face * foot_print = city->addFace(houseNodes, footprint);

        Node  n = TBVectorData::CaclulateCentroid(city, foot_print);
        building->addAttribute("centroid_x", n.getX());
        building->addAttribute("centroid_y", n.getY());

        //Create Walls
        std::vector<DM::Face*> extruded_faces = CGALGeometry::ExtrudeFace(city, building_model, houseNodes, 4);
        int lastID = extruded_faces.size();
        for (int i = 0; i < lastID; i++) {
            DM::Face * f = extruded_faces[i];
            if (i != lastID-1) f->getAttribute("color")->setDoubleVector(wallColor);
            else  f->getAttribute("color")->setDoubleVector(roofColor);
            building->getAttribute("Model")->setLink("Model", f->getUUID());
            f->getAttribute("Parent")->setLink(houses.getName(), building->getUUID());

        }

        //Link Building with Footprint
        DMHelper::LinkComponents(houses, building, footprint, foot_print);

        //Create Links
        building->getAttribute("PARCEL")->setLink(parcels.getName(), parcel->getUUID());
        parcel->getAttribute("BUILDING")->setLink(houses.getName(), building->getUUID());
        
    }
}
