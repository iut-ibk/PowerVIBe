#ifndef REGIONALTEMPERATUR_H
#define REGIONALTEMPERATUR_H

#include <dmmodule.h>

class DM_HELPER_DLL_EXPORT RegionalTemperatur : public DM::Module
{
    DM_DECLARE_NODE(RegionalTemperatur)
private:
        DM::View ThermalRegion;
public:

    RegionalTemperatur();
    void run();
};

#endif // REGIONALTEMPERATUR_H
