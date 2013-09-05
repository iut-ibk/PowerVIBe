#ifndef DISTANCEFIELD_H
#define DISTANCEFIELD_H

#include <dmmodule.h>
#include <dm.h>

class DM_HELPER_DLL_EXPORT DistanceField : public DM::Module
{
	DM_DECLARE_NODE( DistanceField)
	private:
		std::string centerView;
	std::string faceView;
	std::string attributeName;
	DM::View inV;
	DM::View outV;
public:
	DistanceField();
	void run();
	void init();
};

#endif // DISTANCEFIELD_H
