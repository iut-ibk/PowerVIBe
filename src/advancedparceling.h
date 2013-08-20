#ifndef ADVANCEDPARCELING_H
#define ADVANCEDPARCELING_H

#include <dm.h>
#include <dmgeometry.h>
#include <cgalgeometry_p.h>

class DM_HELPER_DLL_EXPORT AdvancedParceling : public DM::Module
{
    DM_DECLARE_NODE(AdvancedParceling)
private:
    DM::View parcels;
    DM::View cityblocks;
    DM::View bbs;

    double aspectRatio;
    double length;
    double offset;

    bool remove_new;
    std::string InputViewName;
    std::string OutputViewName;



public:
    AdvancedParceling();
    void run();
    void init();
    void createSubdevision(DM::System * sys,  DM::Face * f, int gen);
    void finalSubdevision(DM::System * sys, DM::Face * f, int gen);

	/** @brief creates final parceling and identify edges, transfers results from working sys to sys */
	void createFinalFaces(DM::System * workingsys, DM::System *sys, DM::View v);

	/** @brief extract faces and returns vector of face nodes*/
	std::vector<DM::Node *> extractCGALFace(Arrangement_2::Ccb_halfedge_const_circulator hec, DM::SpatialNodeHashMap & sphs);

	/** @brief returns if face is filling of a assumed hole. I assume it is a hole when non of its edges is part of the boundary */
	bool checkIfHoleFilling(Arrangement_2::Ccb_halfedge_const_circulator hec);
};

#endif // ADVANCEDPARCELING_H
