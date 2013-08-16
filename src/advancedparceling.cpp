#include "advancedparceling.h"
#include "tbvectordata.h"
#include "cgalgeometry.h"
#include "dmgeometry.h"
#include "QPolygonF"
#include "QTransform"
#include <cgalgeometry_p.h>
#include <tbvectordata.h>
#include <cgalgeometry.h>

DM_DECLARE_NODE_NAME(AdvancedParceling, CityBlocks)

AdvancedParceling::AdvancedParceling()
{
	this->cityblocks = DM::View("CITYBLOCK", DM::FACE, DM::READ);
	this->cityblocks.getAttribute("selected");
	this->parcels = DM::View("PARCEL", DM::FACE, DM::WRITE);
	this->parcels.addAttribute("selected");
	this->bbs = DM::View("BBS", DM::FACE, DM::WRITE);
	this->bbs.addAttribute("generation");

	this->parcels.addAttribute("generation");

	aspectRatio = 2;
	length = 100;
	offset = 1;
	remove_new = false;

	this->addParameter("AspectRatio", DM::DOUBLE, &aspectRatio);
	this->addParameter("Length", DM::DOUBLE, &length);
	this->addParameter("offset", DM::DOUBLE, & offset);
	this->addParameter("remove_new", DM::BOOL, & remove_new);

	InputViewName = "SUPERBLOCK";
	OutputViewName = "CITYBLOCK";

	this->addParameter("INPUTVIEW", DM::STRING, &InputViewName);
	this->addParameter("OUTPUTVIEW", DM::STRING, &OutputViewName);

	std::vector<DM::View> datastream;
	//datastream.push_back(bbs);
	datastream.push_back(DM::View("dummy", DM::SUBSYSTEM, DM::MODIFY));

	this->addData("city", datastream);


}

void AdvancedParceling::init()
{
	if (InputViewName.empty() || OutputViewName.empty())
		return;

	DM::View InputView = this->getViewInStream("city", InputViewName);

	if (InputView.getType() == -1)
		return;
	cityblocks = DM::View(InputView.getName(), InputView.getType(), DM::READ);
	this->cityblocks.getAttribute("selected");
	parcels = DM::View(OutputViewName, DM::FACE, DM::WRITE);
	this->parcels.addAttribute("selected");
	this->parcels.addAttribute("generation");

	std::vector<DM::View> datastream;


	datastream.push_back(cityblocks);
	datastream.push_back(parcels);
	//datastream.push_back(bbs);


	this->addData("city", datastream);

}

/** The method is based on the minial bounding box */
void AdvancedParceling::run(){

	DM::System * city = this->getData("city");

	DM::System workingSys;

	mforeach (DM::Component * c, city->getAllComponentsInView(this->cityblocks)) {
		DM::Face * f = static_cast<DM::Face *> (c);
		if (f->getAttribute("selected")->getDouble() < 0.01)
			continue;
		DM::Face * fnew = TBVectorData::CopyFaceGeometryToNewSystem(f, &workingSys);
		workingSys.addComponentToView(fnew, this->cityblocks);
	}

	//Here comes the action
	mforeach (DM::Component * c, workingSys.getAllComponentsInView(this->cityblocks)) {
		DM::Face * f = static_cast<DM::Face *> (c);
		this->createSubdevision(&workingSys, f, 0);
	}

	createFinalFaces(&workingSys, city, this->parcels);

	if (!remove_new)
		return;

	mforeach (DM::Component * c, city->getAllComponentsInView(this->cityblocks)) {
		DM::Face * f = static_cast<DM::Face *> (c);
		f->addAttribute("selected", 0);
	}


}

