#ifndef CREATESHADOWS_H
#define CREATESHADOWS_H

#include <dm.h>

class DM_HELPER_DLL_EXPORT CreateShadows : public DM::Module
{
    DM_DECLARE_NODE(CreateShadows)
private:
        DM::View buildings;
        DM::View models;
        DM::View sunrays;
        DM::View sunnodes;
        void transformCooridnates(double &x, double &y);
        DM::Node directionSun( double dAzimuth, double dZenithAngle);

        void testdirectionSun();

        std::vector<DM::Node> createRaster(DM::System * sys, DM::Face * f);




public:
    CreateShadows();
    void run();


};

#endif // CREATESHADOWS_H
