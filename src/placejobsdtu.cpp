#include "placejobsdtu.h"

DM_DECLARE_NODE_NAME(PlaceJobsDTU, UrbanSim)
PlaceJobsDTU::PlaceJobsDTU()
{

	sqft_per_job = 75;
	this->addParameter("sqft_per_job", DM::DOUBLE, &sqft_per_job);
	buildings = DM::View("BUILDING",  DM::FACE, DM::READ);
	buildings.getAttribute("non_residential_sqft");

	households = DM::View("HOUSEHOLD", DM::COMPONENT, DM::READ);
	households.addAttribute("workers");
	households.getAttribute("PERSON");

	persons = DM::View("PERSON", DM::COMPONENT, DM::READ);
	persons.getAttribute("id");
	persons.addAttribute("job_id");

	jobs = DM::View("JOB",  DM::COMPONENT ,DM::WRITE);
	jobs.addAttribute("id");
	jobs.addAttribute("home_based_status");
	jobs.addAttribute("sector_id");
	jobs.addAttribute("building_type");


	jobs.addLinks("BUILDING", buildings);
	buildings.addLinks("JOB", jobs);

	vCity = DM::View("CITY", DM::FACE, DM::READ);
	vCity.addAttribute("JOBS");


	std::vector<DM::View> dataset;

	dataset.push_back(buildings);
	dataset.push_back(jobs);
	dataset.push_back(households);
	dataset.push_back(persons);
	dataset.push_back(vCity);

	this->addData("City", dataset);
}

void PlaceJobsDTU::run()
{

	DM::System * city = this->getData("City");
	std::vector<std::string> buildingUUIDs = city->getUUIDs(buildings);


	std::vector<std::string> PersonsUUIDs = city->getUUIDs(persons);
	std::vector<int> person_rand_int_id;
	for (unsigned int i = 0; i < PersonsUUIDs.size(); i++) {
		person_rand_int_id.push_back(i);
	}
	for (unsigned int i = 0; i < PersonsUUIDs.size(); i++) {
		int index1 =  rand() % person_rand_int_id.size();
		int index2 =  rand() % person_rand_int_id.size();
		int p_id_tmp = person_rand_int_id[index1];
		person_rand_int_id[index1] = person_rand_int_id[index2];
		person_rand_int_id[index2] = p_id_tmp;
	}

	int job_id = 0;
	foreach (std::string uuid, buildingUUIDs) {
		DM::Component * building = city->getComponent(uuid);
		double non_residentail_sqft = building->getAttribute("non_residential_sqft")->getDouble();
		int jobsInBuilding = (int) non_residentail_sqft / (sqft_per_job*1.05);
		for (int i = 0; i < jobsInBuilding; i++) {
			job_id++;
			if (job_id > PersonsUUIDs.size()-1) {
				continue;
			}
			DM::Component * job = new DM::Component();

			//Person ID
			DM::Component * person = city->getComponent(PersonsUUIDs[person_rand_int_id[job_id]]);
			person->addAttribute("job_id", job_id);

			DM::Component * households = city->getComponent(person->getAttribute("HOUSEHOLD")->getLink().uuid);
			households->addAttribute("workers", households->getAttribute("workers")->getDouble()+1);
			job->addAttribute("id", job_id);
			job->addAttribute("home_based_status", 0);
			job->addAttribute("sector_id", 1);
			job->addAttribute("building_type", 1);

			DM::Attribute lAttr("BUILDING");
			lAttr.setLink("BUILDING", building->getUUID());
			job->addAttribute(lAttr);

			//link job to building
			city->addComponent(job, jobs);
			DM::Attribute * lBAttr = building->getAttribute("JOB");
			lBAttr->setLink("JOB",job->getUUID());


		}
	}

	DM::Component * c = city->getComponent(city->getUUIDs(vCity)[0]);

	c->addAttribute("JOBS",job_id);


}


