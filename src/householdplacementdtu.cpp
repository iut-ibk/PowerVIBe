#include "householdplacementdtu.h"

DM_DECLARE_NODE_NAME(HouseholdPlacementDTU, UrbanSim)
HouseholdPlacementDTU::HouseholdPlacementDTU()
{
	address = DM::View("ADDRESS", DM::NODE, DM::READ);
	address.addAttribute("BYG_BEBOLE", DM::Attribute::DOUBLE, DM::READ);

	buildings = DM::View("BUILDING", DM::FACE, DM::READ);
	buildings.addAttribute("residential_units", DM::Attribute::DOUBLE, DM::READ);
	buildings.addAttribute("ADDRESS", "ADDRESS", DM::READ);

	households = DM::View("HOUSEHOLD",  DM::COMPONENT ,DM::WRITE);
	households.addAttribute("id", DM::Attribute::DOUBLE, DM::WRITE);
	households.addAttribute("income", DM::Attribute::DOUBLE, DM::WRITE);
	households.addAttribute("age_of_head", DM::Attribute::DOUBLE, DM::WRITE);
	households.addAttribute("race_id", DM::Attribute::DOUBLE, DM::WRITE);
	households.addAttribute("children", DM::Attribute::DOUBLE, DM::WRITE);
	households.addAttribute("cars", DM::Attribute::DOUBLE, DM::WRITE);

	households.addAttribute("BUILDING", buildings.getName(), DM::WRITE);
	buildings.addAttribute("HOUSEHOLD", households.getName(), DM::WRITE);


	persons = DM::View("PERSON", DM::COMPONENT ,DM::WRITE);
	persons.addAttribute("id", DM::Attribute::DOUBLE, DM::WRITE);
	persons.addAttribute("HOUSEHOLD", households.getName(), DM::WRITE);
	households.addAttribute("PERSON", persons.getName(), DM::WRITE);

	vCity = DM::View("CITY", DM::FACE, DM::READ);
	vCity.addAttribute("HH01", DM::Attribute::DOUBLE, DM::WRITE);
	vCity.addAttribute("HH02", DM::Attribute::DOUBLE, DM::WRITE);
	vCity.addAttribute("HH03", DM::Attribute::DOUBLE, DM::WRITE);
	vCity.addAttribute("HH04", DM::Attribute::DOUBLE, DM::WRITE);
	vCity.addAttribute("HH05", DM::Attribute::DOUBLE, DM::WRITE);

	std::vector<DM::View> data;
	data.push_back(address);
	data.push_back(households);
	data.push_back(buildings);
	data.push_back(persons);
	data.push_back(vCity);
	this->addData("City", data);
}

void HouseholdPlacementDTU::run()
{

	DM::System * city = this->getData("City");
	int household_id = 1;
	int person_id = 1;

	double households_tot[5];

	foreach(DM::Component* building, city->getAllComponentsInView(buildings))
	{
		int persons_tot = 0;
		int residential_units = (int) building->getAttribute("residential_units")->getDouble();

		foreach(DM::Component* addr, building->getAttribute("ADDRESS")->getLinkedComponents())
		{
			int p = (int)addr->getAttribute("BYG_BEBOLE")->getDouble();
			persons_tot+=p;
		}

		for (int i = 0; i <  residential_units; i++) {
			if (persons_tot < 1) {
				continue;
			}
			int per = rand() % 5+1;
			if (i == residential_units-1) {
				if (persons_tot > 5)     {
					per = 5;
				}
				else {
					per = persons_tot;
				}
			}

			if (persons_tot < per)
				per = persons_tot;

			persons_tot = persons_tot - per;

			//Create Household
			Component * h = new Component();
			h->addAttribute("id", household_id++);
			h->addAttribute("income", 1000);
			h->addAttribute("age_of_head", 50);
			h->addAttribute("race_id", 1);
			int children = 0;
			if (per+1 > 2)
				children = 1;
			h->addAttribute("children", children);
			h->addAttribute("cars", children);

			//Link Household - Building
			Attribute linkB("BUILDING");
			linkB.addLink(building, buildings.getName());
			h->addAttribute(linkB);

			//Link Building Household
			Attribute * bh = building->getAttribute("HOUSEHOLD");
			bh->addLink(h, households.getName());

			//Create Households and Persons
			for(int j = 0; j < per; j++) {
				Component * p = new Component();
				p->addAttribute("id", person_id++);

				//Link Person - Houshold
				Attribute l("HOUSEHOLD");
				l.addLink(h, households.getName());
				p->addAttribute(l);

				//Link Household - Person
				Attribute * hp = h->getAttribute("PERSON");
				hp->addLink(p, "PERSON");
				city->addComponent(p, persons);
			}

			city->addComponent(h, households);
			households_tot[per-1] = households_tot[per-1]+1;
		}
	}
	Logger(Debug) << "Created Households " << household_id;
	Logger(Debug) << "Created Persons " << person_id;

	DM::Component * c = city->getAllComponentsInView(vCity)[0];

	c->addAttribute("HH01", households_tot[0]);
	c->addAttribute("HH02", households_tot[1]);
	c->addAttribute("HH03", households_tot[2]);
	c->addAttribute("HH04", households_tot[3]);
	c->addAttribute("HH05", households_tot[4]);
}

