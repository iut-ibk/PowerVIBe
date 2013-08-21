#include "unittestsdmpowervibe.h"

#include <dm.h>
#include <tbvectordata.h>
#include <cgalgeometry.h>

#include <cgalsearchoperations.h>
#include <advancedparceling.h>

#include <dmlog.h>
#include <dmlogger.h>
#include <dmlogsink.h>
#include <print_utils.h>


DM::Face * addRectangleWithHole(DM::System* sys, DM::View v)
{
	DM::Node * n1 = sys->addNode(DM::Node(1,1,0));
	DM::Node * n2 = sys->addNode(DM::Node(2,1,0));
	DM::Node * n3 = sys->addNode(DM::Node(2,2,0));
	DM::Node * n4 = sys->addNode(DM::Node(1,2,0));

	std::vector<DM::Node * > nodes;
	nodes.push_back(n1);
	nodes.push_back(n2);
	nodes.push_back(n3);
	nodes.push_back(n4);
	nodes.push_back(n1);


	DM::Node * n1_h = sys->addNode(DM::Node(1.25,1.25,0));
	DM::Node * n2_h = sys->addNode(DM::Node(1.75,1.25,0));
	DM::Node * n3_h = sys->addNode(DM::Node(1.75,1.75,0));
	DM::Node * n4_h = sys->addNode(DM::Node(1.25,1.75,0));

	std::vector<DM::Node * > nodes_h;
	nodes_h.push_back(n1_h);
	nodes_h.push_back(n2_h);
	nodes_h.push_back(n3_h);
	nodes_h.push_back(n4_h);

	DM::Face * f1 = sys->addFace(nodes, v);
	f1->addHole(nodes_h);

	return f1;
}

DM::Face * addRectangle(DM::System* sys, DM::View v)
{
	DM::Node * n1 = sys->addNode(DM::Node(0,0,0));
	DM::Node * n2 = sys->addNode(DM::Node(1,0,0));
	DM::Node * n3 = sys->addNode(DM::Node(1,1,0));
	DM::Node * n4 = sys->addNode(DM::Node(0,1,0));

	std::vector<DM::Node * > nodes;
	nodes.push_back(n1);
	nodes.push_back(n2);
	nodes.push_back(n3);
	nodes.push_back(n4);

	DM::Face * f1 = sys->addFace(nodes, v);

	return f1;
}


void addRectangleWithHoleArray(DM::System* sys, DM::View v)
{
	int offset = 0;
	for (int i = 0; i < 3; i++) {

		DM::Node * n1 = sys->addNode(DM::Node(1+(i+offset),1,0));
		DM::Node * n2 = sys->addNode(DM::Node(2+(i+offset),1,0));
		DM::Node * n3 = sys->addNode(DM::Node(2+(i+offset),2,0));
		DM::Node * n4 = sys->addNode(DM::Node(1+(i+offset),2,0));

		std::vector<DM::Node * > nodes;
		nodes.push_back(n1);
		nodes.push_back(n2);
		nodes.push_back(n3);
		nodes.push_back(n4);


		DM::Node * n1_h = sys->addNode(DM::Node(1.25+(i+offset),1.25,0));
		DM::Node * n2_h = sys->addNode(DM::Node(1.75+(i+offset),1.25,0));
		DM::Node * n3_h = sys->addNode(DM::Node(1.75+(i+offset),1.75,0));
		DM::Node * n4_h = sys->addNode(DM::Node(1.25+(i+offset),1.75,0));

		std::vector<DM::Node * > nodes_h;
		nodes_h.push_back(n1_h);
		nodes_h.push_back(n2_h);
		nodes_h.push_back(n3_h);
		nodes_h.push_back(n4_h);

		DM::Face * f1 = sys->addFace(nodes, v);
		f1->addHole(nodes_h);
	}
}

