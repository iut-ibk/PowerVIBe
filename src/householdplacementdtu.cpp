#include "householdplacementdtu.h"

DM_DECLARE_NODE_NAME(HouseholdPlacementDTU, UrbanSim)
HouseholdPlacementDTU::HouseholdPlacementDTU()
{
    address = DM::View("ADDRESS", DM::NODE, DM::READ);
    address.getAttribute("BYG_BEBOLE");

    buildings = DM::View("BUILDING", DM::FACE, DM::READ);
    buildings.getAttribute("residential_units");
    buildings.getAttribute("ADDRESS");

    households = DM::View("HOUSEHOLD",  DM::COMPONENT ,DM::WRITE);
    households.addAttribute("id");
    households.addAttribute("income");
    households.addAttribute("age_of_head");
    households.addAttribute("race_id");
    households.addAttribute("children");
    households.addAttribute("cars");




    households.addLinks("BUILDING", buildings);
    buildings.addLinks("HOUSEHOLD", households);


    persons = DM::View("PERSON", DM::COMPONENT ,DM::WRITE);
    persons.addAttribute("id");
    persons.addLinks("HOUSEHOLD", households);
    households.addLinks("PERSON", persons);

    vCity = DM::View("CITY", DM::FACE, DM::READ);
    vCity.addAttribute("HH01");
    vCity.addAttribute("HH02");
    vCity.addAttribute("HH03");
    vCity.addAttribute("HH04");
    vCity.addAttribute("HH05");




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
    std::vector<std::string> building_ids = city->getUUIDs(buildings);
    int household_id = 1;
    int person_id = 1;

    double households_tot[5];


    foreach (std::string building_id, building_ids) {
        DM::Component * building = city->getComponent(building_id);
        std::vector<LinkAttribute> address_links = building->getAttribute("ADDRESS")->getLinks();

        int persons_tot = 0;
        int residential_units = (int) building->getAttribute("residential_units")->getDouble();

        foreach (LinkAttribute lattr, address_links) {
            DM::Component * addr = city->getComponent(lattr.uuid);
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
            linkB.setLink(buildings.getName(), building->getUUID());
            h->addAttribute(linkB);

            //Link Building Household
            Attribute * bh = building->getAttribute("HOUSEHOLD");
            bh->setLink(households.getName(), h->getUUID());

            //Create Households and Persons
            for(int j = 0; j < per; j++) {
                Component * p = new Component();
                p->addAttribute("id", person_id++);

                //Link Person - Houshold
                Attribute l("HOUSEHOLD");
                l.setLink(households.getName(), h->getUUID());
                p->addAttribute(l);

                //Link Household - Person
                Attribute * hp = h->getAttribute("PERSON");
                hp->setLink("PERSON", p->getUUID());
                city->addComponent(p, persons);
            }

            city->addComponent(h, households);
            households_tot[per-1] = households_tot[per-1]+1;
        }
    }
    Logger(Debug) << "Created Households " << household_id;
    Logger(Debug) << "Created Persons " << person_id;

    DM::Component * c = city->getComponent(city->getUUIDs(vCity)[0]);

    c->addAttribute("HH01", households_tot[0]);
    c->addAttribute("HH02", households_tot[1]);
    c->addAttribute("HH03", households_tot[2]);
    c->addAttribute("HH04", households_tot[3]);
    c->addAttribute("HH05", households_tot[4]);
}

