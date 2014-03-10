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
	cityView.addAttribute("year", DM::Attribute::DOUBLE, DM::READ);
	parcels = DM::View("PARCEL", DM::FACE, DM::READ);

	//parcels.addAttribute("is_built");

	/*parcels.getAttribute("centroid_x");
	parcels.getAttribute("centroid_y");*/

	houses = DM::View("BUILDING", DM::COMPONENT, DM::WRITE);

	houses.addAttribute("centroid_x", DM::Attribute::DOUBLE, DM::WRITE);
	houses.addAttribute("centroid_y", DM::Attribute::DOUBLE, DM::WRITE);

	houses.addAttribute("built_year", DM::Attribute::DOUBLE, DM::WRITE);
	houses.addAttribute("stories", DM::Attribute::DOUBLE, DM::WRITE);
	houses.addAttribute("stories_below", DM::Attribute::DOUBLE, DM::WRITE);
	houses.addAttribute("stories_height", DM::Attribute::DOUBLE, DM::WRITE);

	houses.addAttribute("floor_area", DM::Attribute::DOUBLE, DM::WRITE);
	houses.addAttribute("gross_floor_area", DM::Attribute::DOUBLE, DM::WRITE);

	houses.addAttribute("centroid_x", DM::Attribute::DOUBLE, DM::WRITE);
	houses.addAttribute("centroid_y", DM::Attribute::DOUBLE, DM::WRITE);

	houses.addAttribute("l_bounding", DM::Attribute::DOUBLE, DM::WRITE);
	houses.addAttribute("b_bounding", DM::Attribute::DOUBLE, DM::WRITE);
	houses.addAttribute("h_bounding", DM::Attribute::DOUBLE, DM::WRITE);
	houses.addAttribute("alhpa_bounding", DM::Attribute::DOUBLE, DM::WRITE);

	houses.addAttribute("alpha_roof", DM::Attribute::DOUBLE, DM::WRITE);

	houses.addAttribute("cellar_used", DM::Attribute::DOUBLE, DM::WRITE);
	houses.addAttribute("roof_used", DM::Attribute::DOUBLE, DM::WRITE);

	houses.addAttribute("T_heating", DM::Attribute::DOUBLE, DM::WRITE);
	houses.addAttribute("T_cooling", DM::Attribute::DOUBLE, DM::WRITE);

	houses.addAttribute("Geometry", "Geometry", DM::WRITE);
	houses.addAttribute("V_living", DM::Attribute::DOUBLE, DM::WRITE);

	footprints = DM::View("Footprint", DM::FACE, DM::READ);
	footprints.addAttribute("stories", DM::Attribute::DOUBLE, DM::READ);
	footprints.addAttribute("PARCEL", "PARCEL", DM::READ);
	footprints.addAttribute("BUILDING", houses.getName(), DM::WRITE);

	building_model = DM::View("Geometry", DM::FACE, DM::WRITE);
	building_model.addAttribute("type", DM::Attribute::STRING, DM::WRITE);

	parcels.addAttribute("BUILDING", houses.getName(), DM::WRITE);
	houses.addAttribute("PARCEL", parcels.getName(), DM::WRITE);

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

	//std::vector<std::string> city_uuid = city->getUUIDs(cityView);
	int buildyear = 1980;
	int numberOfHouseBuild = 0;

	foreach(DM::Component* cmp, city->getAllComponentsInView(this->footprints))
	{
		DM::Face * footprint = (DM::Face*)cmp;
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

		//std::vector<LinkAttribute> plinks = footprint->getAttribute("PARCEL")->getLinks();
		//foreach (LinkAttribute link, plinks) {
		foreach(Component* parcel, footprint->getAttribute("PARCEL")->getLinkedComponents())
		{
			building->getAttribute("PARCEL")->addLink(parcel, "PARCEL");
			parcel->getAttribute("BUILDING")->addLink(building, "BUILDING");
		}

		building->getAttribute("Footprint")->addLink(footprint, this->footprints.getName());
		footprint->getAttribute("BUILDING")->addLink(building, this->houses.getName());

		//Create Links
		//building->getAttribute("PARCEL")->setLink(parcels.getName(), parcel->getUUID());
		//parcel->getAttribute("BUILDING")->setLink(houses.getName(), building->getUUID());
		//parcel->addAttribute("is_built",1);
		numberOfHouseBuild++;

	}
	Logger(Debug) << "Created Houses " << numberOfHouseBuild;
}