TEST_F(UnitTestsDMPowerVIBe, TestFinalFaces_final_2_holes) {
	ostream *out = &cout;
	DM::Log::init(new DM::OStreamLogSink(*out), DM::Debug);

	DM::System * sys = new DM::System();
	DM::System * result_sys = new DM::System();
	DM::View inputView("In", DM::FACE, DM::WRITE);
	DM::View resultView("Out", DM::FACE, DM::WRITE);
	DM::Face * fr = addRectangle(sys,resultView);

	DM::Node * n1_h = sys->addNode(DM::Node(0.1,0.1,0));
	DM::Node * n2_h = sys->addNode(DM::Node(0.2,0.1,0));
	DM::Node * n3_h = sys->addNode(DM::Node(0.2,0.2,0));
	DM::Node * n4_h = sys->addNode(DM::Node(0.1,0.2,0));

	std::vector<DM::Node * > nodes_h;
	nodes_h.push_back(n1_h);
	nodes_h.push_back(n2_h);
	nodes_h.push_back(n3_h);
	nodes_h.push_back(n4_h);

	fr->addHole(nodes_h);

	DM::Node * n1_h1 = sys->addNode(DM::Node(0.4,0.4,0));
	DM::Node * n2_h1 = sys->addNode(DM::Node(0.5,0.4,0));
	DM::Node * n3_h1 = sys->addNode(DM::Node(0.5,0.5,0));
	DM::Node * n4_h1 = sys->addNode(DM::Node(0.4,0.5,0));

	std::vector<DM::Node * > nodes_h1;
	nodes_h1.push_back(n1_h1);
	nodes_h1.push_back(n2_h1);
	nodes_h1.push_back(n3_h1);
	nodes_h1.push_back(n4_h1);


	fr->addHole(nodes_h1);

	AdvancedParceling parceling;
	parceling.setLength(1);
	parceling.setAspectRatio(1);
	parceling.setOffset(0);

	parceling.setResultView(resultView);
	parceling.setInputView(inputView);

	parceling.createFinalFaces(sys, result_sys,fr, resultView);

	int face_counter = 0;
	int hole_counter = 0;
	double area = 0;
	mforeach (DM::Component * c, result_sys->getAllComponentsInView(resultView)) {
		DM::Face * f = static_cast<DM::Face*>(c);
		DM::Logger(DM::Debug) << "face ";
		foreach (DM::Node * n, f->getNodePointers()) {
			DM::Logger(DM::Debug) << n->getX() << "\t"<< n->getY()<< "\t"<< n->getZ();
		}
		foreach (DM::Face * h, f->getHolePointers()) {
			DM::Logger(DM::Debug) << "hole ";
			foreach (DM::Node * n, h->getNodePointers()) {
				DM::Logger(DM::Debug) << n->getX() << "\t"<< n->getY()<< "\t"<< n->getZ();
			}
			hole_counter++;
		}
		area+=DM::CGALGeometry::CalculateArea2D(f);
		face_counter++;

	}

	DM::Logger(DM::Debug) << face_counter;
	DM::Logger(DM::Debug) << hole_counter;
	DM::Logger(DM::Debug) << area;
	EXPECT_EQ(face_counter,1);
	EXPECT_EQ(hole_counter,2);
	EXPECT_DOUBLE_EQ(area,DM::CGALGeometry::CalculateArea2D(fr));
}


TEST_F(UnitTestsDMPowerVIBe, TestParceling_final_with_holes_at_border) {
	ostream *out = &cout;
	DM::Log::init(new DM::OStreamLogSink(*out), DM::Standard);

	DM::System * sys = new DM::System();
	DM::System * result_sys = new DM::System();
	DM::View inputView("In", DM::FACE, DM::WRITE);
	DM::View resultView("Out", DM::FACE, DM::WRITE);

	DM::Face * fr = addRectangle(sys, DM::View("TEST", DM::FACE, DM::WRITE));

	DM::Node * n1_h = sys->addNode(DM::Node(0.3,0.0,0));
	DM::Node * n2_h = sys->addNode(DM::Node(0.3,0.3,0));
	DM::Node * n3_h = sys->addNode(DM::Node(0.6,0.3,0));
	DM::Node * n4_h = sys->addNode(DM::Node(0.6,0.0,0));


	std::vector<DM::Node * > nodes_h;
	nodes_h.push_back(n1_h);
	nodes_h.push_back(n2_h);
	nodes_h.push_back(n3_h);
	nodes_h.push_back(n4_h);

	fr->addHole(nodes_h);

	std::vector<DM::Face*> faces = DM::CGALGeometry::CleanFace(sys, fr); //Should just return 1 face
	EXPECT_EQ(faces.size(),1);

	foreach (DM::Face * f, faces)
		sys->addComponentToView(f, inputView);

	AdvancedParceling parceling;
	parceling.setLength(0.25);
	parceling.setAspectRatio(1);
	parceling.setOffset(0);

	parceling.setResultView(resultView);
	parceling.setInputView(inputView);

	double area = 0;
	mforeach (DM::Component * c, sys->getAllComponentsInView(inputView)) {
		DM::Face * f = static_cast<DM::Face*>(c);
		parceling.createSubdevision(sys, f, 0);
		parceling.createFinalFaces(sys, result_sys, f, resultView);
		area = DM::CGALGeometry::CalculateArea2D(f);
	}
	int counter = 0;
	double parcel_area = 0;
	mforeach (DM::Component * c, result_sys->getAllComponentsInView(resultView)) {
		DM::Face * f = static_cast<DM::Face*>(c);
		parcel_area+=DM::CGALGeometry::CalculateArea2D(f);
		counter++;
	}
	EXPECT_DOUBLE_EQ(area,parcel_area);
}


