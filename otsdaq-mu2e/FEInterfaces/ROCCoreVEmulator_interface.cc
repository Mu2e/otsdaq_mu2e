#include "otsdaq-mu2e/FEInterfaces/ROCCoreVEmulator.h"

#include "otsdaq-core/Macros/InterfacePluginMacros.h"

using namespace ots;

#undef 	__MF_SUBJECT__
#define __MF_SUBJECT__ "FE-ROCCoreVEmulator"

//=========================================================================================
ROCCoreVEmulator::ROCCoreVEmulator(
			   const std::string& rocUID,
			   const ConfigurationTree& theXDAQContextConfigTree,
			   const std::string& theConfigurationPath)
: ROCCoreInterface			(rocUID,theXDAQContextConfigTree,theConfigurationPath)
, workloopExit_				(false)
, workloopRunning_			(false)
{
  INIT_MF("ROCCoreVEmulator");
}

//==========================================================================================
ROCCoreVEmulator::~ROCCoreVEmulator(void)
{
	while(workloopRunning_)
	{
		__COUT__ << "Attempting to exit thread..." << __E__;
		workloopExit_ = true;
		sleep(1);
	}

	__COUT__ << "Workloop thread is not running." << __E__;
}

DEFINE_OTS_INTERFACE(ROCCoreVEmulator)
