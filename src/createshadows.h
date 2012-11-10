#ifndef CREATESHADOWS_H
#define CREATESHADOWS_H

#include <dm.h>
#include <QDate>

struct cSunCoordinates;

class DM_HELPER_DLL_EXPORT CreateShadows : public DM::Module
{
    DM_DECLARE_NODE(CreateShadows)
private:
        DM::View buildings;
        DM::View models;
        DM::View sunrays;
        DM::View sunnodes;

        bool createRays;
        bool createDayly;
        bool createHourly;
        bool onlyWindows;

        void transformCooridnates(double &x, double &y);
        void directionSun(DM::Node * n, double dAzimuth, double dZenithAngle);


        std::vector<DM::Node> createRaster(DM::System * sys, DM::Face * f);


        /** @brief calculate hourly sun pos for the simulation period */
        void caclulateSunPositions(const QDate &start, const QDate &end, std::vector<DM::Node*> & sunPos , std::vector<cSunCoordinates *> & sunLoc);



public:
    CreateShadows();
    void run();


};

#endif // CREATESHADOWS_H