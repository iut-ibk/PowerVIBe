#include "extrudebuildings.h"
#include "dmgeometry.h"
#include "cgalgeometry.h"
#include "tbvectordata.h"
#include <QPolygonF>
#include <QTransform>
#include <dmhelper.h>
#include <tbvectordata.h>
#include <littlegeometryhelpers.h>
#ifdef _OPENMP
#include <omp.h>
#endif

using namespace DM;

DM_DECLARE_NODE_NAME(ExtrudeBuildings, PowerVIBe)

ExtrudeBuildings::ExtrudeBuildings()
{
    heatingT = 20;
    coolingT = 20;
    minArea = 75;

    withWindows = false;

    this->addParameter("T_heating", DM::DOUBLE, &heatingT);
    this->addParameter("T_cooling", DM::DOUBLE, &coolingT);
    this->addParameter("withWindows", DM::BOOL, &withWindows);

    this->addParameter("minArea", DM::DOUBLE, &minArea);

    cityView = DM::View("CITY", DM::FACE, DM::READ);
    cityView.getAttribute("year");
    parcels = DM::View("PARCEL", DM::FACE, DM::READ);

    //parcels.addAttribute("is_built");

    /*parcels.getAttribute("centroid_x");
    parcels.getAttribute("centroid_y");*/

    houses = DM::View("BUILDING", DM::COMPONENT, DM::WRITE);

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

    footprints = DM::View("Footprint", DM::FACE, DM::READ);
    footprints.getAttribute("stories");
    footprints.getAttribute("PARCEL");
    footprints.addLinks("BUILDING", houses);

    building_model = DM::View("Geometry", DM::FACE, DM::WRITE);
    building_model.addAttribute("type");

    parcels.addLinks("BUILDING", houses);
    houses.addLinks("PARCEL", parcels);

    std::vector<DM::View> data;
    data.push_back(houses);
    data.push_back(parcels);
    data.push_back(footprints);
    data.push_back(building_model);
    data.push_back(cityView);
    this->addData("City", data);
}

void ExtrudeBuildings::run()
{
    DM::System * city = this->getData("City");
    DM::SpatialNodeHashMap spatialNodeMap(city, 100);

    std::vector<std::string> city_uuid = city->getUUIDs(cityView);
    int buildyear = 1980;


    std::vector<std::string> footprintUUIDs = city->getUUIDs(this->footprints);

    int nfootprints = footprintUUIDs.size();
    int numberOfHouseBuild = 0;

    for (int i = 0; i < nfootprints; i++) {
        DM::Face * footprint = city->getFace(footprintUUIDs[i]);
        int stories =  (int) footprint->getAttribute("stories")->getDouble();

        std::vector<DM::Node * > nodes  = TBVectorData::getNodeListFromFace(city, footprint);

        double area = fabs(TBVectorData::CalculateArea(city, footprint));
        if (area < this->minArea)
            continue;
        if (!CGALGeometry::CheckOrientation(nodes))
        {
            std::reverse(nodes.begin(), nodes.end());
            Logger(Debug) << "reverse";

        }




        std::vector<DM::Node> bB;
        //Calcualte bounding minial bounding box
        std::vector<double> size;
        double angle = CGALGeometry::CalculateMinBoundingBox(nodes, bB,size);
        //Node centroid = DM::Node(parcel->getAttribute("centroid_x")->getDouble(),  parcel->getAttribute("centroid_y")->getDouble(), 0);

        if (nodes.size() < 2) {
            Logger(Error) << "Can't create House";
            continue;
        }
        //std::vector<DM::Node * > houseNodes;
        //houseNodes.push_back(houseNodes[0]);

        DM::Component * building = city->addComponent(new Component(), houses);



        Node  n = DM::CGALGeometry::CalculateCentroid(city, footprint);
        building->addAttribute("type", "single_family_house");
        building->addAttribute("built_year", buildyear);
        building->addAttribute("stories", stories);
        building->addAttribute("stories_below", 0); //cellar counts as story
        building->addAttribute("stories_height",3 );

        building->addAttribute("floor_area", area);
        building->addAttribute("gross_floor_area", area*stories);

        building->addAttribute("centroid_x", n.getX());
        building->addAttribute("centroid_y", n.getY());

        building->addAttribute("l_bounding", size[0]);
        building->addAttribute("b_bounding", size[1]);
        building->addAttribute("h_bounding", stories * 3);

        building->addAttribute("alpha_bounding", angle);


        building->addAttribute("cellar_used", 1);
        building->addAttribute("roof_used", 0);

        building->addAttribute("T_heating", heatingT);
        building->addAttribute("T_cooling", coolingT);
       // building->addAttribute("b_id", footprint->getAttribute("b_id")->getDouble());


        building->addAttribute("V_living", TBVectorData::CalculateArea(nodes)*stories * 3);

        LittleGeometryHelpers::CreateStandardBuilding(city, houses, building_model, building, nodes, stories,withWindows);

        std::vector<LinkAttribute> plinks = footprint->getAttribute("PARCEL")->getLinks();
        foreach (LinkAttribute link, plinks) {
            building->getAttribute("PARCEL")->setLink("PARCEL", link.uuid);
            DM::Component * parcel = city->getComponent( link.uuid);
            parcel->getAttribute("BUILDING")->setLink("BUILDING", building->getUUID());
        }
        building->getAttribute("Footprint")->setLink(this->footprints.getName(), footprint->getUUID());

        footprint->getAttribute("BUILDING")->setLink(this->houses.getName(), building->getUUID());


        //Create Links
        //building->getAttribute("PARCEL")->setLink(parcels.getName(), parcel->getUUID());
        //parcel->getAttribute("BUILDING")->setLink(houses.getName(), building->getUUID());
        //parcel->addAttribute("is_built",1);
        numberOfHouseBuild++;

    }
    Logger(Debug) << "Created Houses " << numberOfHouseBuild;
}
