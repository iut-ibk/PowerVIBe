#include "createsinglefamilyhouses.h"
#include "dmgeometry.h"
#include "cgalgeometry.h"
#include "tbvectordata.h"
#include <QPolygonF>
#include <QTransform>
#include <dmhelper.h>
#include <tbvectordata.h>
#include <cutelittlegeometryhelpers.h>
#include <omp.h>

DM_DECLARE_NODE_NAME(CreateSingleFamilyHouses, BlockCity)

CreateSingleFamilyHouses::CreateSingleFamilyHouses()
{

    stories = 1;
    this->addParameter("Stories", DM::INT, &stories);

    parcels = DM::View("PARCEL", DM::FACE, DM::READ);
    parcels.getAttribute("centroid_x");
    parcels.getAttribute("centroid_y");

    houses = DM::View("BUILDING", DM::COMPONENT, DM::WRITE);
    houses.addAttribute("footprint_area");
    houses.addAttribute("centroid_x");
    houses.addAttribute("centroid_y");

    houses.addAttribute("built_year");
    houses.addAttribute("stories");
    houses.addAttribute("stories_below");
    houses.addAttribute("stories_height");

    houses.addAttribute("floor_area");
    houses.addAttribute("gross_floor_area");

    houses.addAttribute("centroid_x");
    houses.addAttribute("centroid_y");

    houses.addAttribute("l_bounding");
    houses.addAttribute("b_bounding");
    houses.addAttribute("h_bounding");
    houses.addAttribute("alhpa_bounding");

    houses.addAttribute("alpha_roof");

    houses.addAttribute("cellar_used");
    houses.addAttribute("roof_used");

    houses.addAttribute("T_heating");
    houses.addAttribute("T_cooling");

    houses.addAttribute("Geometry");
    houses.addAttribute("V_living");

    footprint = DM::View("Footprint", DM::FACE, DM::WRITE);

    building_model = DM::View("Geometry", DM::FACE, DM::WRITE);
    building_model.addAttribute("type");

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

    std::vector<std::string> parcelUUIDs = city->getUUIDs(parcels);

    int nparcels = parcelUUIDs.size();
    //#pragma omp parallel for
    for (int i = 0; i < nparcels; i++) {
        DM::Face * parcel = city->getFace(parcelUUIDs[i]);
        std::vector<DM::Node * > nodes  = TBVectorData::getNodeListFromFace(city, parcel);
        
        std::vector<DM::Node> bB;
        //Calcualte bounding minial bounding box
        std::vector<double> size;
        double angle = CGALGeometry::CalculateMinBoundingBox(nodes, bB,size);
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

        DM::Component * building = city->addComponent(new Component(), houses);


        //Create Building and Footprints
        DM::Face * foot_print = city->addFace(houseNodes, footprint);

        Node  n = TBVectorData::CaclulateCentroid(city, foot_print);
        building->addAttribute("type", "single_family_house");
        building->addAttribute("built_year", 1980);
        building->addAttribute("stories", stories);
        building->addAttribute("stories_below", 0); //cellar counts as story
        building->addAttribute("stories_height",3 );

        building->addAttribute("floor_area", l*b);
        building->addAttribute("gross_floor_area", l * b * stories * 3);

        building->addAttribute("centroid_x", n.getX());
        building->addAttribute("centroid_y", n.getY());

        building->addAttribute("l_bounding", l);
        building->addAttribute("b_bounding", b);
        building->addAttribute("h_bounding", stories * 3);

        building->addAttribute("alpha_bounding", angle);

        building->addAttribute("alpha_roof", 0);

        building->addAttribute("cellar_used", 1);
        building->addAttribute("roof_used", 0);

        building->addAttribute("T_heating", 20);
        building->addAttribute("T_cooling", 26);


        building->addAttribute("V_living", l*b*stories * 3);

        CuteLittleGeometryHelpers::CreateStandardBuilding(city, houses, building_model, building, houseNodes, stories);

        //Create Links
        building->getAttribute("PARCEL")->setLink(parcels.getName(), parcel->getUUID());
        parcel->getAttribute("BUILDING")->setLink(houses.getName(), building->getUUID());
        
    }
    Logger(Debug) << "Created Houses " << nparcels;
}
