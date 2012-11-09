#include "createshadows.h"
#include <tbvectordata.h>
#include <solarrediation.h>

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
#include <omp.h>

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
    buildings = DM::View("BUILDING", DM::COMPONENT, DM::READ);
    buildings.getAttribute("Geometry");

    models = DM::View("Geometry", DM::FACE, DM::READ);
    models.addAttribute("solar_radiation_dayly");
    models.addAttribute("solar_radiation_hourly");
    models.addAttribute("solar_hours_dayly");
    models.addLinks("SunRays", sunrays);


    sunrays = DM::View("SunRays", DM::EDGE, DM::WRITE);

    sunnodes = DM::View("SunNodes", DM::NODE, DM::WRITE);
    sunnodes.addAttribute("sunrays");

    onlyWindows = false;

    std::vector<DM::View> data;
    data.push_back(buildings);
    data.push_back(models);
    data.push_back(sunrays);

    createRays = false;
    this->addParameter("CreateRays", DM::BOOL, &createRays);
    this->addParameter("Only Windows", DM::BOOL, &onlyWindows);

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



void CreateShadows::directionSun(DM::Node * n, double dAzimuth, double dZenithAngle)
{
    n->setZ(cos(dZenithAngle/180*pi));
    n->setX(sin(dAzimuth/180*pi) * sin(dZenithAngle/180*pi));
    n->setY(cos(dAzimuth/180*pi) * sin(dZenithAngle/180*pi));
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


    //Create Parcels

    unsigned int elements_x = blockWidth/2 + 1;
    unsigned int elements_y = blockHeight/2 + 1;
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

void CreateShadows::caclulateSunPositions(const QDate &start, const QDate &end, std::vector<DM::Node*> & sunPos , std::vector<cSunCoordinates *> & sunLoc)
{
    QDate date = start;
    int totalHours = -1;

    cLocation pos;
    pos.dLatitude = 47.2611;
    pos.dLongitude = -11.3928;

    do {

        cTime datum;
        datum.iYear = date.year();
        datum.iMonth = date.month();
        datum.iDay = date.day();
        datum.dHours = 0;
        datum.dMinutes = 0;
        datum.dSeconds = 0;


        for (int h = 0; h < 24; h++) {
            totalHours++;
            datum.dHours = h;
            sunLoc[totalHours] = new cSunCoordinates();
            sunpos(datum, pos, sunLoc[totalHours]);

            //Below horizon
            if (sunLoc[totalHours]->dZenithAngle > 90) {
                sunPos[totalHours] = 0;
                continue;
            }

            sunPos[totalHours] = new DM::Node();
            this->directionSun(sunPos[totalHours], sunLoc[totalHours]->dAzimuth, sunLoc[totalHours]->dZenithAngle);

        }


        date = date.addDays(1);
    } while (date < end);
}

void CreateShadows::run()
{
    ofstream myfile;
    myfile.open ("ress.txt");

    DM::System * city = this->getData("city");

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


    tree.build();
    QDate startDate(2012, 1,1);
    QDate endDate(2013, 1,1);
    QDate date = startDate;
    int numberOfDays = startDate.daysTo(endDate);
    std::vector<std::string> day_dates;
    for (int i = 0; i < numberOfDays; i++) {
        std::stringstream datestring_dm;
        datestring_dm << date.toString("yyyy-MM-dd").toStdString();
        day_dates.push_back(datestring_dm.str());
        date = date.addDays(1);
    }

    int nuuids = model_uuids.size();


    std::vector<DM::Node * > sunPos(numberOfDays*24);

    std::vector<cSunCoordinates *> sunLocs(numberOfDays*24);
    for (int i = 0; i < numberOfDays*24; i++) {
        sunPos[i] = 0;
        sunLocs[i] = 0;
    }

    //Prepare sun pos vector
    this->caclulateSunPositions(startDate, endDate, sunPos, sunLocs);


    int TODO = nuuids;
    for (int i = 0; i < nuuids; i++){
        //CreateInitalSolarRadiationVector
        std::vector<double> solarRadiation(numberOfDays, 0);
        std::vector<double> solarHours(numberOfDays, 0);
        std::string uuid = model_uuids[i];
        //DM::Logger(DM::Debug) << "Face " << uuid;
        //Create Face Point
        DM::Face * f = city->getFace(uuid);
        myfile << uuid << "\t" << f->getAttribute("type")->getString() << "\n";
        if (f->getAttribute("type")->getString() != "window")
            continue;
        std::vector<DM::Node*> nodes = TBVectorData::getNodeListFromFace(city, f);
        DM::Node dN1 = *(nodes[1]) - *(nodes[0]);
        DM::Node dN2 = *(nodes[2]) - *(nodes[0]);
        DM::Node dir = TBVectorData::NormalVector(dN1, dN2);
        std::vector<DM::Node> centers = this->createRaster(city, f);

        //Angle Between Vectors since face is planar the it stays the same for all positions
        std::vector<double> directions(numberOfDays*24,0);
        std::vector<double> beamradiation(numberOfDays*24,0);
        for (int h = 0; h < numberOfDays*24; h++){
            if (!sunPos[h])
                continue;
            directions[h]= TBVectorData::AngelBetweenVectors(dir,  *(sunPos[h]) ) * 180/pi;
            beamradiation[h] = SolarRediation::BeamRadiation((int)h/24, 500, (90 -sunLocs[h]->dZenithAngle)/180.*pi ,directions[h]/180.*pi);
        }


        date = startDate;
        int numberofCheckedIntersections = 0;
        int numberOfCenters = centers.size();
        //#pragma omp parallel for
        for (int c = 0; c < numberOfCenters; c++) {
            DM::Node * n1 =  city->addNode(centers[c], sunnodes);
            f->getAttribute("SunRays")->setLink("SunRays", n1->getUUID());

            std::vector<std::string> dates;

            std::vector<double> angles;

            QDate date = startDate;
            int dayInSimulation = -1;
            int hoursInSimulation = -1;

            do {
                dayInSimulation++;
                for (int h = 0; h < 24; h++) {
                    hoursInSimulation++;
                    if (!sunPos[hoursInSimulation])
                        continue;

                    double w = directions[hoursInSimulation];

                    //DM::Logger(DM::Debug) << h << " " << sunPos[hoursInSimulation]->getZ() << " " << w <<" " << 90 -sunLocs[hoursInSimulation]->dZenithAngle;
                    //No direct sun
                    if (w  > 90 && w < 270)
                        continue;

                    double x1 = n1->getX();
                    double y1 = n1->getY();
                    double z1 = n1->getZ();

                    double sx1 = sunPos[hoursInSimulation]->getX();
                    double sy1 = sunPos[hoursInSimulation]->getY();
                    double sz1 = sunPos[hoursInSimulation]->getZ();

                    Point p1(x1 + sx1 * 0.0001 , y1 + sy1 * 0.0001, z1 + sz1 * 0.0001);
                    Point p2(x1 + sx1 * 10000 , y1 + sy1 * 10000, z1 + sz1 * 10000);
                    Segment segment_query(p1,p2);

                    numberofCheckedIntersections++;
                    //Check if Intersects
                    if (tree.do_intersect(segment_query)> 0) {
                        continue;
                    }

                    double radiation = beamradiation[hoursInSimulation];
                    solarRadiation[dayInSimulation] = solarRadiation[dayInSimulation] + radiation/(double) centers.size();
                    solarHours[dayInSimulation] = solarHours[dayInSimulation] + 1/(double) centers.size();
                    if (!createRays)
                        continue;
                    DM::Node * n2 = city->addNode(*n1+*(sunPos[hoursInSimulation]));
                    DM::Edge * e = city->addEdge(n1, n2, this->sunrays);
                    std::stringstream datestring;
                    datestring << date.toString("yyyy-MM-dd").toStdString();
                    QTime t(h, 0,0);
                    datestring << "T" <<  t.toString("hh:mm:ss").toStdString() << "Z";

                    std::stringstream datestring_dm;
                    datestring_dm << date.toString("yyyy-MM-dd").toStdString();
                    datestring_dm << " " <<  t.toString("hh:mm:ss").toStdString();
                    dates.push_back(datestring_dm.str());

                    e->addAttribute("date", datestring.str());

                    angles.push_back(w);
                }
                date = date.addDays(1);

                if (!createRays)
                    n1->getAttribute("sunrays")->addTimeSeries(dates, angles);
            } while (date < endDate);
        }
        TODO--;
        DM::Logger(DM::Debug) << "intersections checked " << numberofCheckedIntersections << " TODO " << TODO;
        f->getAttribute("solar_radiation_dayly")->addTimeSeries(day_dates, solarRadiation);
        f->getAttribute("solar_hourss_dayly")->addTimeSeries(day_dates, solarHours);


        for (int i = 0; i < solarRadiation.size(); i++)
            myfile << day_dates[i] << "\t" << solarHours[i]<< "\t" << solarRadiation[i] <<"\n";

    }

    for (int i = 0; i < numberOfDays*24; i++) {
        if (sunPos[i])
            delete sunPos[i];
        delete sunLocs[i];
    }



}