TEST_F(UnitTestsDMPowerVIBe, TestParceling_final_with_small_hole) {
	ostream *out = &cout;
	DM::Log::init(new DM::OStreamLogSink(*out), DM::Standard);

	DM::System * sys = new DM::System();
	DM::System * result_sys = new DM::System();
	DM::View inputView("In", DM::FACE, DM::WRITE);
	DM::View resultView("Out", DM::FACE, DM::WRITE);
	DM::Face * fr = addRectangle(sys,inputView);

	DM::Node * n1_h = sys->addNode(DM::Node(0.3,0.1,0));
	DM::Node * n2_h = sys->addNode(DM::Node(0.3,0.3,0));
	DM::Node * n3_h = sys->addNode(DM::Node(0.6,0.3,0));
	DM::Node * n4_h = sys->addNode(DM::Node(0.6,0.1,0));

	std::vector<DM::Node * > nodes_h;
	nodes_h.push_back(n1_h);
	nodes_h.push_back(n2_h);
	nodes_h.push_back(n3_h);
	nodes_h.push_back(n4_h);

	fr->addHole(nodes_h);

	AdvancedParceling parceling;
	parceling.setLength(0.1);
	parceling.setAspectRatio(1);
	parceling.setOffset(0);

	parceling.setResultView(resultView);
	parceling.setInputView(inputView);

	double area = 0;
	mforeach (DM::Component * c, sys->getAllComponentsInView(inputView)) {
		DM::Face * f = static_cast<DM::Face*>(c);
		parceling.createSubdevision(sys, f, 0);
		parceling.createFinalFaces(sys, result_sys, f, resultView);
		area = DM::CGALGeometry::CalculateArea2D(f);
	}
	double parcel_area_input = 0;
	mforeach (DM::Component * c, sys->getAllComponentsInView(resultView)) {
		DM::Face * f = static_cast<DM::Face*>(c);
		DM::Logger(DM::Debug) << DM::CGALGeometry::CalculateArea2D(f);
		parcel_area_input+=DM::CGALGeometry::CalculateArea2D(f);
	}
	EXPECT_DOUBLE_EQ(area,parcel_area_input);

	int counter = 0;
	double parcel_area = 0;
	mforeach (DM::Component * c, result_sys->getAllComponentsInView(resultView)) {
		DM::Face * f = static_cast<DM::Face*>(c);
		DM::Logger(DM::Debug) << DM::CGALGeometry::CalculateArea2D(f);
		parcel_area+=DM::CGALGeometry::CalculateArea2D(f);
		counter++;
	}
	DM::Logger(DM::Standard) << "counter parcels " << counter;
	DM::Logger(DM::Debug) << "area_parcel " << parcel_area;
	EXPECT_DOUBLE_EQ(area,parcel_area);
}

