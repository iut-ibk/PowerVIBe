#ifndef CREATESINGLEFAMILYHOUSES_H
#define CREATESINGLEFAMILYHOUSES_H

#include <dm.h>

using namespace DM;
class CreateSingleFamilyHouses : public Module
{
    DM_DECLARE_NODE(CreateSingleFamilyHouses);

private:
    DM::View houses;
    DM::View parcels;
    DM::View building_model;
    DM::View footprint;
    int stories;
    void createWindows(DM::Face * f, double distance, double width, double height);
public:
    CreateSingleFamilyHouses();
    void run();

};

#endif // CREATESINGLEFAMILYHOUSES_H
