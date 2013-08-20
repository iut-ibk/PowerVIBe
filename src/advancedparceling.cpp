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

//Helper
void print_ccb (Arrangement_2::Ccb_halfedge_const_circulator circ)
{
	Arrangement_2::Ccb_halfedge_const_circulator curr = circ;
	std::cout << "(" << curr->source()->point() << ")\n";
	do {
		std::cout << "   [" << curr->curve() << "]   "
				  << "(" << curr->target()->point() << ")\n";
	} while (++curr != circ);
	std::cout << std::endl;
}

void print_face (Arrangement_2::Face_const_handle f)
{
	// Print the outer boundary.
	if (f->is_unbounded())
		std::cout << "Unbounded face. " << std::endl;
	else {
		std::cout << "Outer boundary: ";
		print_ccb (f->outer_ccb());
	}

	// Print the boundary of each of the holes.
	Arrangement_2::Hole_const_iterator hi;
	int                                 index = 1;
	for (hi = f->holes_begin(); hi != f->holes_end(); ++hi, ++index) {
		std::cout << "    Hole #" << index << ": ";
		print_ccb (*hi);
	}

	// Print the isolated vertices.
	Arrangement_2::Isolated_vertex_const_iterator iv;
	for (iv = f->isolated_vertices_begin(), index = 1;
		 iv != f->isolated_vertices_end(); ++iv, ++index)
	{
		std::cout << "    Isolated vertex #" << index << ": "
				  << "(" << iv->point() << ")" << std::endl;
	}
}


AdvancedParceling::AdvancedParceling()
{
	this->cityblocks = DM::View("CITYBLOCK", DM::FACE, DM::READ);
	this->cityblocks.getAttribute("selected");
	this->parcels = DM::View("PARCEL", DM::FACE, DM::WRITE);
	this->parcels.addAttribute("selected");
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

	DM::Node center = DM::CGALGeometry::CalculateCentroid(sys, bb);

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

	DM::Node center = DM::CGALGeometry::CalculateCentroid(sys, bb);
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

std::vector<DM::Node *> AdvancedParceling::extractCGALFace(Arrangement_2::Ccb_halfedge_const_circulator hec, DM::SpatialNodeHashMap & sphs)
{
	std::vector<DM::Node *> vp;
	Arrangement_2::Ccb_halfedge_const_circulator curr = hec;
	do{
		float x = CGAL::to_double(curr->source()->point().x());
		float y = CGAL::to_double(curr->source()->point().y());
		DM::Node * n = sphs.addNode(x,y,0,0);
		vp.push_back(n);
	}
	while(++curr != hec );

	return vp;
}

bool AdvancedParceling::checkIfHoleFilling(Arrangement_2::Ccb_halfedge_const_circulator hec)
{
	bool isHole = true;
	Arrangement_2::Ccb_halfedge_const_circulator curr = hec;
	do{
		if(curr->twin()->face()->is_unbounded()) {
			isHole = false;
		}
	}  while(++curr != hec );

	return isHole;
}

void AdvancedParceling::createFinalFaces(DM::System *workingsys, DM::System * sys, DM::View v)
{
	DM::SpatialNodeHashMap sphs(sys,100,false);
	Arrangement_2::Face_const_iterator              fit;
	Segment_list_2									segments;
	Arrangement_2									arr;

	segments = DM::CGALGeometry_P::EdgeToSegment2D(workingsys, v);
	insert (arr, segments.begin(), segments.end());

	int faceCounter = 0;
	for (fit = arr.faces_begin(); fit != arr.faces_end(); ++fit) {
		if (fit->is_unbounded()) { //if unboundI don't want you
			continue;
		}
		if (checkIfHoleFilling(fit->outer_ccb())) // Don't add face if filling of hole
			continue;

		std::vector<DM::Node *> vp = extractCGALFace(fit->outer_ccb(), sphs);
		faceCounter++;

		if (vp.size() < 2)
			continue;
		DM::Face * f = sys->addFace(vp, v);
		f->addAttribute("selected", 1);

		//Extract Holes
		Arrangement_2::Hole_const_iterator hi;
		for (hi = fit->holes_begin(); hi != fit->holes_end(); ++hi) {
			std::vector<DM::Node *> hole = extractCGALFace((*hi), sphs);
			f->addHole(hole);
		}
	}
}




