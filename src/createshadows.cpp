#include "createshadows.h"

#include <tbvectordata.h>

#include <iostream>
#include <list>
#include <fstream>
#include <cgalgeometry.h>

#include <CGAL/Simple_cartesian.h>
#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits.h>
#include <CGAL/AABB_triangle_primitive.h>
#include <CGAL/Polyhedron_3.h>
#include <sunpos.h>
#include <QDate>


#include <ogr_spatialref.h>
#include <ogrsf_frmts.h>


typedef CGAL::Simple_cartesian<double> K;

typedef K::FT FT;
typedef K::Ray_3 Ray;
typedef K::Line_3 Line;
typedef K::Point_3 Point;
typedef K::Triangle_3 Triangle;
typedef CGAL::Polyhedron_3<K> Polyhedron;

typedef std::list<Triangle>::iterator Iterator;
typedef CGAL::AABB_triangle_primitive<K,Iterator> Primitive;
typedef CGAL::AABB_traits<K, Primitive> AABB_triangle_traits;
typedef CGAL::AABB_tree<AABB_triangle_traits> Tree;



DM_DECLARE_NODE_NAME(CreateShadows, PowerVIBe)

CreateShadows::CreateShadows()
{
    buildings = DM::View("Building", DM::COMPONENT, DM::READ);
    buildings.getAttribute("Model");

    models = DM::View("Model", DM::FACE, DM::READ);

    sunrays = DM::View("SunRays", DM::EDGE, DM::WRITE);

    std::vector<DM::View> data;
    //data.push_back(buildings);
    data.push_back(models);
    data.push_back(sunrays);

    this->addData("city", data);

}



void CreateShadows::transformCooridnates(double &x, double &y)
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



DM::Node CreateShadows::directionSun(double dAzimuth, double dZenithAngle)
{
    double z = cos(dZenithAngle/180*pi);
    double x = sin(dAzimuth/180*pi) * sin(dZenithAngle/180*pi);
    double y = cos(dAzimuth/180*pi) * sin(dZenithAngle/180*pi);

    return DM::Node(x,y,z);



}

void CreateShadows::testdirectionSun()
{
    DM::Node n1 = this->directionSun(0,0);
    DM::Logger(DM::Debug) << n1.getX() << n1.getY() << n1.getZ();

    n1 = this->directionSun(90,0);
    DM::Logger(DM::Debug) << n1.getX() << n1.getY() << n1.getZ();

    n1 = this->directionSun(90,90);
    DM::Logger(DM::Debug) << n1.getX() << n1.getY() << n1.getZ();


    n1 = this->directionSun(180,180);
    DM::Logger(DM::Debug) << n1.getX() << n1.getY() << n1.getZ();

    n1 = this->directionSun(270,270);
    DM::Logger(DM::Debug) << n1.getX() << n1.getY() << n1.getZ();
}





void CreateShadows::run()
{

    DM::System * city = this->getData("city");


    cLocation pos;
    pos.dLatitude = 47.2611;
    pos.dLongitude = -11.3928;

    std::vector<std::string> model_uuids =  city->getUUIDs(models);

    foreach (std::string uuid, model_uuids) {
        QDate date(2012, 1,1);
        //Create Face Point
        DM::Face * f = city->getFace(uuid);
        std::vector<DM::Node> triangels = DM::CGALGeometry::FaceTriangulation(city, f);
        DM::Logger(DM::Debug) << "Face";
        foreach (DM::Node n, triangels)
            DM::Logger(DM::Debug) << n.getX() << "\t" << n.getY() << "\t" << n.getZ();

  /*      DM::Node center = TBVectorData::CentroidPlane3D(city, f);

        std::vector<DM::Node*> nodes = TBVectorData::getNodeListFromFace(city, f);

        DM::Node dir = TBVectorData::NormalVector(*(nodes[0]), *(nodes[1]));
        DM::Node * n1 =  city->addNode(center);

        do {
            cTime datum;
            datum.iYear = date.year();
            datum.iMonth = date.month();
            datum.iDay = date.day();
            datum.dHours = 0;
            datum.dMinutes = 0;
            datum.dSeconds = 0;
            for (int h = 0; h < 24; h++) {
                datum.dHours = h;
                cSunCoordinates * sunloc = new cSunCoordinates();
                sunpos(datum, pos, sunloc);
                DM::Node sun = this->directionSun(sunloc->dAzimuth,sunloc->dZenithAngle);
                if (sunloc->dAzimuth < 90 || sunloc->dAzimuth > 270)
                    continue;
                double w = TBVectorData::AngelBetweenVectors(dir, sun) * 180/pi;

                DM::Node * n2 = city->addNode(*n1+sun);

                DM::Edge * e = city->addEdge(n1, n2, this->sunrays);
                std::stringstream datestring;
                datestring << date.toString("yyyy-MM-dd").toStdString();
                QTime t(datum.dHours, datum.dMinutes, datum.dSeconds);
                datestring << "T" <<  t.toString("hh:mm:ss").toStdString() << "Z";

                e->addAttribute("date", datestring.str());

            }
            date = date.addDays(1);
        } while (date.year()< 2013);*/
    }


    //this->testdirectionSun();
    /*Point a(1.0, 0.0, 0.0);
    Point b(0.0, 1.0, 0.0);
    Point c(0.0, 0.0, 1.0);
    Point d(0.0, 0.0, 0.0);

    std::list<Triangle> triangles;
    triangles.push_back(Triangle(a,b,c));
    triangles.push_back(Triangle(a,b,d));
    triangles.push_back(Triangle(a,d,c));

    // constructs AABB tree
    Tree tree(triangles.begin(),triangles.end());

    // counts #intersections
    Ray ray_query(a,b);
    std::cout << tree.number_of_intersected_primitives(ray_query)
        << " intersections(s) with ray query" << std::endl;

    // compute closest point and squared distance
    Point point_query(2.0, 2.0, 2.0);
    Point closest_point = tree.closest_point(point_query);
    std::cerr << "closest point is: " << closest_point << std::endl;
    FT sqd = tree.squared_distance(point_query);
    std::cout << "squared distance: " << sqd << std::endl;*/



}