void AdvancedParceling::createSubdevision(DM::System * sys, DM::Face *f, int gen)
{
	std::vector<DM::Node> box;
	std::vector<double> size;

	double alpha = DM::CGALGeometry::CalculateMinBoundingBox(TBVectorData::getNodeListFromFace(sys, f), box, size);

	DM::Face * bb;
	std::vector<DM::Node*> l_bb;
	foreach (DM::Node  n, box) {
		l_bb.push_back(sys->addNode(n));
	}
	l_bb.push_back(l_bb[0]);

	bb = sys->addFace(l_bb);

	DM::Node center = TBVectorData::CaclulateCentroid(sys, bb);

	double x_c = center.getX();
	double y_c = center.getY();

	if (this->length*2 > size[0]) {
		finalSubdevision(sys, f, gen+1);
		return;
	}
	//Create New Face
	int elements = size[1]/(this->length);
	elements = 2;
	for (int i = 0; i < 2; i++) {
		double l = size[0];
		double w = size[1];
		QRectF r1 (-l/2.+ i*l/(double)elements,  -w/2-10, l/(double)elements,w+10);

		QTransform t;
		t.rotate(alpha);

		QPolygonF intersection_tmp = t.map(r1);

		QTransform t1;
		t1.translate(x_c, y_c);
		QPolygonF intersection = t1.map(intersection_tmp);

		std::vector<DM::Node* > intersection_p;
		for (int i = 0; i < intersection.size()-1; i++) {
			QPointF & p = intersection[i];
			intersection_p.push_back(sys->addNode(DM::Node(p.x(), p.y(), 0)));
		}
		intersection_p.push_back(intersection_p[0]);
		DM::Face * bb = sys->addFace(intersection_p, bbs);

		bb->addAttribute("generation", gen);
		std::vector<DM::Face *> intersected_faces = DM::CGALGeometry::IntersectFace(sys, f, bb);

		if (intersected_faces.size() == 0) {
			DM::Logger(DM::Warning) << "Advanced parceling createSubdevision interseciton failed";
			continue;
		}

		foreach (DM::Face * f_new ,intersected_faces ) {
			f_new->addAttribute("generation", gen);
			this->createSubdevision(sys, f_new, gen+1);
		}
	}
}

void AdvancedParceling::finalSubdevision(DM::System *sys, DM::Face *f, int gen)
{
	//DM::Logger(DM::Debug) << "Start Final Subdevision";
	std::vector<DM::Node> box;
	std::vector<double> size;

	double alpha = DM::CGALGeometry::CalculateMinBoundingBox(TBVectorData::getNodeListFromFace(sys, f), box, size);

	DM::Face * bb;
	std::vector<DM::Node*> l_bb;
	foreach (DM::Node  n, box) {
		l_bb.push_back(sys->addNode(n));
	}
	l_bb.push_back(l_bb[0]);

	bb = sys->addFace(l_bb);

	DM::Node center = TBVectorData::CaclulateCentroid(sys, bb);
	//Finale spilts in the other direciton
	//Calculate Number of Splits
	int elements = size[1]/2/(this->length/this->aspectRatio)+1;
	//CreateSplitBox
	//DM::Logger(DM::Debug) << elements;
	//0---1---2
	//0---1---2---3
	for (int i = 0; i < elements; i++) {
		QRectF r1 (-size[0]/2.-10, -size[1]/2 + i*size[1]/((double)elements),  size[0]+10, size[1]/(double)elements);
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

		std::vector<DM::Face *> intersected_face = DM::CGALGeometry::IntersectFace(sys, f, sys->addFace(intersection_p));

		if (intersected_face.size() == 0) {
			DM::Logger(DM::Warning) << "Final Intersection Failed";
			continue;
		}
		foreach (DM::Face * nF ,intersected_face ) {

			std::vector<DM::Node*> newFace = nF->getNodePointers();
			if (offset > 0.0001) {
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
					continue;
				}
			}
			DM::Face * f_new = sys->addFace(newFace, this->parcels);
			f_new->addAttribute("generation", gen);

		}
	}

}

