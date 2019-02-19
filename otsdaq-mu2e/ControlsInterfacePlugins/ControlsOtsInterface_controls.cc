#include "otsdaq-core/Macros/ControlsPluginMacros.h"
#include "otsdaq-demo/ControlsInterfacePlugins/ControlsOtsInterface.h"

using namespace ots;

ControlsOtsInterface::ControlsOtsInterface(
    const std::string&       interfaceUID,
    const ConfigurationTree& theXDAQContextConfigTree,
    const std::string&       controlsConfigurationPath)
    //  :Socket            (
    //  theXDAQContextConfigTree.getNode(interfaceConfigurationPath).getNode("HostIPAddress").getValue<std::string>()
    // ,theXDAQContextConfigTree.getNode(interfaceConfigurationPath).getNode("HostPort").getValue<unsigned
    // int>())
    // ,
    : ControlsVInterface(
          interfaceUID, theXDAQContextConfigTree, controlsConfigurationPath)
{
}

ControlsOtsInterface::~ControlsOtsInterface() { destroy(); }

void ControlsOtsInterface::initialize() {}

void ControlsOtsInterface::destroy() {}

std::string ControlsOtsInterface::getList(std::string format)
{
	//__COUT__ <<
	// theXDAQContextConfigTree.getNode(controlsConfigurationPath).getValue <<
	// std::endl;
	return (std::string) "list";
}
void ControlsOtsInterface::subscribe(std::string Name) {}

void ControlsOtsInterface::subscribeJSON(std::string List) {}

void ControlsOtsInterface::unsubscribe(std::string Name) {}

std::array<std::string, 4> ControlsOtsInterface::getCurrentValue(std::string Name)
{
	return {"a", "b", "c", "d"};
}

std::array<std::string, 9> ControlsOtsInterface::getSettings(std::string Name)
{
	return {"a", "b", "c", "d", "e", "f", "g", "h", "i"};
}

DEFINE_OTS_CONTROLS(ControlsOtsInterface)
