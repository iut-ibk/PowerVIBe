#include "placehouseholds.h"

DM_DECLARE_NODE_NAME(PlaceHouseholds, PowerVIBe)

int PlaceHouseholds::sumHouseholds(int (&hh)[5])
{
    int sum = 0;
    for(int i = 0; i < 5; i++)
        sum+=hh[i];
    return sum;
}

int PlaceHouseholds::chooseHousehold(int (&hh)[5])
{
    std::vector<int> vChoose;
    for (int i = 0; i < 5; i++) {
        if (hh > 0)
            vChoose.push_back(i);
    }

    int chooser= rand() % vChoose.size();

    return vChoose[chooser];
}

PlaceHouseholds::PlaceHouseholds()
{
    grids = DM::View("GRID", DM::FACE, DM::READ);
    grids.getAttribute("HH01");
    grids.getAttribute("HH02");
    grids.getAttribute("HH03");
    grids.getAttribute("HH04");
    grids.getAttribute("HH05");
    grids.getAttribute("BUILDING");

    buildings = DM::View("BUILDING",  DM::FACE, DM::READ);
    buildings.getAttribute("residential_units");


    households = DM::View("HOUSEHOLD",  DM::COMPONENT ,DM::WRITE);
    households.addAttribute("id");
    households.addLinks("BUILDING", buildings);

    persons = DM::View("Persons", DM::COMPONENT ,DM::WRITE);
    persons.addAttribute("id");
    persons.addLinks("HOUSEHOLD", households);


    std::vector<DM::View> dataset;
    dataset.push_back(grids);
    dataset.push_back(buildings);
    dataset.push_back(households);
    dataset.push_back(persons);

    this->addData("City", dataset);
}

void PlaceHouseholds::run()
{
    DM::System * city = this->getData("City");

    std::vector<std::string> gridUuids = city->getUUIDsOfComponentsInView(grids);    


    foreach (std::string gUuid, gridUuids) {
          DM::Component * grid = city->getComponent(gUuid);

          int hh[5];
          hh[0] = (int) grid->getAttribute("HH01")->getDouble();
          hh[1] = (int) grid->getAttribute("HH02")->getDouble();
          hh[2] = (int) grid->getAttribute("HH03")->getDouble();
          hh[3] = (int) grid->getAttribute("HH04")->getDouble();
          hh[4] = (int) grid->getAttribute("HH05")->getDouble();

          //Get Buildings
          Attribute  * attrBuildings = grid->getAttribute("BUILDING");
          std::vector<LinkAttribute> lBuildings = attrBuildings->getLinks();
          int numberOfBuildings = lBuildings.size();

          std::vector<int> avalibleUnits;
          foreach (LinkAttribute lB, lBuildings) {
              //int avalible units
              //avalibleUnits
          }

          while (this->sumHouseholds(hh) > 0) {
              //int chooser = this->chooseHousehold(hh);






          }


    }


}

