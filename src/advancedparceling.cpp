#include "advancedparceling.h"
#include "tbvectordata.h"
#include "cgalgeometry.h"
#include "dmgeometry.h"
#include "QPolygonF"
#include "QTransform"

DM_DECLARE_NODE_NAME(AdvancedParceling, CityBlocks)

AdvancedParceling::AdvancedParceling()
{
    this->cityblocks = DM::View("CITYBLOCK", DM::FACE, DM::READ);
    this->cityblocks.getAttribute("new_parcel");
    this->parcels = DM::View("PARCEL", DM::FACE, DM::WRITE);
    this->parcels.addAttribute("generation");
    this->cityblocks.getAttribute("new_parcel");

    aspectRatio = 2;
    length = 100;
    offset = 1;

    this->addParameter("AspectRatio", DM::DOUBLE, &aspectRatio);
    this->addParameter("Length", DM::DOUBLE, &length);
    this->addParameter("offest", DM::DOUBLE, & offset);

    InputViewName = "SUPERBLOCK";
    OutputViewName = "CITYBLOCK";

    this->addParameter("INPUTVIEW", DM::STRING, &InputViewName);
    this->addParameter("OUTPUTVIEW", DM::STRING, &OutputViewName);

    std::vector<DM::View> datastream;
    datastream.push_back(DM::View("dummy", DM::SUBSYSTEM, DM::MODIFY));

    this->addData("city", datastream);


}

void AdvancedParceling::init()
{
    if (InputViewName.empty() || OutputViewName.empty())
        return;

    DM::System * sys = this->getData("city");

    if (!sys)
        return;

    DM::View * InputView = sys->getViewDefinition(InputViewName);
    //DM::View * OutputView = sys->getViewDefinition(OutputViewName);

    if (!InputView)
        return;
    cityblocks = DM::View(InputView->getName(), InputView->getType(), DM::READ);

    parcels = DM::View(OutputViewName, DM::FACE, DM::WRITE);

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
        DM::Face *f  =city->getFace(uuid);
        if (f->getAttribute("new")->getDouble() > 0.01)
            this->createSubdevision(city, f, 0);
    }
}

void AdvancedParceling::createSubdevision(DM::System * sys, DM::Face *f, int gen)
{
    std::vector<DM::Node> box;
    std::vector<double> size;

    double alpha = DM::CGALGeometry::CalculateMinBoundingBox(TBVectorData::getNodeListFromFace(sys, f), box, size);// + rand() % 30-15;

    DM::Face * bb;
    std::vector<DM::Node*> l_bb;
    foreach (DM::Node  n, box) {
        l_bb.push_back(sys->addNode(n));
    }
    l_bb.push_back(l_bb[0]);

    bb = sys->addFace(l_bb);

    DM::Node center = TBVectorData::CaclulateCentroid(sys, bb);


    if (this->length*2 > size[0]) {
        finalSubdevision(sys, f, gen+1);
        return;
    }
    //Create New Face
    QPolygonF f_origin = TBVectorData::FaceAsQPolgonF(sys, f);

    int elements = size[1]/(this->length);
    if (elements < 4) {
        elements = 3;
    } else {
        elements = 2;
    }

    for (int i = 0; i < elements; i++) {
        QRectF r1 (-size[0]/2.+ i*size[0]/(double)elements,  -size[1]/2, size[0]/(double)elements, size[1]);

        //QRectF r1 (-size[0]/2., -size[1]/2 + i*size[1]/((double)elements),  size[0], size[1]/(double)elements);
        //QRectF r1 (-size[0]/2.+i*size[0]/2., -size[1]/2, size[0]/2, size[1]);
        QTransform t;
        t.rotate(alpha);

        QPolygonF intersection_tmp = t.map(r1);

        QTransform t1;
        t1.translate(center.getX(), center.getY());
        QPolygonF intersection = t1.map(intersection_tmp);
        QPolygonF intersected = f_origin.intersected(intersection);

        if (intersected.size() == 0)
            intersected = intersection;

        std::vector<DM::Node* > intersection_p;
        for (int i = 0; i < intersection.size()-1; i++) {
            QPointF & p = intersection[i];
            intersection_p.push_back(sys->addNode(DM::Node(p.x(), p.y(), 0)));
        }
        intersection_p.push_back(intersection_p[0]);

        std::vector<DM::Node> intersected_nodes = DM::CGALGeometry::IntersectFace(sys, f, sys->addFace(intersection_p));


        std::vector<DM::Node*> newFace;

        foreach (DM::Node n, intersected_nodes)
            newFace.push_back(sys->addNode(DM::Node(n.getX(), n.getY(), 0)));
        newFace.push_back(newFace[0]);

        if (newFace.size() < 3) {
            DM::Logger(DM::Warning) << "Advanced parceling interseciton failed";
            continue;
        }

        DM::Face * f_new = sys->addFace(newFace);
        this->createSubdevision(sys, f_new, gen+1);
    }
}