TEST_F(UnitTestsDMPowerVIBe, TestParceling_final_with_holes) {
	ostream *out = &cout;
	DM::Log::init(new DM::OStreamLogSink(*out), DM::Standard);

	DM::System * sys = new DM::System();
	DM::System * result_sys = new DM::System();
	DM::View inputView("In", DM::FACE, DM::WRITE);
	DM::View resultView("Out", DM::FACE, DM::WRITE);
	addRectangleWithHole(sys,inputView);

	AdvancedParceling parceling;
	parceling.setLength(0.25);
	parceling.setAspectRatio(1);
	parceling.setOffset(0);

	parceling.setResultView(resultView);
	parceling.setInputView(inputView);

	double area = 0;
	mforeach (DM::Component * c, sys->getAllComponentsInView(inputView)) {
		DM::Face * f = static_cast<DM::Face*>(c);
		parceling.createSubdevision(sys, f, 0);
		parceling.createFinalFaces(sys, result_sys, f, resultView);
		area = DM::CGALGeometry::CalculateArea2D(f);
	}
	int counter = 0;
	double parcel_area = 0;
	mforeach (DM::Component * c, result_sys->getAllComponentsInView(resultView)) {
		DM::Face * f = static_cast<DM::Face*>(c);
		parcel_area+=DM::CGALGeometry::CalculateArea2D(f);
		counter++;
	}

	DM::Logger(DM::Debug) << "counter " << counter;
	EXPECT_EQ(counter,12);
	DM::Logger(DM::Debug) << "area_parcel " << parcel_area;
	EXPECT_DOUBLE_EQ(area,parcel_area);
}

TEST_F(UnitTestsDMPowerVIBe, TestParceling_with_holes) {
	ostream *out = &cout;
	DM::Log::init(new DM::OStreamLogSink(*out), DM::Standard);

	DM::System * sys = new DM::System();
	DM::View inputView("In", DM::FACE, DM::WRITE);
	DM::View resultView("Out", DM::FACE, DM::WRITE);
	addRectangleWithHole(sys,inputView);

	AdvancedParceling parceling;
	parceling.setLength(0.25);
	parceling.setAspectRatio(1);
	parceling.setOffset(0);

	parceling.setResultView(resultView);
	parceling.setInputView(inputView);

	double area = 0;
	mforeach (DM::Component * c, sys->getAllComponentsInView(inputView)) {
		DM::Face * f = static_cast<DM::Face*>(c);
		parceling.createSubdevision(sys, f, 0);
		area = DM::CGALGeometry::CalculateArea2D(f);
	}
	int counter = 0;
	double parcel_area = 0;
	mforeach (DM::Component * c, sys->getAllComponentsInView(resultView)) {
		DM::Face * f = static_cast<DM::Face*>(c);
		parcel_area+=DM::CGALGeometry::CalculateArea2D(f);
		counter++;
	}

	DM::Logger(DM::Debug) << "counter " << counter;
	EXPECT_EQ(counter,12);
	DM::Logger(DM::Debug) << "area_parcel " << parcel_area;
	EXPECT_DOUBLE_EQ(area,parcel_area);
}


TEST_F(UnitTestsDMPowerVIBe, TestParceling) {
	ostream *out = &cout;
	DM::Log::init(new DM::OStreamLogSink(*out), DM::Standard);

	DM::System * sys = new DM::System();
	DM::System * ReturnSys = new DM::System();

	DM::View inputView("In", DM::FACE, DM::WRITE);
	DM::View resultView("Out", DM::FACE, DM::WRITE);
	addRectangle(sys,inputView);

	AdvancedParceling parceling;
	parceling.setLength(0.25);
	parceling.setAspectRatio(1);
	parceling.setOffset(0);

	parceling.setResultView(resultView);
	parceling.setInputView(inputView);

	double area = 0;
	mforeach (DM::Component * c, sys->getAllComponentsInView(inputView)) {
		DM::Face * f = static_cast<DM::Face*>(c);
		parceling.createSubdevision(sys, f, 0);
		area = DM::CGALGeometry::CalculateArea2D(f);
		parceling.createFinalFaces(sys, ReturnSys, f, resultView);
	}
	int counter = 0;
	double parcel_area = 0;
	mforeach (DM::Component * c, sys->getAllComponentsInView(resultView)) {
		DM::Face * f = static_cast<DM::Face*>(c);
		parcel_area+=DM::CGALGeometry::CalculateArea2D(f);
		counter++;
	}

	DM::Logger(DM::Debug) << "counter " << counter;
	EXPECT_EQ(counter,16);
	DM::Logger(DM::Debug) << "area_parcel " << parcel_area;
	EXPECT_DOUBLE_EQ(area,parcel_area);

	int face_counter = 0;
	int hole_counter = 0;
	double area_end = 0;
	mforeach (DM::Component * c, ReturnSys->getAllComponentsInView(resultView)) {
		DM::Face * f = static_cast<DM::Face*>(c);
		foreach (DM::Node * n, f->getNodePointers()) {
			DM::Logger(DM::Debug) << n->getX() << "\t"<< n->getY()<< "\t"<< n->getZ();
		}
		foreach (DM::Face * h, f->getHolePointers()) {
			DM::Logger(DM::Debug) << "hole ";
			foreach (DM::Node * n, h->getNodePointers()) {
				DM::Logger(DM::Debug) << n->getX() << "\t"<< n->getY()<< "\t"<< n->getZ();
			}
			hole_counter++;
		}
		area_end+=DM::CGALGeometry::CalculateArea2D(f);
		face_counter++;
		DM::Logger(DM::Debug ) << "Area: " << DM::CGALGeometry::CalculateArea2D(f);

	}
	DM::Logger(DM::Debug) << face_counter;
	DM::Logger(DM::Debug) << hole_counter;
	DM::Logger(DM::Debug) << area;

	EXPECT_DOUBLE_EQ(area_end,area);

}

