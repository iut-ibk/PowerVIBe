#include "singlehouse.h"
#include "dmgeometry.h"
#include "cgalgeometry.h"
#include "tbvectordata.h"
#include <QPolygonF>
#include <QTransform>
#include <dmhelper.h>
#include <tbvectordata.h>
#include <cutelittlegeometryhelpers.h>

DM_DECLARE_NODE_NAME(SingleHouse, TestModules)
SingleHouse::SingleHouse()
{
    houses = DM::View("BUILDING", DM::FACE, DM::WRITE);
    houses.addAttribute("built_year");
    houses.addAttribute("stories");
    houses.addAttribute("stories_below"); //cellar counts as story
    houses.addAttribute("stories_height");

    houses.addAttribute("floor_area");
    houses.addAttribute("gross_floor_area");

    houses.addAttribute("centroid_x");
    houses.addAttribute("centroid_y");

    houses.addAttribute("l_bounding");
    houses.addAttribute("b_bounding");
    houses.addAttribute("h_bounding");

    houses.addAttribute("alpha_bounding");

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

    std::vector<DM::View> data;
    data.push_back(houses);
    data.push_back(footprint);
    data.push_back(building_model);
    heatingT = 21;
    buildyear = 1985;
    stories = 1;
    this->addParameter("HeatingT", DM::DOUBLE, &heatingT);
    this->addParameter("buildyear", DM::INT, &buildyear);
    this->addParameter("stories", DM::INT, &stories);
    this->addData("city", data);


}

void SingleHouse::run()
{

    DM::System * city = this->getData("city");

    double l = 16;
    double b = 10;
    QPointF f1 ( -l/2,  - b/2);
    QPointF f2 (l/2, -b/2);
    QPointF f3 (l/2,  b/2);
    QPointF f4 (-l/2, b/2);

    double angle = 0;
    QPolygonF original = QPolygonF() << f1 << f2 << f3 << f4;
    QTransform transform = QTransform().rotate(angle);
    QPolygonF rotated = transform.map(original);

    std::vector<DM::Node * > houseNodes;



    foreach (QPointF p, rotated) {
        houseNodes.push_back(city->addNode(p.x(), p.y(), 0));

    }
    if (houseNodes.size() < 2) {
        Logger(Error) << "Can't create House";

    }
    houseNodes.push_back(houseNodes[0]);

    DM::Component * building = city->addFace(houseNodes, houses);

    //Create Building and Footprints
    DM::Face * foot_print = city->addFace(houseNodes, footprint);
    //Link Building with Footprint
    DMHelper::LinkComponents(houses, building, footprint, foot_print);

    Node  n = TBVectorData::CaclulateCentroid(city, foot_print);
    building->addAttribute("type", "single_family_house");
    building->addAttribute("built_year", buildyear);
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


    building->addAttribute("T_heating", heatingT);
    building->addAttribute("T_cooling", 26);

    building->addAttribute("V_living", l*b*stories * 3);

    CuteLittleGeometryHelpers::CreateStandardBuilding(city, houses, building_model, building, houseNodes, stories);




}
