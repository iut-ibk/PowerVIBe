#include "createshadows.h"
#include <omp.h>
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
typedef K::Segment_3 Segment;
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
    models.addLinks("SunRays", sunrays);

    sunrays = DM::View("SunRays", DM::EDGE, DM::WRITE);

    sunnodes = DM::View("SunNodes", DM::NODE, DM::WRITE);
    sunnodes.addAttribute("sunrays");

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

std::vector<DM::Node> CreateShadows::createRaster(DM::System *sys, DM::Face *f)
{
    //Make Place Plane
    std::vector<DM::Node*> nodeList = TBVectorData::getNodeListFromFace(sys, f);

    std::vector<DM::Node> returnNodes_t;

    double E[3][3];
    TBVectorData::CorrdinateSystem( DM::Node(0,0,0), DM::Node(1,0,0), DM::Node(0,1,0), E);

    double E_to[3][3];

    TBVectorData::CorrdinateSystem( *(nodeList[0]), *(nodeList[1]), *(nodeList[2]), E_to);

    double alphas[3][3];
    TBVectorData::RotationMatrix(E, E_to, alphas);

    double alphas_t[3][3];
    for (int i = 0; i < 3; i++){
        for (int j = 0; j < 3; j++){
            alphas_t[j][i] =  alphas[i][j];
        }
    }

    DM::System transformedSys;

    std::vector<DM::Node*> ns_t;
    double const_height;
    for (unsigned int i = 0; i < nodeList.size(); i++) {
        DM::Node n = *(nodeList[i]);
        DM::Node n_t =  TBVectorData::RotateVector(alphas, n);
        ns_t.push_back(transformedSys.addNode(n_t));
        const_height = n_t.getZ();
    }

    double minX = 0;
    double maxX = 0;
    double minY = 0;
    double maxY = 0;
    for (unsigned int i = 0; i < ns_t.size(); i++){
        DM::Node * n1 = ns_t[i];
        if (i == 0) {
            minX = n1->getX();
            maxX = n1->getX();
            minY = n1->getY();
            maxY = n1->getY();
        }

        if(minX > n1->getX())
            minX = n1->getX();
        if(maxX < n1->getX())
            maxX = n1->getX();

        if(minY > n1->getY())
            minY = n1->getY();
        if(maxY < n1->getY())
            maxY = n1->getY();
    }
    double blockWidth = maxX - minX;
    double blockHeight = maxY - minY;


    DM::Logger(DM::Debug) << blockHeight;
    DM::Logger(DM::Debug) << blockWidth;


    //Create Parcels

    unsigned int elements_x = blockWidth/2;
    unsigned int elements_y = blockHeight/2;
    double realwidth = blockWidth / elements_x;
    double realheight = blockHeight / elements_y;



    for (unsigned int x = 0; x < elements_x; x++) {
        for (unsigned int y = 0; y < elements_y; y++) {
            double x_p = minX + ((double)x+0.5) * realwidth;
            double y_p = minY + ((double)y+0.5) * realheight;
            returnNodes_t.push_back(DM::Node(x_p,y_p, const_height));
        }
    }

    std::vector<DM::Node> returnNodes;
    for (unsigned int  i = 0; i <returnNodes_t.size(); i++ ) {
        returnNodes.push_back(TBVectorData::RotateVector(alphas_t,returnNodes_t[i]));
    }

    return returnNodes;

}





void CreateShadows::run()
{
    DM::System * city = this->getData("city");


    cLocation pos;
    pos.dLatitude = 47.2611;
    pos.dLongitude = -11.3928;

    std::vector<std::string> model_uuids =  city->getUUIDs(models);

    //Init Triangles
    std::list<Triangle> triangles;
    foreach (std::string uuid, model_uuids) {
        DM::Face * f = city->getFace(uuid);
        std::vector<DM::Node> tri = DM::CGALGeometry::FaceTriangulation(city, f);
        for(unsigned int i = 0; i < tri.size(); i +=3){
            std::vector<Point> points;
            for(unsigned j = 0; j < 3; j++){
                points.push_back(Point(tri[j+i].getX(), tri[j+i].getY(), tri[j+i].getZ()));
            }
            triangles.push_back(Triangle(points[0], points[1], points[2]));
        }
    }
    // constructs AABB tree
    Tree tree(triangles.begin(),triangles.end());


    int nuuids = model_uuids.size();

    for (int i = 0; i < nuuids; i++){

        std::string uuid = model_uuids[i];
        DM::Logger(DM::Debug) << "Face " << uuid;
        //Create Face Point
        DM::Face * f = city->getFace(uuid);
        std::vector<DM::Node*> nodes = TBVectorData::getNodeListFromFace(city, f);
        DM::Node dir = TBVectorData::NormalVector(*(nodes[0]), *(nodes[1]));

        std::vector<DM::Node> centers = this->createRaster(city, f);
        foreach (DM::Node center, centers) {
            //DM::Node center = TBVectorData::CentroidPlane3D(city, f);
            DM::Node * n1 =  city->addNode(center, sunnodes);

            f->getAttribute("SunRays")->setLink("SunRays", n1->getUUID());

            std::vector<std::string> dates;
            std::vector<double> angles;
            QDate date(2012, 1,1);
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

                    //Below horizon
                    if (sunloc->dZenithAngle > 90)
                        continue;
                    DM::Node sun = this->directionSun(sunloc->dAzimuth,sunloc->dZenithAngle);

                    double w = TBVectorData::AngelBetweenVectors(dir, sun) * 180/pi;

                    //No direct sun
                    if (w  > 90 && w < 270)
                        continue;

                    DM::Node pn1 = *n1+sun*0.001;
                    DM::Node pn2 = *n1+sun*1000;

                    Point p1(pn1.getX(), pn1.getY(), pn1.getZ());
                    Point p2(pn2.getX(), pn2.getY(), pn2.getZ());
                    Segment segment_query(p1,p2);

                    //Check if Intersects
                    if (tree.number_of_intersected_primitives(segment_query)> 0) {
                        continue;
                    }

                    DM::Node * n2 = city->addNode(*n1+sun);
                    DM::Edge * e = city->addEdge(n1, n2, this->sunrays);
                    std::stringstream datestring;
                    datestring << date.toString("yyyy-MM-dd").toStdString();
                    QTime t(datum.dHours, datum.dMinutes, datum.dSeconds);
                    datestring << "T" <<  t.toString("hh:mm:ss").toStdString() << "Z";

                    std::stringstream datestring_dm;
                    datestring_dm << date.toString("yyyy-MM-dd").toStdString();
                    datestring_dm << " " <<  t.toString("hh:mm:ss").toStdString();
                    dates.push_back(datestring_dm.str());

                    e->addAttribute("date", datestring.str());
                    angles.push_back(w);
                }
                n1->getAttribute("sunrays")->addTimeSeries(dates, angles);
                date = date.addDays(1);
            } while (date.year()< 2013);
        }
    }


    //this->testdirectionSun();



}

