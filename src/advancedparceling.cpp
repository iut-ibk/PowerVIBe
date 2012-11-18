#include "advancedparceling.h"
#include "tbvectordata.h"
#include "cgalgeometry.h"
#include "QPolygonF"
#include "QTransform"

DM_DECLARE_NODE_NAME(AdvancedParceling, CityBlocks)

AdvancedParceling::AdvancedParceling()
{
    this->cityblocks = DM::View("CITYBLOCK", DM::FACE, DM::READ);
    this->parcels = DM::View("PARCEL", DM::FACE, DM::WRITE);
    this->parcels.addAttribute("generation");

    aspectRatio = 2;
    length = 100;

    this->addParameter("AspectRatio", DM::DOUBLE, &aspectRatio);
    this->addParameter("Length", DM::DOUBLE, &length);

    std::vector<DM::View> datastream;
    datastream.push_back(cityblocks);
    datastream.push_back(parcels);

    this->addData("city", datastream);


}

/** The method is based on the minial bounding box */
void AdvancedParceling::run(){

    DM::System * city = this->getData("city");

    std::vector<std::string> block_uuids = city->getUUIDs(this->cityblocks);

    //Here comes the action
    foreach (std::string uuid, block_uuids) {
        this->createSubdevision(city, city->getFace(uuid), 0);
    }


}


void AdvancedParceling::createSubdevision(DM::System * sys, DM::Face *f, int gen)
{
    std::vector<DM::Node> box;
    std::vector<double> size;
    DM::Node center = TBVectorData::CaclulateCentroid(sys, f);
    double alpha = DM::CGALGeometry::CalculateMinBoundingBox(TBVectorData::getNodeListFromFace(sys, f), box, size);

    DM::Logger(DM::Debug) << alpha;
    if (this->length > size[0]) {
        finalSubdevision(sys, f, gen+1);
        return;
    }


    //Create New Face
    QPolygonF f_origin = TBVectorData::FaceAsQPolgonF(sys, f);

    for (int i = 0; i < 2; i++) {
        QRectF r1 (-size[0]/2.+i*size[0]/2, -size[1]/2, size[0]/2., size[1]);

        QTransform t;
        t.translate(center.getX(), center.getY());
        t.rotate(alpha);

        QPolygonF intersection = t.map(r1);

        DM::Logger(DM::Debug) << center.getX() <<" " << center.getY();

        DM::Logger(DM::Debug) << intersection[0].x() <<" " << intersection[1].y();
        QPolygonF intersected = f_origin.intersected(intersection);

        DM::Logger(DM::Debug) << intersected.size();

        std::vector<DM::Node*> newFace;
        foreach (QPointF p, intersected) {
            newFace.push_back(sys->addNode(p.x(), p.y(), 0.));
        }

        DM::Logger(DM::Debug) << newFace.size();
        if (newFace.size() < 3) {
            DM::Logger(DM::Warning) << "Advaned parceling interseciton failed";
            return;
        }


        DM::Face * f_new = sys->addFace(newFace, this->parcels);
        f_new->addAttribute("generation", gen);
        this->createSubdevision(sys, f_new, gen+1);
    }



}

void AdvancedParceling::finalSubdevision(DM::System *sys, DM::Face *f, int gen)
{
    std::vector<DM::Node> box;
    std::vector<double> size;
    QPolygonF f_origin = TBVectorData::FaceAsQPolgonF(sys, f);
    DM::Node center = TBVectorData::CaclulateCentroid(sys, f);
    double alpha = DM::CGALGeometry::CalculateMinBoundingBox(TBVectorData::getNodeListFromFace(sys, f), box, size);

    DM::Logger(DM::Debug) << alpha;
    //Finale spilts in the other direciton
    //Calculate Number of Splits
    int elements = (size[1]/(this->length/this->aspectRatio));
    //CreateSplitBox

    //0---1---2
    //0---1---2---3
    for (int i = 0; i < elements; i++) {
        QRectF r1 (-size[0]/2., -size[1]/2 + i*size[1]/((double)elements),  size[0], size[1]/(double)elements);
        QTransform t;
        t.translate(center.getX(), center.getY());
        t.rotate(alpha);

        QPolygonF intersection = t.map(r1);

        DM::Logger(DM::Debug) << center.getX() <<" " << center.getY();

        DM::Logger(DM::Debug) << intersection[0].x() <<" " << intersection[1].y();
        QPolygonF intersected = f_origin.intersected(intersection);

        if (intersected.size() == 0)
            intersected = intersection;
        DM::Logger(DM::Debug) << intersected.size();

        std::vector<DM::Node*> newFace;
        foreach (QPointF p, intersected) {
            newFace.push_back(sys->addNode(p.x(), p.y(), 0.));
        }
        DM::Face * f_new = sys->addFace(newFace, this->parcels);
        f_new->addAttribute("generation", gen++);


    }





}


