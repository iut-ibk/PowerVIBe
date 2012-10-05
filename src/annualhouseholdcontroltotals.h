#ifndef ANNUALHOUSEHOLDCONTROLTOTALS_H
#define ANNUALHOUSEHOLDCONTROLTOTALS_H

#include <dmmodule.h>
#include <dm.h>

using namespace DM;
class DM_HELPER_DLL_EXPORT AnnualHouseholdControlTotals : public Module
{
    DM_DECLARE_NODE(AnnualHouseholdControlTotals)
    private:
            DM::View city;
            double growthRate;
            int startYear;
            int endYear;
    public:
        AnnualHouseholdControlTotals();
    void run();

};

#endif // ANNUALHOUSEHOLDCONTROLTOTALS_H
