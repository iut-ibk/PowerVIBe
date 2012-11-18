#ifndef ADVANCEDPARCELING_H
#define ADVANCEDPARCELING_H

#include <dm.h>

class DM_HELPER_DLL_EXPORT AdvancedParceling : public DM::Module
{
    DM_DECLARE_NODE(AdvancedParceling)
private:
    DM::View parcels;
    DM::View cityblocks;

    double aspectRatio;
    double length;

public:
    AdvancedParceling();
    void run();

    void createSubdevision(DM::System * sys,  DM::Face * f, int gen);
    void finalSubdevision(DM::System * sys, DM::Face * f, int gen);
};

#endif // ADVANCEDPARCELING_H