void AdvancedParceling::finalSubdevision(DM::System *sys, DM::Face *f, int gen)
{
    DM::Logger(DM::Debug) << "Start Final Subdevision";
    std::vector<DM::Node> box;
    std::vector<double> size;
    QPolygonF f_origin = TBVectorData::FaceAsQPolgonF(sys, f);

    double alpha = DM::CGALGeometry::CalculateMinBoundingBox(TBVectorData::getNodeListFromFace(sys, f), box, size);

    DM::Face * bb;
    std::vector<DM::Node*> l_bb;
    foreach (DM::Node  n, box) {
        l_bb.push_back(sys->addNode(n));
    }
    l_bb.push_back(l_bb[0]);

    bb = sys->addFace(l_bb);

    DM::Node center = TBVectorData::CaclulateCentroid(sys, bb);

    //DM::Logger(DM::Debug) << alpha;
    //Finale spilts in the other direciton
    //Calculate Number of Splits
    int elements = size[1]/2/(this->length/this->aspectRatio)+1;
    //CreateSplitBox
    //DM::Logger(DM::Debug) << elements;
    //0---1---2
    //0---1---2---3
    for (int i = 0; i < elements; i++) {
        QRectF r1 (-size[0]/2., -size[1]/2 + i*size[1]/((double)elements),  size[0], size[1]/(double)elements);
        QTransform t;
        t.rotate(alpha);

        QPolygonF intersection_tmp = t.map(r1);

        QTransform t1;
        t1.translate(center.getX(), center.getY());
        QPolygonF intersection = t1.map(intersection_tmp);
        std::vector<DM::Node* > intersection_p;
        for (int i = 0; i < intersection.size()-1; i++) {
            QPointF & p = intersection[i];
            intersection_p.push_back(sys->addNode(DM::Node(p.x(), p.y(), 0)));
        }
        intersection_p.push_back(intersection_p[0]);

        std::vector<DM::Node> intersected_nodes = DM::CGALGeometry::IntersectFace(sys, f, sys->addFace(intersection_p));


        std::vector<DM::Node*> newFace;

        foreach (DM::Node n, intersected_nodes)
            newFace.push_back(sys->addNode(DM::Node(n.getX(), n.getY(), 0)));
        newFace.push_back(newFace[0]);

        if (offset > 0.0001) {
            DM::Logger(DM::Debug) << "----";
            DM::Logger(DM::Debug)<< newFace.size();
            foreach (DM::Node * n, newFace) {
                DM::Logger(DM::Debug) << n->getX() <<" " << n->getY();
            }
             DM::Logger(DM::Debug) << "Start offset";
            std::vector<DM::Node> new_parcel = DM::CGALGeometry::OffsetPolygon(newFace, offset);
            if (new_parcel.size() < 3) {
                DM::Logger(DM::Warning) << "Advaned offset interseciton failed";
                return;
            }
            std::vector<DM::Node*> newFace_Offset;

            foreach (DM::Node p, new_parcel) {
                newFace_Offset.push_back(sys->addNode(p));
            }
            newFace_Offset.push_back(newFace_Offset[0]);
            newFace = newFace_Offset;
            //DM::Logger(DM::Debug) << newFace.size();
            if (newFace.size() < 3) {
                DM::Logger(DM::Warning) << "Advaned parceling interseciton failed";
                return;
            }
        }
        DM::Face * f_new = sys->addFace(newFace, this->parcels);
        f_new->addAttribute("generation", gen);
        f_new->addAttribute("new", 1);

    }

    DM::Logger(DM::Debug) << "Done Final Subdevision";





}


