#include "distancefield.h"
#include <CGAL/Simple_cartesian.h>
#include <CGAL/point_generators_2.h>
#include <CGAL/Orthogonal_k_neighbor_search.h>
#include <CGAL/Search_traits_2.h>
#include <cgalgeometry.h>
#include <list>
#include <cmath>
#include <tbvectordata.h>
#ifdef _OPENMP
#include <omp.h>
#endif
DM_DECLARE_NODE_NAME(DistanceField, SpatialOperations)
DistanceField::DistanceField()
{
	centerView = "";
	faceView = "";
	attributeName = "";

	this->addParameter("CenterView", DM::STRING, &centerView);
	this->addParameter("FaceView", DM::STRING, &faceView);
	this->addParameter("AttributeView", DM::STRING, &attributeName);

	//Add Dummy datastream
	std::vector<DM::View> datastream;
	datastream.push_back(DM::View("dummy", DM::SUBSYSTEM, DM::MODIFY));

	this->addData("sys", datastream);


}

void DistanceField::init() {
	if (centerView.empty())
		return;
	if (faceView.empty())
		return;
	if (attributeName.empty())
		return;

	std::vector<DM::View> datastream;


	inV = DM::View(centerView, DM::NODE, DM::READ);
	outV = DM::View(faceView, DM::FACE, DM::READ);
	outV.addAttribute(attributeName);

	datastream.push_back(inV);
	datastream.push_back(outV);

	this->addData("sys", datastream);

}

void DistanceField::run() {

	typedef CGAL::Simple_cartesian<double> K;
	typedef K::Point_2 Point_d;
	typedef CGAL::Search_traits_2<K> TreeTraits;
	typedef CGAL::Orthogonal_k_neighbor_search<TreeTraits> Neighbor_search;
	typedef Neighbor_search::Tree Tree;

	const unsigned int N = 1;

	std::list<Point_d> points;

	if (centerView.empty())
		return;
	if (faceView.empty())
		return;
	if (attributeName.empty())
		return;

	//CreateSearchTree
	DM::System * sys = this->getData("sys");
	std::vector<std::string> uuids = sys->getUUIDs(inV);
	foreach (std::string uuid, uuids) {
		DM::Node * n = sys->getNode(uuid);
		points.push_back(Point_d(n->getX(), n->getY()));
	}

	Tree tree(points.begin(), points.end());

	uuids = sys->getUUIDs(outV);
	//
	int NumberOfFaces = uuids.size();
	//#pragma omp parallel for
	for (int i = 0; i < NumberOfFaces; i++) {
		DM::Face * f = sys->getFace(uuids[i]);
		DM::Node cn = DM::CGALGeometry::CalculateCentroid(sys, f);

		Point_d query(cn.getX(),cn.getY());
		double l = -1;
		Neighbor_search search(tree, query, N);
		for(Neighbor_search::iterator it = search.begin(); it != search.end(); ++it){
			l = sqrt((it->second));
		}
		f->addAttribute(attributeName, l);


	}

}
