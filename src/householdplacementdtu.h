#ifndef HOUSEHOLDPLACEMENTDTU_H
#define HOUSEHOLDPLACEMENTDTU_H

#include <dm.h>

using namespace DM;

class DM_HELPER_DLL_EXPORT HouseholdPlacementDTU : public Module
{
    DM_DECLARE_NODE(HouseholdPlacementDTU)
private:
    DM::View address;
    DM::View buildings;
    DM::View households;
    DM::View persons;
    DM::View vCity;

public:

    HouseholdPlacementDTU();
    void run();
};

#endif // HOUSEHOLDPLACEMENTDTU_H
