#include <fstream>
#include <iostream>

#include "otsdaq-components/FEInterfaces/FEOtsUDPTemplateInterface.h"
#include "otsdaq/ConfigurationInterface/ConfigurationInterface.h"
#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq/TableCore/TableGroupKey.h"
/*

#include "otsdaq-components/FEInterfaces/FEWOtsUDPFSSRInterface.h"
#include "otsdaq-components/FEInterfaces/FEWOtsUDPHCALInterface.h"
#include "otsdaq/ConfigurationDataFormats/FEWOtsUDPHardwareTable.h"
*/
using namespace ots;

int main(int argc, char** argv)
{
	// Variables
	const int          supervisorInstance_    = 1;
	const unsigned int configurationKeyValue_ = 1;

	ConfigurationManager* theConfigurationManager_ = new ConfigurationManager();

	std::string XDAQContextConfigurationName_ = "XDAQContextTable";
	std::string supervisorContextUID_         = "mainContext";
	std::string supervisorApplicationUID_     = "FESupervisor";
	std::string interfaceUID_                 = "ExampleInterface0";
	std::string supervisorConfigurationPath_ =
	    "/" + supervisorContextUID_ + "/LinkToApplicationConfiguration/" +
	    supervisorApplicationUID_ + "/LinkToSupervisorConfiguration";
	const ConfigurationTree theXDAQContextConfigTree_ =
	    theConfigurationManager_->getNode(XDAQContextConfigurationName_);

	std::string                           configurationGroupName = "defaultConfig";
	std::pair<std::string, TableGroupKey> theGroup(configurationGroupName,
	                                               TableGroupKey(configurationKeyValue_));

	theConfigurationManager_->loadTableGroup(theGroup.first, theGroup.second, true);

	FEOtsUDPTemplateInterface* theInterface_ = new FEOtsUDPTemplateInterface(
	    interfaceUID_,
	    theXDAQContextConfigTree_,
	    supervisorConfigurationPath_ + "/LinkToFEInterfaceTable/" + interfaceUID_ +
	        "/LinkToFETypeConfiguration");
	std::cout << "Done with new" << std::endl;
	// Test interface class methods here //
	theInterface_->configure();
	theInterface_->start(std::string(argv[1]));
	unsigned int second  = 1000;  // x1ms
	unsigned int time    = 60 * 60 * second;
	unsigned int counter = 0;
	while(counter++ < time)
	{
		theInterface_->running();  // There is a 1ms sleep inside the running
		                           // std::cout << counter << std::endl;
	}
	theInterface_->stop();

	return 0;
}