TEST_F(UnitTestsDMPowerVIBe, AP_Holes_createFinalFaces_array) {
	ostream *out = &cout;
	DM::Log::init(new DM::OStreamLogSink(*out), DM::Standard);

	DM::System * sys = new DM::System();
	DM::System * ReturnSys = new DM::System();

	DM::View v("UNIT_TEST", DM::FACE, DM::READ);
	addRectangleWithHoleArray(sys,v);

	AdvancedParceling parcling;
	double x1 = 0;
	double y1 = 0;
	double x2 = 0;
	double y2 = 0;

	TBVectorData::GetViewExtend(sys, v, x1, y1, x2, y2);

	DM::Node * n1 = sys->addNode(x1, y1, 0);
	DM::Node * n2 = sys->addNode(x1, y2, 0);
	DM::Node * n3 = sys->addNode(x2, y2, 0);
	DM::Node * n4 = sys->addNode(x2, y1, 0);

	std::vector<DM::Node *> vF;
	vF.push_back(n1);
	vF.push_back(n2);
	vF.push_back(n3);
	vF.push_back(n4);

	DM::Face * bF = sys->addFace(vF, DM::View("BB", DM::FACE));
	mforeach (DM::Component * c, sys->getAllComponentsInView(v)) {
		DM::Face * f = static_cast<DM::Face*>(c);
		foreach(DM::Face * h, f->getHolePointers()) {
			bF->addHole(h);
		}
	}

	parcling.createFinalFaces(sys, ReturnSys,bF, v);


	int face_counter = 0;
	int hole_counter = 0;
	double area = 0;
	mforeach (DM::Component * c, ReturnSys->getAllComponentsInView(v)) {
		DM::Face * f = static_cast<DM::Face*>(c);
		DM::Logger(DM::Debug) << "face ";
		foreach (DM::Node * n, f->getNodePointers()) {
			DM::Logger(DM::Debug) << n->getX() << "\t"<< n->getY()<< "\t"<< n->getZ();
		}
		foreach (DM::Face * h, f->getHolePointers()) {
			DM::Logger(DM::Debug) << "hole ";
			foreach (DM::Node * n, h->getNodePointers()) {
				DM::Logger(DM::Debug) << n->getX() << "\t"<< n->getY()<< "\t"<< n->getZ();
			}
			DM::Logger(DM::Debug ) << "Hole: " << DM::CGALGeometry::CalculateArea2D(h);
			hole_counter++;
		}
		area+=DM::CGALGeometry::CalculateArea2D(f);
		face_counter++;
		DM::Logger(DM::Debug ) << "Area: " << DM::CGALGeometry::CalculateArea2D(f);

	}
	DM::Logger(DM::Debug) << face_counter;
	DM::Logger(DM::Debug) << hole_counter;
	DM::Logger(DM::Debug) << area;
	EXPECT_EQ(face_counter,3);
	EXPECT_EQ(hole_counter,3);
	EXPECT_DOUBLE_EQ(area,2.25);
}