void AdvancedParceling::createFinalFaces(DM::System *workingsys, DM::System * sys, DM::View v)
{
	DM::SpatialNodeHashMap sphs(sys,10000,false);

	Arrangement_2::Edge_iterator					eit;
	Arrangement_2::Face_const_iterator              fit;
	Arrangement_2::Ccb_halfedge_const_circulator    curr;
	Segment_list_2									segments;
	Arrangement_2									arr;
	segments = DM::CGALGeometry_P::EdgeToSegment2D(workingsys, v);

	insert (arr, segments.begin(), segments.end());

	int faceCounter = 0;
	int NodeCounter = 0;
	for (fit = arr.faces_begin(); fit != arr.faces_end(); ++fit) {
		if (fit->is_unbounded()) {
			DM::Logger(DM::Debug) << "Unbounded Face";
			continue;
		}
		curr = fit->outer_ccb();

		Arrangement_2::Ccb_halfedge_const_circulator hec = fit->outer_ccb();
		Arrangement_2::Ccb_halfedge_const_circulator end = hec;
		Arrangement_2::Ccb_halfedge_const_circulator next = hec;
		bool isBoundary = false;
		if(hec->twin()->face()->is_unbounded()) {
			isBoundary = true;
		}
		std::vector<Point_2> ressults_P2;
		std::vector<DM::Node *> vp;
		next++;
		if (hec->curve().target() == next->curve().source() || hec->curve().target() == next->curve().target()) {
			ressults_P2.push_back(hec->curve().target());
			float x = CGAL::to_double(hec->curve().target().x());
			float y = CGAL::to_double(hec->curve().target().y());
			if (!sphs.findNode(x,y,0.001)){
				DM::Logger(DM::Debug) << "Found\t" << x << "\t" << y;
				NodeCounter++;
			}
			DM::Node * n = sphs.addNode(x,y,0,0.001);
			if (isBoundary) n->addAttribute("boundary_node", 1);
			vp.push_back(n);

		} else {
			ressults_P2.push_back(hec->curve().source());
			float x = CGAL::to_double(hec->curve().source().x());
			float y = CGAL::to_double(hec->curve().source().y());
			if (!sphs.findNode(x,y,0.001)){
				DM::Logger(DM::Debug) << "Found\t" << x << "\t" << y;
				NodeCounter++;
			}
			DM::Node * n = sphs.addNode(x,y,0,0.001);
			if (isBoundary) n->addAttribute("boundary_node", 1);
			vp.push_back(n);

		}
		do{
			++hec;
			bool source = false;
			bool target = false;
			for ( unsigned int i = 0; i < ressults_P2.size(); i++) {
				if ( ressults_P2[i] == hec->curve().target() ) {
					target = true;
				}  else if ( ressults_P2[i] == hec->curve().source() )   {
					source = true;
				}

			}

			if (source == false ) {
				ressults_P2.push_back(hec->curve().source());
				float x = CGAL::to_double(hec->curve().source().x());
				float y = CGAL::to_double(hec->curve().source().y());
				if (!sphs.findNode(x,y,0.001)){
					DM::Logger(DM::Debug) << "Found\t" << x << "\t" << y;
					NodeCounter++;

				}
				DM::Node * n = sphs.addNode(x,y,0,0.001);
				if (isBoundary) n->addAttribute("boundary_node", 1);
				vp.push_back(n);
			}
			if (target == false ) {
				ressults_P2.push_back(hec->curve().target());
				float x = CGAL::to_double(hec->curve().target().x());
				float y = CGAL::to_double(hec->curve().target().y());
				if (!sphs.findNode(x,y,0.001)){
					DM::Logger(DM::Debug) << "Found\t" << x << "\t" << y;
					NodeCounter++;
				}
				DM::Node * n = sphs.addNode(x,y,0,0.001);
				if (isBoundary) n->addAttribute("boundary_node", 1);
				vp.push_back(n);
			}

		}
		while(hec != end );
		faceCounter++;
		if (vp.size() < 3)
			continue;
		vp.push_back(vp[0]);
		DM::Face * f = sys->addFace(vp, v);
		f->addAttribute("selected", 1);
	}

	DM::Logger(DM::Standard) << "FaceCounter " << faceCounter;
	DM::Logger(DM::Standard) << "NodeCounter " << NodeCounter;
}


