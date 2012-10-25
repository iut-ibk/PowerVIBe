#include "createshadows.h"

#include <iostream>
#include <list>
#include <fstream>

#include <CGAL/Simple_cartesian.h>
#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits.h>
#include <CGAL/AABB_triangle_primitive.h>
#include <CGAL/Polyhedron_3.h>
#include <sunpos.h>
#include <QDate>

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
DM::Node CreateShadows::normalVector(std::vector<DM::Node*> nodes)
{
    if (nodes.size() != 3){
        DM::Logger(DM::Warning) << "Normal Vector needs exactly 3 Points";
        return DM::Node(0,0,0);
    }

    DM::Node n1 = *(nodes[0]) - *(nodes[1]);
    DM::Node n2 = *(nodes[1]) - *(nodes[2]);

    double x = n1.getY()*n2.getZ() - n1.getZ()*n2.getY();
    double y = n1.getZ()*n2.getX() - n1.getX()*n2.getZ();
    double z = n1.getX()*n2.getY() - n1.getY()*n2.getX();
    double l = sqrt(x*x+y*y+z*z);
    return DM::Node(x/l,y/l,z/l);
}

double CreateShadows::angelBetweenVectors(const DM::Node & n1, const DM::Node & n2)
{
    double val1 = n1.getX()*n2.getX()+n1.getY()*n2.getY()+n1.getZ()*n2.getZ();
    double N1 = n1.getX()*n1.getX()+n1.getY()*n1.getY()+n1.getZ()*n1.getZ();
    double N2 = n2.getX()*n2.getX()+n2.getY()*n2.getY()+n2.getZ()*n2.getZ();
    if (N1 == 0 || N2 == 0) {
        DM::Logger(DM::Warning) << "n1 or n2 is null!";
        return -1;
    }

    double cosangel = val1/(sqrt(N1)*sqrt(N2));
    double dir = acos(cosangel);
    //DM::Logger(DM::Debug) << "Direction: " << dir*180/pi;
    return dir;
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



CreateShadows::CreateShadows()
{
}


void CreateShadows::run()
{


    QDate date(2012, 6,1);
    DM::Node west(-1,0,0);
    DM::Node east(1,0,0);
    DM::Node north(0,1,0);
    DM::Node south(0,-1,0);
    DM::Node above(0,0,1);


    cLocation pos;
    pos.dLatitude = 47.2611;
    pos.dLongitude = -11.3928;

    ofstream ress;
    ress.open ("ress.txt");



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

            //DM::Logger(DM::Debug) << "\t" << sunloc->dAzimuth << "\t"<< sunloc->dZenithAngle;


            DM::Node n1 = this->directionSun(sunloc->dAzimuth,sunloc->dZenithAngle);

            DM::Logger(DM::Debug) << "\t" << n1.getX() << "\t" << n1.getY() << "\t" << n1.getZ();

            double w = this->angelBetweenVectors(west, n1) * 180/pi;
            double e = this->angelBetweenVectors(east, n1)* 180/pi;
            double n = this->angelBetweenVectors(north, n1)* 180/pi;
            double s = this->angelBetweenVectors(south, n1)* 180/pi;
            double a = this->angelBetweenVectors(above, n1)* 180/pi;

            if (sunloc->dZenithAngle >= 90) {
                w = 0;
                e = 0;
                n = 0;
                s = 0;
                a = 0;
            }

            ress << w << "\t"  << e << "\t" << n << "\t" << s << "\t" << a << "\n";
        }
        date = date.addDays(1000);
        DM::Logger(DM::Debug) << date.toString();
    } while (date.year()< 2013);

    ress.close();

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

