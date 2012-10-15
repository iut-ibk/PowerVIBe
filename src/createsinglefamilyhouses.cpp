#include "createsinglefamilyhouses.h"
#include "dmgeometry.h"
#include "geometry.h"
#include "tbvectordata.h"
#include <QPolygonF>
#include <QTransform>

DM_DECLARE_NODE_NAME(CreateSingleFamilyHouses, BlockCity)

CreateSingleFamilyHouses::CreateSingleFamilyHouses()
{
    houses = DM::View("BUILDING", DM::FACE, DM::WRITE);

    parcels = DM::View("PARCEL", DM::FACE, DM::READ);
    parcels.getAttribute("centroid_x");
    parcels.getAttribute("centroid_y");
    

    parcels.addLinks("BUILDING", houses);
    houses.addLinks("PARCEL", parcels);

    std::vector<DM::View> data;
    data.push_back(houses);
    data.push_back(parcels);
    this->addData("City", data);
}

void CreateSingleFamilyHouses::run()
{
    DM::System * city = this->getData("City");
    DM::SpatialNodeHashMap spatialNodeMap(city, 100);

    std::vector<std::string> parcelUUIDs = city->getUUIDs(parcels);
    foreach (std::string parcelUUID, parcelUUIDs) {
        DM::Face * parcel = city->getFace(parcelUUID);
        std::vector<DM::Node * > nodes  = TBVectorData::getNodeListFromFace(city, parcel);
        
        std::vector<DM::Node> bB;        
        //Calcualte bounding minial bounding box
        
        double angle = Geometry::calculateMinBoundingBox(nodes, bB);
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
        DM::Face * building = city->addFace(houseNodes, houses);
        
        //Create Links
        building->getAttribute("PARCEL")->setLink(parcels.getName(), parcel->getUUID());
        parcel->getAttribute("BUILDING")->setLink(houses.getName(), building->getUUID());
        
    }
}
