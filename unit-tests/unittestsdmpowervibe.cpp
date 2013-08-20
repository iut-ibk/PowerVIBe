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


void addRectangleWithHole(DM::System* sys, DM::View v)
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
	nodes_h.push_back(n1_h);

	DM::Face * f1 = sys->addFace(nodes, v);
	f1->addHole(nodes_h);
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



TEST_F(UnitTestsDMPowerVIBe, AP_Holes_createFinalFaces_array) {
	ostream *out = &cout;
	DM::Log::init(new DM::OStreamLogSink(*out), DM::Debug);

	DM::System * sys = new DM::System();
	DM::System * ReturnSys = new DM::System();

	DM::View v("UNIT_TEST", DM::FACE, DM::READ);
	addRectangleWithHoleArray(sys,v);

	AdvancedParceling parcling;
	parcling.createFinalFaces(sys, ReturnSys, v);

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

TEST_F(UnitTestsDMPowerVIBe, AP_Holes_createFinalFaces) {
	ostream *out = &cout;
	DM::Log::init(new DM::OStreamLogSink(*out), DM::Standard);

	DM::System * sys = new DM::System();
	DM::System * ReturnSys = new DM::System();

	DM::View v("UNIT_TEST", DM::FACE, DM::READ);
	addRectangleWithHole(sys,v);

	AdvancedParceling parcling;
	parcling.createFinalFaces(sys, ReturnSys, v);

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

