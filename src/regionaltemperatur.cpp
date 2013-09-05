#include "regionaltemperatur.h"
#include <dm.h>
#include <tbvectordata.h>
#include <math.h>

DM_DECLARE_NODE_NAME(RegionalTemperatur, PowerVIBe)

RegionalTemperatur::RegionalTemperatur()
{
	this->ThermalRegion = DM::View("ThermalRegion", DM::FACE, DM::READ);
	this->ThermalRegion.getAttribute("geothermal_energy");
	this->ThermalRegion.addAttribute("delta_T");
	this->ThermalRegion.getAttribute("kf");
	this->ThermalRegion.getAttribute("n");
	this->ThermalRegion.getAttribute("I_ground_water");
	this->ThermalRegion.getAttribute("width");

	std::vector<DM::View> datastream;
	datastream.push_back(this->ThermalRegion);


	this->addData("city", datastream);


}

void RegionalTemperatur::run() {
	DM::System * city = this->getData("city");

	std::vector<std::string> uuids = city->getUUIDs(this->ThermalRegion);

	foreach (std::string uuid, uuids) {
		DM::Face * f = city->getFace(uuid);
		double w0 = f->getAttribute("geothermal_energy")->getDouble() / (365.*24.);
		double area = TBVectorData::CalculateArea(TBVectorData::getNodeListFromFace(city, f));
		double lambda = 1.0;
		double t_first = 5;
		double t_second = 25;

		double kf = f->getAttribute("kf")->getDouble();
		double I = f->getAttribute("I_ground_water")->getDouble();
		double n = f->getAttribute("n")->getDouble();
		double w = f->getAttribute("width")->getDouble();
		double V = kf*t_second*w*n*I;


		//double w1 = lambda * area * 1/(t_first+t_second/4);
		double w1 = lambda * area * 1/(t_first);
		double w2 = V * 1 * 4190000;
		double deltaT = w0/(w1+w2);

		/*while (fabs(1.-(w1+w2)/w0) > 0.01) {
			if (w1 > w0)
				deltaT -= 0.001;
			else
				deltaT += 0.001;
			w1 = lambda * area * deltaT/(t_first+t_second/4);
			w2 = V * deltaT * 4190000;


			counter ++;
			if (counter > 10000)
				break;
		}*/

		DM::Logger(DM::Debug) << w0 << " " << w1 << " " << w2 <<" "<< deltaT;

		f->addAttribute("deltaT", deltaT);

	}


}
