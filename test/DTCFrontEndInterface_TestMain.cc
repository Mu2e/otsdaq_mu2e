// #include <otsdaq_demo/otsdaq-demo/FEInterfaces/FEWROtsUDPFSSRInterface.h>
// #include
//<otsdaq_demo/otsdaq-demo/UserConfigurationDataFormats/FEWROtsUDPFSSRInterfaceConfiguration.h>
#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq/FECore/FEVInterfacesManager.h"
// #include "otsdaq/TableDataFormats/TableGroupKey.h"

// #include "otsdaq-demo/FEInterfaces/FEWOtsGenericInterface.h"
#include "otsdaq/FECore/FEVInterface.h"

#include <iostream>
#include <memory>

#include "otsdaq/ConfigurationInterface/ConfigurationInterface.h"
#include "otsdaq/FECore/MakeInterface.h"
#include "otsdaq/TableCore/MakeTable.h"

// #include "otsdaq-components/FEInterfaces/FEWOtsUDPFSSRInterface.h"
#include "otsdaq-mu2e/FEInterfaces/DTCFrontEndInterface.h"

using namespace ots;

int main(int argc, char* argv[])
{
   __COUT__ << "DTCFrontEndInterface Test main()";


	//==============================================================================
	// Define environment variables
	//	Note: normally these environment variables are set by StartOTS.sh

	// These are needed by
	// otsdaq/otsdaq/ConfigurationDataFormats/ConfigurationInfoReader.cc [207]
	setenv("CONFIGURATION_TYPE", "File", 1);  // Can be File, Database, DatabaseTest
	setenv("CONFIGURATION_DATA_PATH", (std::string(getenv("USER_DATA")) + "/ConfigurationDataExamples").c_str(), 1);
	setenv("TABLE_INFO_PATH", (std::string(getenv("USER_DATA")) + "/TableInfo").c_str(), 1);
	////////////////////////////////////////////////////


	// Some configuration plug-ins use __ENV__("OTSDAQ_LIB") and
	// __ENV__("OTSDAQ_UTILITIES_LIB") in init() so define it 	to a non-sense place is ok
	setenv("OTSDAQ_LIB", (std::string(getenv("USER_DATA")) + "/").c_str(), 1);
	setenv("OTSDAQ_UTILITIES_LIB", (std::string(getenv("USER_DATA")) + "/").c_str(), 1);

	// Some configuration plug-ins use __ENV__("OTS_MAIN_PORT") in init() so define it
	setenv("OTS_MAIN_PORT", "2015", 1);

	// also xdaq envs for XDAQContextTable
	setenv("XDAQ_CONFIGURATION_DATA_PATH", (std::string(getenv("USER_DATA")) + "/XDAQConfigurations").c_str(), 1);
	setenv("XDAQ_CONFIGURATION_XML", "otsConfigurationNoRU_CMake", 1);
	////////////////////////////////////////////////////


	// // Variables
	std::string supervisorContextUID_           = "ContextCalo03";
	std::string supervisorApplicationUID_       = "FESupervisorCalo03";
	std::string feUID_       					= "DTC4";
	std::string theConfigurationPath_ = supervisorContextUID_ + "/LinkToApplicationTable/" + supervisorApplicationUID_ + 
        "/LinkToSupervisorTable/LinkToFEInterfaceTable/" + feUID_ + "/LinkToFETypeTable";


    ConfigurationManager cfgMgr;

	//need to activate configure group
	cfgMgr.restoreActiveTableGroups(
		true, //bool                                throwErrors /*=false*/,
		"", //const std::string&                  pathToActiveGroupsFile /*=""*/,
		ConfigurationManager::LoadGroupType::ALL_TYPES //ConfigurationManager::LoadGroupType onlyLoadIfBackboneOrContext /*= ConfigurationManager::LoadGroupType::ALL_TYPES */,
		//std::string*                        accumulatedWarnings /*=0*/)
	);
	

	// std::string name = cfgMgr.getNode(ConfigurationManager::XDAQ_CONTEXT_TABLE_NAME).getBackNode(theConfigurationPath_)
	// 	    .getNode("FEInterfacePluginName")
	// 	    .getValue<std::string>();
	// __COUTV__(name);

    DTCFrontEndInterface dtc(feUID_,
        cfgMgr.getNode(ConfigurationManager::XDAQ_CONTEXT_TABLE_NAME),
        theConfigurationPath_);


	__COUTV__(argc);
	for(int i=0;i<argc;++i)
	{
		__COUT__ << "arg[" << i << "] = " << argv[i] << __E__;
	}
	if(argc < 2)
	{
		__SS__ << "Need at least 1 argument: DTCFrontEndInterface_TestMain.cc <numberOfEventWindowMarkers>" << __E__;
		__SS_THROW__;
	}

    dtc.getCFOandDTCRegisters()->ReadDesignDate();

	uint32_t numberOfEventWindowMarkers = atoi(argv[1]);
    
    dtc.SetCFOEmulatorFixedWidthEmulation(
        1, //bool enable, 
        1, //bool useDetachedBufferTest,
        "0x44 clocks", //std::string eventDuration, 
        numberOfEventWindowMarkers, //uint32_t numberOfEventWindowMarkers, 
        0, //uint64_t initialEventWindowTag,
        1, //uint64_t eventWindowMode, 
        0, //bool enableClockMarkers, 
        1, //bool enableAutogenDRP, 
        0, //bool saveBinaryDataToFile,
        0, //bool saveSubeventHeadersToDataFile,
        0//	bool doNotResetCounters )
    );
	int i=0;
	while(1) 
	{
		sleep(1);
		if(i%10 == 9)
			__COUT_INFO__ << "\n" << 
				DTCFrontEndInterface::getDetachedBufferTestStatus(dtc.bufferTestThreadStruct_) << __E__;	

		if(DTCFrontEndInterface::getDetachedBufferTestReceivedCount(dtc.bufferTestThreadStruct_) >= numberOfEventWindowMarkers - 1)// start mutex scope
		{
			__COUT_INFO__ << "\n" << 
				DTCFrontEndInterface::getDetachedBufferTestStatus(dtc.bufferTestThreadStruct_) << __E__;	

			sleep(1);
			std::lock_guard<std::mutex> lock(dtc.bufferTestThreadStruct_->lock_);
			dtc.bufferTestThreadStruct_->exitThread_ = true;
			break;
		}
		++i;
	} //end main loop
	sleep(1);
	__COUT_INFO__ << "Thread and main exited!" << __E__;
} //end main()
