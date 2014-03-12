#include "advancedparceling.h"
#include "tbvectordata.h"
#include "cgalgeometry.h"
#include "dmgeometry.h"
#include "QPolygonF"
#include "QTransform"
#include <cgalgeometry_p.h>
#include <tbvectordata.h>
#include <cgalgeometry.h>

DM_DECLARE_NODE_NAME(AdvancedParceling, BlockCity)

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



double AdvancedParceling::getLength() const
{
	return length;
}

void AdvancedParceling::setLength(double value)
{
	length = value;
}

double AdvancedParceling::getOffset() const
{
	return offset;
}

void AdvancedParceling::setOffset(double value)
{
	offset = value;
}

double AdvancedParceling::getAspectRatio() const
{
	return aspectRatio;
}

void AdvancedParceling::setAspectRatio(double value)
{
	aspectRatio = value;
}

DM::View AdvancedParceling::getInputView() const
{
	return inputView;
}

void AdvancedParceling::setInputView(const DM::View &value)
{
	inputView = value;
}

DM::View AdvancedParceling::getResultView() const
{
	return resultView;
}

void AdvancedParceling::setResultView(const DM::View &value)
{
	resultView = value;
}

bool AdvancedParceling::getCombined_edges() const
{
	return combined_edges;
}

void AdvancedParceling::setCombined_edges(bool value)
{
	combined_edges = value;
}
AdvancedParceling::AdvancedParceling()
{

	//Parameter Definition
	aspectRatio = 2;
	this->addParameter("AspectRatio", DM::DOUBLE, &aspectRatio);

	length = 100;
	this->addParameter("Length", DM::DOUBLE, &length);

	offset = 1;
	this->addParameter("offset", DM::DOUBLE, & offset);

	remove_new = false;
	this->addParameter("remove_new", DM::BOOL, & remove_new);

	InputViewName = "SUPERBLOCK";
	this->addParameter("INPUTVIEW", DM::STRING, &InputViewName);

	OutputViewName = "CITYBLOCK";
	this->addParameter("OUTPUTVIEW", DM::STRING, &OutputViewName);

	tol = 0.00001; //should not be to small
	this->addParameter("tolerance", DM::DOUBLE, &tol);

	debug = false;
	this->addParameter("debug", DM::BOOL, &debug);

	combined_edges = false;
	this->addParameter("combined_edges", DM::BOOL, &combined_edges);

	splitShortSide = false;
	this->addParameter("splitShortSide", DM::BOOL, &splitShortSide);

	//Datastream
	this->inputView = DM::View("CITYBLOCK", DM::FACE, DM::READ);
	this->inputView.addAttribute("selected", DM::Attribute::DOUBLE, DM::READ);

	this->resultView = DM::View("PARCEL", DM::FACE, DM::WRITE);
	this->resultView.addAttribute("selected", DM::Attribute::DOUBLE, DM::WRITE);

	this->resultView.addAttribute("generation", DM::Attribute::DOUBLE, DM::WRITE);

	this->bbs = DM::View("BBS", DM::FACE, DM::WRITE);

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
	inputView = DM::View(InputView.getName(), InputView.getType(), DM::READ);
	this->inputView.addAttribute("selected", DM::Attribute::DOUBLE, DM::READ);
	resultView = DM::View(OutputViewName, DM::FACE, DM::WRITE);
	this->resultView.addAttribute("selected", DM::Attribute::DOUBLE, DM::WRITE);
	this->resultView.addAttribute("generation", DM::Attribute::DOUBLE, DM::WRITE);
	face_nodes = DM::View("FACE_NODES", DM::NODE, DM::WRITE);
	face_nodes.addAttribute("street_side", DM::Attribute::DOUBLE, DM::WRITE);

	std::vector<DM::View> datastream;

	datastream.push_back(inputView);
	datastream.push_back(resultView);
	datastream.push_back(face_nodes);
	if (debug)
		datastream.push_back(DM::View("faces_debug", DM::FACE, DM::WRITE));

	this->addData("city", datastream);
}

/** The method is based on the minial bounding box */
void AdvancedParceling::run(){

	DM::Logger(DM::Warning) << "Redevelopment not finished yet - offset is still missing";
	if (this->aspectRatio < 1) {
		DM::Logger(DM::Warning) <<  "Aspect Ration < 1 please, just values > 1 are used";
	}

	DM::System * city = this->getData("city");

	DM::SpatialNodeHashMap sphs(city,100,false);

	foreach (DM::Component * c, city->getAllComponentsInView(this->inputView)) {
		DM::System workingSys;
		DM::Face * f = static_cast<DM::Face *> (c);
		if (f->getAttribute("selected")->getDouble() < 0.01) {
			DM::Logger(DM::Debug) << "Continue";
			continue;
		}
		DM::Logger(DM::Debug) << "start parceling";
		DM::Face * fnew = TBVectorData::CopyFaceGeometryToNewSystem(f, &workingSys);
		workingSys.addComponentToView(fnew, this->inputView);
		this->createSubdevision(&workingSys, fnew, 0);
		createFinalFaces(&workingSys, city, fnew, this->resultView, sphs);
		DM::Logger(DM::Debug) << "end parceling";
	}

	if (!remove_new)
		return;

	foreach (DM::Component * c, city->getAllComponentsInView(this->inputView)) {
		DM::Face * f = static_cast<DM::Face *> (c);
		f->addAttribute("selected", 0);
	}

}

