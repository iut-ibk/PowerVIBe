#include <dmmoduleregistry.h>
#include <dmnodefactory.h>
#include <placehouseholds.h>

using namespace DM;

extern "C" void DM_HELPER_DLL_EXPORT  registerModules(ModuleRegistry *registry) {
    registry->addNodeFactory(new NodeFactory<PlaceHouseholds>());
}
