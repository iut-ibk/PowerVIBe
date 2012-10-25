#ifndef CREATESHADOWS_H
#define CREATESHADOWS_H

#include <dm.h>

class DM_HELPER_DLL_EXPORT CreateShadows : public DM::Module
{
    DM_DECLARE_NODE(CreateShadows)
private:
        DM::Node normalVector(std::vector<DM::Node*> nodes);
        double angelBetweenVectors(const DM::Node &n1, const DM::Node &n2);
        DM::Node directionSun( double dAzimuth, double dZenithAngle);

        void testdirectionSun();
public:
    CreateShadows();
    void run();


};

#endif // CREATESHADOWS_H
