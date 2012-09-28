#ifndef PLACEHOUSEHOLDS_H
#define PLACEHOUSEHOLDS_H

#include <dmmodule.h>
#include <dm.h>

using namespace DM;

class DM_HELPER_DLL_EXPORT PlaceHouseholds : public Module
{
    DM_DECLARE_NODE(PlaceHouseholds)
    private:
            DM::View grids;
            DM::View households;
            DM::View buildings;
            DM::View persons;
            int sumHouseholds(int (&hh)[5] );
            int chooseHousehold( int (&hh)[5] );
            int chooseBuilding();
    public:
        PlaceHouseholds();
        void run();
};

#endif // PLACEHOUSEHOLDS_H
