#include "createsinglefamilyhouses.h"
#include "dmgeometry.h"

DM_DECLARE_NODE_NAME(CreateSingleFamilyHouses, BlockCity)

CreateSingleFamilyHouses::CreateSingleFamilyHouses()
{
    houses = DM::View("BUILDING", DM::FACE, DM::WRITE);

    parcels = DM::View("PARCEL", DM::FACE, DM::READ);

    parcels.addLinks("BUILDING", houses);
    houses.addLinks("PARCEL", parcels);

    std::vector<DM::View> data;
    data.push_back(houses);
    data.push_back(parcels);
    this->addData("City", data);
}

void CreateSingleFamilyHouses::run()
{
    DM::System * city = this->getData("City");
    DM::SpatialNodeHashMap spatialNodeMap(city, 100);

    std::vector<std::string> parcelUUIDs = city->getUUIDs(parcels);
    foreach (std::string parcelUUID, parcelUUIDs) {
        DM::Face * pracel = city->getFace(parcelUUID);



    }
}
