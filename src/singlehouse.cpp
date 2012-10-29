#include "singlehouse.h"
#include "dmgeometry.h"
#include "cgalgeometry.h"
#include "tbvectordata.h"
#include <QPolygonF>
#include <QTransform>
#include <dmhelper.h>
#include <tbvectordata.h>

DM_DECLARE_NODE_NAME(SingleHouse, TestModules)
SingleHouse::SingleHouse()
{
    houses = DM::View("Building", DM::COMPONENT, DM::WRITE);
    houses.addAttribute("Model");

    footprint = DM::View("Footprint", DM::FACE, DM::WRITE);

    building_model = DM::View("Model", DM::FACE, DM::WRITE);
    building_model.addAttribute("type");

    std::vector<DM::View> data;
    data.push_back(houses);
    data.push_back(footprint);
    data.push_back(building_model);


    this->addData("city", data);


}

void SingleHouse::run()
{
    DM::System * city = this->getData("city");
    DM::Component * building = city->addComponent(new DM::Component(), houses);
    double l = 16;
    double b = 10;
    double stories = 2;
    QPointF f1 ( - l/2,  - b/2);
    QPointF f2 (l/2, - b/2);
    QPointF f3 (l/2,  b/2);
    QPointF f4 ( - l/2, b/2);

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



    //Set footprint as floor

    DM::Face * base_plate = city->addFace(houseNodes, building_model);
    building->getAttribute("Model")->setLink("Model", base_plate->getUUID());
    base_plate->getAttribute("Parent")->setLink(houses.getName(), building->getUUID());
    base_plate->addAttribute("type", "ceiling_cellar");

    //Create Walls
    std::vector<DM::Face*> extruded_faces = TBVectorData::ExtrudeFace(city, building_model, houseNodes, stories * 3);
    int lastID = extruded_faces.size();


    for (int i = 0; i < lastID; i++) {
        DM::Face * f = extruded_faces[i];
        if (i != lastID-1) {
            f->addAttribute("type", "wall_outside");
        }
        else {
            f->addAttribute("type", "ceiling_roof");
        }
        building->getAttribute("Model")->setLink("Model", f->getUUID());
        f->getAttribute("Parent")->setLink(houses.getName(), building->getUUID());

    }



    //Link Building with Footprint
    DMHelper::LinkComponents(houses, building, footprint, foot_print);



}