void AdvancedParceling::createSubdevision(DM::System * sys,  DM::Face *f, int gen)
{
	bool split_length = false;
	std::vector<DM::Node> box;
	std::vector<double> size;

	double alpha = DM::CGALGeometry::CalculateMinBoundingBox(TBVectorData::getNodeListFromFace(sys, f), box, size);


	DM::Face * bb;
	std::vector<DM::Node*> l_bb;
	foreach (DM::Node  n, box) {
		l_bb.push_back(sys->addNode(n));
	}

	bb = sys->addFace(l_bb);

	DM::Node center = DM::CGALGeometry::CalculateCentroid(sys, bb);

	double x_c = center.getX();
	double y_c = center.getY();

	double split_l = size[0];
	double split_width = size[1];

	if (2*this->length > split_l) {
		if ( (this->length / this->aspectRatio) * 2 >   split_width) { //width
			sys->addComponentToView(f, this->resultView);
			return;
		}
	}

	if (this->splitShortSide) {
		 split_l = size[1];
		 split_width = size[0];
	}
	if (2*this->length > split_l)
		split_length = true;
	int elements = 2;
	if (split_length) {
		if(!this->splitShortSide)
			elements = size[1] / (this->length / this->aspectRatio);
		else
			elements = size[0] / (this->length / this->aspectRatio);
	}
	for (int i = 0; i < elements; i++) {
		double l = size[0];
		double w = size[1];
		QRectF r1;
		//if (!this->splitShortSide)
			r1 = QRectF(-l/2.+ i*l/(double)elements,  -w/2-10, l/(double)elements,w+10);
		if (this->splitShortSide && !split_length)
			r1 = QRectF(-l/2.-10,  -w/2. + i*w/(double)elements, l + 10,w/(double)elements);

		//Rotate Intersection Rect
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

std::vector<DM::Node *> AdvancedParceling::extractCGALFace(Arrangement_2::Ccb_halfedge_const_circulator hec, DM::SpatialNodeHashMap & sphs)
{
	std::vector<DM::Node *> vp;
	Arrangement_2::Ccb_halfedge_const_circulator curr = hec;
	do{
		double x = CGAL::to_double(curr->source()->point().x());
		double y = CGAL::to_double(curr->source()->point().y());
		DM::Node * n = sphs.addNode(x,y,0,tol, this->face_nodes);
		if (curr->twin()->face()->is_unbounded())
			n->addAttribute("street_side", 1);
		vp.push_back(n);

	}
	while(++curr != hec );

	return vp;
}

bool AdvancedParceling::checkIfHoleFilling(DM::Face * orig, DM::Face * face_new)
{
	// Create Small offest and check if node is in hole or node to identify if this is just a filling of a hole
	for (double i = -1; i < 2; i++) {
		for (double j = -1; j < 2; j++) {
			if (i == 0 || j == 0)
				continue;
			DM::Node * en = face_new->getNodePointers()[0];
			DM::Node n(en->getX() + tol*10.* i, en->getY() + tol*10. * i, en->getZ() + tol*10.* i);
			if (debug){
				DM::Logger(DM::Debug)<< "Number of holes" << orig->getHolePointers().size();
				DM::Logger(DM::Debug) << "Face new" << en->getX() + 1 * i << " " << en->getY() + 1;
				TBVectorData::PrintFace(face_new, DM::Debug);
			}
			DM::Logger(DM::Debug) << "------";
			if (DM::CGALGeometry::NodeWithinFace(face_new, n)) { //Valid Point
				if (DM::CGALGeometry::NodeWithinFace(orig, n)) { //if outside the origin face this means there is a hole
					return false;
				} else {
					return true;
				}
			}
		}
	}
	return false;
}

void AdvancedParceling::createFinalFaces(DM::System *workingsys, DM::System * sys, DM::Face * orig, DM::View v, DM::SpatialNodeHashMap &sphs)
{

	Arrangement_2::Face_const_iterator              fit;
	Segment_list_2									segments;
	Arrangement_2									arr;
	if (tol != 0)
		segments = DM::CGALGeometry_P::Snap_Rounding_2D(workingsys, v,tol);
	else
		segments = DM::CGALGeometry_P::EdgeToSegment2D(workingsys, v);
	insert (arr, segments.begin(), segments.end());

	int faceCounter_orig = 0;

	if (debug) {
		foreach (DM::Component * cmp, workingsys->getAllComponentsInView(v)) {
			DM::Face * f = dynamic_cast<DM::Face*>(cmp);
			DM::Face * f1 = TBVectorData::CopyFaceGeometryToNewSystem(f, sys);
			sys->addComponentToView(f1, DM::View("faces_debug", DM::FACE, DM::WRITE));
			faceCounter_orig++;
		}
	}
	if (!combined_edges){
		foreach (DM::Component * cmp, workingsys->getAllComponentsInView(v)) {
			DM::Face * f = dynamic_cast<DM::Face*>(cmp);
			DM::Face * f1 = TBVectorData::CopyFaceGeometryToNewSystem(f, sys);
			sys->addComponentToView(f1,v);
			f1->addAttribute("selected", 1);
		}
		return;
	}

	int faceCounter = 0;
	int removed_faces = 0;
	for (fit = arr.faces_begin(); fit != arr.faces_end(); ++fit) {
		if (fit->is_unbounded()) { //if unbound I don't want you
			continue;
		}

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


		if (checkIfHoleFilling(orig, f)) {
			DM::Logger(DM::Debug) << "Remove face";
			sys->removeComponentFromView(f, v);
			sys->removeChild(f);
			removed_faces++;
		}

	}

	DM::Logger(DM::Debug) << "Faces in Arrangment" << arr.number_of_faces();
	DM::Logger(DM::Debug) << "Input Faces " << faceCounter_orig;
	DM::Logger(DM::Debug) << "Created Faces " << faceCounter;
	DM::Logger(DM::Debug) << "Removed Faces " << removed_faces;
}