TEST_F(UnitTestsDMPowerVIBe, TestFinalFaces_final_small_hole) {
	ostream *out = &cout;
	DM::Log::init(new DM::OStreamLogSink(*out), DM::Standard);

	DM::System * sys = new DM::System();
	DM::System * result_sys = new DM::System();
	DM::View inputView("In", DM::FACE, DM::WRITE);
	DM::View resultView("Out", DM::FACE, DM::WRITE);
	DM::Face * fr = addRectangle(sys,resultView);

	DM::Node * n1_h = sys->addNode(DM::Node(0.3,0.1,0));
	DM::Node * n2_h = sys->addNode(DM::Node(0.3,0.3,0));
	DM::Node * n3_h = sys->addNode(DM::Node(0.6,0.3,0));
	DM::Node * n4_h = sys->addNode(DM::Node(0.6,0.1,0));

	std::vector<DM::Node * > nodes_h;
	nodes_h.push_back(n1_h);
	nodes_h.push_back(n2_h);
	nodes_h.push_back(n3_h);
	nodes_h.push_back(n4_h);

	fr->addHole(nodes_h);

	AdvancedParceling parceling;
	parceling.setLength(1);
	parceling.setAspectRatio(1);
	parceling.setOffset(0);

	parceling.setResultView(resultView);
	parceling.setInputView(inputView);

	parceling.createFinalFaces(sys, result_sys,fr, resultView);

	int face_counter = 0;
	int hole_counter = 0;
	double area = 0;
	mforeach (DM::Component * c, result_sys->getAllComponentsInView(resultView)) {
		DM::Face * f = static_cast<DM::Face*>(c);
		DM::Logger(DM::Debug) << "face ";
		foreach (DM::Node * n, f->getNodePointers()) {
			DM::Logger(DM::Debug) << n->getX() << "\t"<< n->getY()<< "\t"<< n->getZ();
		}
		foreach (DM::Face * h, f->getHolePointers()) {
			DM::Logger(DM::Debug) << "hole ";
			foreach (DM::Node * n, h->getNodePointers()) {
				DM::Logger(DM::Debug) << n->getX() << "\t"<< n->getY()<< "\t"<< n->getZ();
			}
			hole_counter++;
		}
		area+=DM::CGALGeometry::CalculateArea2D(f);
		face_counter++;

	}

	DM::Logger(DM::Debug) << face_counter;
	DM::Logger(DM::Debug) << hole_counter;
	DM::Logger(DM::Debug) << area;
	EXPECT_EQ(face_counter,1);
	EXPECT_EQ(hole_counter,1);
	EXPECT_DOUBLE_EQ(area,DM::CGALGeometry::CalculateArea2D(fr));
}

TEST_F(UnitTestsDMPowerVIBe, AP_Holes_createFinalFaces) {
	ostream *out = &cout;
	DM::Log::init(new DM::OStreamLogSink(*out), DM::Standard);

	DM::System * sys = new DM::System();
	DM::System * ReturnSys = new DM::System();

	DM::View v("UNIT_TEST", DM::FACE, DM::READ);
	DM::Face * boundary = addRectangleWithHole(sys,v);

	AdvancedParceling parceling;
	parceling.createFinalFaces(sys, ReturnSys,boundary, v);

	int face_counter = 0;
	int hole_counter = 0;
	double area = 0;
	mforeach (DM::Component * c, ReturnSys->getAllComponentsInView(v)) {
		DM::Face * f = static_cast<DM::Face*>(c);
		DM::Logger(DM::Debug) << "face ";
		foreach (DM::Node * n, f->getNodePointers()) {
			DM::Logger(DM::Debug) << n->getX() << "\t"<< n->getY()<< "\t"<< n->getZ();
		}
		foreach (DM::Face * h, f->getHolePointers()) {
			DM::Logger(DM::Debug) << "hole ";
			foreach (DM::Node * n, h->getNodePointers()) {
				DM::Logger(DM::Debug) << n->getX() << "\t"<< n->getY()<< "\t"<< n->getZ();
			}
			hole_counter++;
		}
		area+=DM::CGALGeometry::CalculateArea2D(f);
		face_counter++;

	}
	DM::Logger(DM::Debug) << face_counter;
	DM::Logger(DM::Debug) << hole_counter;
	DM::Logger(DM::Debug) << area;
	EXPECT_EQ(face_counter,1);
	EXPECT_EQ(hole_counter,1);
	EXPECT_DOUBLE_EQ(area,0.75);
}

