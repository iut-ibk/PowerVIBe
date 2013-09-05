#ifndef PLACEJOBSDTU_H
#define PLACEJOBSDTU_H

#include <dm.h>


class DM_HELPER_DLL_EXPORT PlaceJobsDTU : public DM::Module
{
	DM_DECLARE_NODE(PlaceJobsDTU)
	private:
		DM::View jobs;
	DM::View buildings;

	DM::View households;
	DM::View persons;
	DM::View vCity;
	double sqft_per_job;
public:
	PlaceJobsDTU();
	void run();
};

#endif // PLACEJOBSDTU_H
