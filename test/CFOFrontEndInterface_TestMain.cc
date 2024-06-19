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
#include "otsdaq-mu2e/FEInterfaces/CFOFrontEndInterface.h"

using namespace ots;

int main(int argc, char* argv[])
try
{
   	__COUT_INFO__ << "CFOFrontEndInterface Test main()";

	__COUTV__(argc);
	for(int i=0;i<argc;++i)
	{
		__COUT_INFO__ << "arg[" << i << "] = " << argv[i] << __E__;
	}
	if(argc != 4)
	{
		__COUT_ERR__ << "\n\n\tUsage = Need 3 arguments: CFOFrontEndInterface_TestMain <deviceIndex> <numberOfEventWindowMarkers> <runPlanMode>\n\n" << __E__;		
		return 0;
	}

	uint32_t deviceIndex = atoi(argv[1]);
	uint32_t numberOfEventWindowMarkers = atoi(argv[2]);
	uint32_t runPlanMode = atoi(argv[3]); //0 for fixed width, 1 for super cycles

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
	std::string feUID_       					= deviceIndex==0? "DTC4" : "DTC5"; //DTC4 for Device0 and DTC5 for Device1
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

    CFOFrontEndInterface cfo(feUID_,
        cfgMgr.getNode(ConfigurationManager::XDAQ_CONTEXT_TABLE_NAME),
        theConfigurationPath_);


	if(numberOfEventWindowMarkers == uint32_t(-1))
	{
		__COUT_INFO__ << "Setting up CFO for RTF, Reset and Buffer Release!" << __E__;

		cfo.getCFOandDTCRegisters()->SetJitterAttenuatorSelect(1 /* select RJ45 */, false /* alsoResetJA */);
		sleep(1);
    	__COUT_INFO__ << "JA Status = " << cfo.getCFOandDTCRegisters()->FormatJitterAttenuatorCSR() << __E__;
		
		cfo.thisCFO_->CFOandDTC_Registers::ResetSERDES();
		cfo.thisCFO_->ResetSERDES(CFOLib::CFO_Link_ID::CFO_Link_ALL);
		
		cfo.thisCFO_->SoftReset();
		cfo.thisCFO_->ReleaseAllBuffers(CFO_DMA_Engine_DAQ);

		__COUT_INFO__ << "Reset and ReleaseAllBuffers called!" << __E__;
		return 0;
	}
	if(numberOfEventWindowMarkers == uint32_t(-2))
	{
		__COUT_INFO__ << "Attempting Buffer Release ONLY!" << __E__;
		cfo.thisCFO_->ReleaseAllBuffers(CFO_DMA_Engine_DAQ);
		__COUT_INFO__ << "ReleaseAllBuffers called!" << __E__;
		return 0;
	}
	if(numberOfEventWindowMarkers == uint32_t(-3))
	{
		__COUT_INFO__ << "Setting to 32KB Max DMA Transfer size!" << __E__;
		cfo.thisCFO_->SetTriggerDMATransferLength(0x8000);
		__COUT_INFO__ << "CFO DMA sizes = " << cfo.thisCFO_->FormatDMATransferLength() << __E__;
		return 0;
	}
	if(numberOfEventWindowMarkers == uint32_t(-4))
	{
		__COUT_INFO__ << "Setting to 64KB Max DMA Transfer size!" << __E__;
		cfo.thisCFO_->SetTriggerDMATransferLength(0xFFF8);
		__COUT_INFO__ << "CFO DMA sizes = " << cfo.thisCFO_->FormatDMATransferLength() << __E__;
		return 0;
	}
	if(numberOfEventWindowMarkers == uint32_t(-5))
	{
		__COUT_INFO__ << "Resetting SESRDES!" << __E__;
		cfo.thisCFO_->CFOandDTC_Registers::ResetSERDES();
		__COUT_INFO__ << "Done resetting SESRDES!" << __E__;
		sleep(1);
		__COUT_INFO__ << "CFO links CDR: \n" << cfo.thisCFO_->FormatSERDESRXCDRLock() << __E__;
		return 0;
	}
	if(numberOfEventWindowMarkers == uint32_t(-6))
	{
		__COUT_INFO__ << "Resetting SESRDES RX!" << __E__;
		cfo.thisCFO_->ResetSERDES(CFOLib::CFO_Link_ID::CFO_Link_ALL);
		__COUT_INFO__ << "Done resetting SESRDES RX!" << __E__;
		sleep(1);
		__COUT_INFO__ << "CFO links CDR: \n" << cfo.thisCFO_->FormatSERDESRXCDRLock() << __E__;
		return 0;
	}
	if(numberOfEventWindowMarkers == uint32_t(-7))
	{
		__COUT_INFO__ << "Disabling and re-enabling links!" << __E__;
		cfo.thisCFO_->DisableLink(CFOLib::CFO_Link_ID::CFO_Link_ALL);
		sleep(1);
		cfo.thisCFO_->EnableLink(CFOLib::CFO_Link_ID::CFO_Link_ALL);
		__COUT_INFO__ << "Done disabling and re-enabling links!" << __E__;
		sleep(1);
		__COUT_INFO__ << "CFO links CDR: \n" << cfo.thisCFO_->FormatSERDESRXCDRLock() << __E__;
		return 0;
	}
	

    __COUT_INFO__ << "CFO version = " << cfo.thisCFO_->ReadDesignDate() << __E__;
	__COUT_INFO__ << "CFO DMA sizes = " << cfo.thisCFO_->FormatDMATransferLength() << __E__;


	cfo.getCFOandDTCRegisters()->SetJitterAttenuatorSelect(1 /* select RJ45 */, false /* alsoResetJA */);
	for(int i=0;i<10;++i) //wait for JA to lock before reading
	{
		if(cfo.getCFOandDTCRegisters()->ReadJitterAttenuatorLocked())
			break;
		sleep(1);
	}
    __COUT_INFO__ << "JA Status = " << cfo.getCFOandDTCRegisters()->FormatJitterAttenuatorCSR() << __E__;

	__COUT_INFO__ << "runPlanMode = " << runPlanMode << __E__;
	if(runPlanMode == 0)
		cfo.CompileSetAndLaunchTemplateFixedWidthRunPlan(
			1, //bool enable, 
			1, //bool useDetachedBufferTest,
			"0x44 clocks", //std::string eventDuration, 
			numberOfEventWindowMarkers, //uint32_t numberOfEventWindowMarkers, 
			100, //uint64_t initialEventWindowTag,
			0x333, //uint64_t eventWindowMode, 
			0, //bool enableClockMarkers, 
			0, //bool saveBinaryDataToFile,
			0, //bool saveSubeventHeadersToDataFile,
			0  //bool doNotResetCounters )
		);
	else
		cfo.CompileSetAndLaunchTemplateSuperCycleRunPlan(
			1, //bool enable,
			1, //bool useDetachedBufferTest, 
			numberOfEventWindowMarkers, //uint32_t numberOfSuperCycles, 
			100, //uint64_t initialEventWindowTag,
			0, //bool enableClockMarkers, 
			0, //bool saveBinaryDataToFile,
			0, //bool saveSubeventHeadersToDataFile,
			0  //bool doNotResetCounters )
		); numberOfEventWindowMarkers *= 245000; //set event cout expectation for check below (235K on-spill + 10K off-spill per cycle)

	int i=0;
	bool dumpSpy = false;
	while(1) 
	{
		sleep(1);
		std::cout << '.' << std::flush;
		if(i%10 == 9)
		{
			std::cout << "time(0) = " << time(0) << '\n' << std::flush;
			__COUT_INFO__ << "\n" << 
				CFOFrontEndInterface::getDetachedBufferTestStatus(cfo.bufferTestThreadStruct_) << __E__;	
		}

		if((i > 5 && !cfo.bufferTestThreadStruct_->running_) || 
			CFOFrontEndInterface::getDetachedBufferTestReceivedCount(cfo.bufferTestThreadStruct_) >= numberOfEventWindowMarkers - 1)// start mutex scope
		{
			__COUT_INFO__ << "Iteration exit #" << i << " - thread running = " << cfo.bufferTestThreadStruct_->running_ << "\n" << 
				CFOFrontEndInterface::getDetachedBufferTestStatus(cfo.bufferTestThreadStruct_) << __E__;	

			sleep(1);
			std::lock_guard<std::mutex> lock(cfo.bufferTestThreadStruct_->lock_);
			cfo.bufferTestThreadStruct_->exitThread_ = true;

			if(CFOFrontEndInterface::getDetachedBufferTestReceivedCount(cfo.bufferTestThreadStruct_) < numberOfEventWindowMarkers - 1)
				dumpSpy = true;			
			break;
		}
		++i;
	} //end main loop
	sleep(1);
	if(cfo.bufferTestThreadStruct_->running_)
		sleep(1); //give 1 more second for thread

	std::cout << "time(0) = " << time(0) << '\n' << std::flush;
	__COUT_INFO__ << "\n" << 
		CFOFrontEndInterface::getDetachedBufferTestStatus(cfo.bufferTestThreadStruct_) << __E__;	

	if(dumpSpy)
		cfo.getDevice()->spy(CFO_DMA_Engine_DAQ, 3 /* for once */ | 8 /* for wide view */ | 16 /* for stack trace */);

	__COUT_INFO__ << "Thread and main exited!" << __E__;
	return 0;
} //end main()
catch(const std::runtime_error& e)
{
	__COUT_ERR__ << "Exception caught:\n\n" << e.what();
}
catch(...)
{
	__COUT_ERR__ << "Unknown exception caught.";
}