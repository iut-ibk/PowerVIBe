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

    dem = DM::View("SUPERBLOCK", DM::FACE, DM::READ);

    sunrays = DM::View("SunRays", DM::EDGE, DM::WRITE);

    sunnodes = DM::View("SunNodes", DM::NODE, DM::WRITE);
    sunnodes.addAttribute("sunrays");
    sunnodes.addAttribute("sunhours");

    mesh = DM::View("Mesh", DM::FACE, DM::WRITE);

    onlyWindows = false;
    gridSize = 1;

    this->addParameter("GridSize", DM::DOUBLE, &gridSize);

    std::vector<DM::View> data;
    data.push_back(buildings);
    data.push_back(models);
    data.push_back(sunrays);
    data.push_back(sunnodes);
    data.push_back(dem);
    data.push_back(mesh);

    createRays = false;
    this->addParameter("CreateRays", DM::BOOL, &createRays);
    this->addParameter("Only Windows", DM::BOOL, &onlyWindows);

    startday = 1;
    startmonth = 1;
    startyear = 2012;

    endday = 1;
    endmonth = 2;
    endyear = 2012;

    this->addParameter("startday", DM::INT, &startday);
    this->addParameter("startmonth", DM::INT, &startmonth);
    this->addParameter("startyear", DM::INT, &startyear);
    this->addParameter("endday", DM::INT, &endday);
    this->addParameter("endmonth", DM::INT, &endmonth);
    this->addParameter("endyear", DM::INT, &endyear);

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


    //Create Raster

    unsigned int elements_x = blockWidth/gridSize + 1;
    unsigned int elements_y = blockHeight/gridSize + 1;
    double realwidth = blockWidth / elements_x;
    double realheight = blockHeight / elements_y;

    QPolygonF h1;
    for (unsigned int i = 0; i < ns_t.size()-1; i++){
        DM::Node * n1 = ns_t[i];
        h1.push_back(QPointF(n1->getX(), n1->getY()));
    }

    std::vector<QPolygonF> holes;
    foreach (std::vector<std::string> hole, f->getHoles()) {
        QPolygonF h;
        for (unsigned int i = 0; i < hole.size(); i++) {
            DM::Node n = *(sys->getNode(hole[i]));
            DM::Node n_t =  TBVectorData::RotateVector(alphas, n);
            h.push_back(QPointF(n_t.getX(), n_t.getY()));
        }
        holes.push_back(h);
    }

    for (unsigned int x = 0; x < elements_x; x++) {
        for (unsigned int y = 0; y < elements_y; y++) {
            double x_p = minX + ((double)x+0.5) * realwidth;
            double y_p = minY + ((double)y+0.5) * realheight;
            //Cehck if Point is valid
            QPointF p(x_p, y_p);

            if (!h1.containsPoint(p, Qt::WindingFill))
                continue;
            bool isInHole = false;
            foreach (QPolygonF h1, holes) {
                if (h1.containsPoint(p, Qt::WindingFill)) {
                    isInHole = true;
                    break;
                }
            }
            if (isInHole)
                continue;
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
    //ofstream myfile;
    //myfile.open ("ress.txt");

    DM::System * city = this->getData("city");

    std::vector<std::string> model_uuids;

    std::vector<std::string> geom_uuids =  city->getUUIDs(models);
    //Add Digital Elevation May
    std::vector<std::string> elevation_uuids = city->getUUIDs(dem);

    foreach (std::string uuid, elevation_uuids) {
        model_uuids.push_back(uuid);
    }

    //Add Digital Elevation May
    std::vector<std::string> building_uuids = city->getUUIDs(buildings);

    foreach (std::string uuid, building_uuids) {
        DM::Component * cmp = city->getComponent(uuid);
        std::vector<DM::LinkAttribute> links = cmp->getAttribute("Geometry")->getLinks();
        foreach (DM::LinkAttribute link, links) {
            model_uuids.push_back(link.uuid);
        }
    }

    //Init Triangles
    std::list<Triangle> triangles;
    foreach (std::string uuid, geom_uuids) {
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
    QDate startDate(startyear, startmonth,startday);
    QDate endDate(endyear, endmonth,endday);
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
        //myfile << uuid << "\t" << f->getAttribute("type")->getString() << "\n";
        if (f->getAttribute("type")->getString() != "window" && this->onlyWindows)
            continue;
        std::vector<DM::Node*> nodes = TBVectorData::getNodeListFromFace(city, f);
        DM::Node dN1 = *(nodes[1]) - *(nodes[0]);
        DM::Node dN2 = *(nodes[2]) - *(nodes[0]);
        DM::Node dir = TBVectorData::NormalVector(dN1, dN2);
        //std::vector<DM::Node> centers = this->createRaster(city, f);
        std::vector<DM::Node> centers = DM::CGALGeometry::RegularFaceTriangulation(city, f, gridSize);
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
        //Create Mesh
        std::vector<DM::Node*> nodesToCheck(numberOfCenters);
        for (int c = 0; c < numberOfCenters/3; c++) {
            std::vector<DM::Node*> tnodes;
            for (int j = 0; j < 3; j++) {
                DM::Node * n = city->addNode(centers[c*3+j], sunnodes);
                tnodes.push_back(n);
                nodesToCheck[c*3+j] = n;
            }
            tnodes.push_back(tnodes[0]);
            city->addFace(tnodes, this->mesh);
        }

        int newnumberOfCenters = nodesToCheck.size();
        newnumberOfCenters++;
        //#pragma omp parallel for
        for (int c = 0; c < numberOfCenters; c++) {
            DM::Node * n1 =  nodesToCheck[c];
            n1->addAttribute("sunHoursNode", 0);
            std::vector<double> color(3);
            color[2] = 1;
            n1->getAttribute("color")->setDoubleVector(color);
            f->getAttribute("SunRays")->setLink("SunRays", n1->getUUID());

            std::vector<std::string> dates;

            std::vector<double> angles;

            QDate date = startDate;
            int dayInSimulation = -1;
            int hoursInSimulation = -1;

            do {
                dayInSimulation++;
                int sunHoursNode = 0;
                for (int h = 0; h < 24; h++) {

                    hoursInSimulation++;
                    if (!sunPos[hoursInSimulation])
                        continue;

                    double w = directions[hoursInSimulation];

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
                    sunHoursNode++;
                    n1->addAttribute("sunhours", sunHoursNode);
                    if (sunHoursNode <= 16) {
                        color[0] = sunHoursNode/(16.);
                        color[1] = 1- sunHoursNode/(16.);
                        color[2] = 0;
                    }
                    if (sunHoursNode <= 16/2) {
                        color[0] = 2*sunHoursNode/(16.);
                        color[1] = 1;
                        color[2] = 0;
                    }
                    if (sunHoursNode <= 16/4) {
                        color[1] = 4*sunHoursNode/(16.);
                        color[2] = 1. - (sunHoursNode  * 4)/16.;
                    }
                    color[0] = sunHoursNode/(16.);
                    color[2] = 1. - sunHoursNode/16.;
                    n1->getAttribute("color")->setDoubleVector(color);
                    if (!createRays )
                        continue;
                    DM::Node * n2 = city->addNode(*n1+*(sunPos[hoursInSimulation])*gridSize/2.);
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


        //for (int i = 0; i < solarRadiation.size(); i++)
        //myfile << day_dates[i] << "\t" << solarHours[i]<< "\t" << solarRadiation[i] <<"\n";

    }

    for (int i = 0; i < numberOfDays*24; i++) {
        if (sunPos[i])
            delete sunPos[i];
        delete sunLocs[i];
    }



}

