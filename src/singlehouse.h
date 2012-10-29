#ifndef SINGLEHOUSE_H
#define SINGLEHOUSE_H

#include <dm.h>

using namespace DM;

class DM_HELPER_DLL_EXPORT SingleHouse : public Module
{
    DM_DECLARE_NODE(SingleHouse)
private:
        DM::View houses;
        DM::View footprint;
        DM::View building_model;
public:
    SingleHouse();
    void run();
};

#endif // SINGLEHOUSE_H
