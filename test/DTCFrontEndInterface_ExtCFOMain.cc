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
try
{
   	__COUT_INFO__ << "DTCFrontEndInterface External CFO main()";

	__COUTV__(argc);
	for(int i=0;i<argc;++i)
	{
		__COUT_INFO__ << "arg[" << i << "] = " << argv[i] << __E__;
	}
	if(argc < 3)
	{
		__COUT_ERR__ << "\n\n\tUsage = Need at least 2 arguments: DTCFrontEndInterface_ExtCFOMain <deviceIndex> <numberOfEventWindowMarkers>\n\n" << __E__;
		__COUT_ERR__ << "\n\n\t\t 3+ aruments will apply ROC emulator data generation size.\n\n" << __E__;
		return 0;
	}

	uint32_t deviceIndex = atoi(argv[1]);
	uint32_t numberOfEventWindowMarkers = atoi(argv[2]);

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

    DTCFrontEndInterface dtc(feUID_,
        cfgMgr.getNode(ConfigurationManager::XDAQ_CONTEXT_TABLE_NAME),
        theConfigurationPath_);

	
	if(numberOfEventWindowMarkers == uint32_t(-1))
	{
		__COUT_INFO__ << "Setting up DTC with external CFO for Loopback!" << __E__;

		dtc.thisDTC_->DisableCFOEmulation(); //enable external CFO
		dtc.thisDTC_->EnableCFOLoopback(); //loopback at this DTC

		dtc.SetupCFOInterface(
			0, //int forceCFOedge, 
			false, //bool useCFOemulator, 
			true, //bool alsoSetupJA,
			true, //bool cfoRxTxEnable, 
			true); //bool enableAutogenDRP);

		dtc.thisDTC_->SoftReset();
		dtc.thisDTC_->ReleaseAllBuffers(DTC_DMA_Engine_DAQ);
		__COUT_INFO__ << "Reset and ReleaseAllBuffers called!" << __E__;
		return 0;
	}
	if(numberOfEventWindowMarkers == uint32_t(-2))
	{
		__COUT_INFO__ << "Setting up DTC with external CFO for Passhthrough!" << __E__;
		dtc.thisDTC_->DisableCFOEmulation(); //enable external CFO
		dtc.thisDTC_->DisableCFOLoopback(); //passhtrough CFO control link at this DTC
		
		dtc.SetupCFOInterface(
			0, //int forceCFOedge, 
			false, //bool useCFOemulator, 
			true, //bool alsoSetupJA,
			true, //bool cfoRxTxEnable, 
			true); //bool enableAutogenDRP);

		dtc.thisDTC_->SoftReset();
		dtc.thisDTC_->ReleaseAllBuffers(DTC_DMA_Engine_DAQ);
		__COUT_INFO__ << "Reset and ReleaseAllBuffers called!" << __E__;
		return 0;
	}
	if(numberOfEventWindowMarkers == uint32_t(-3))
	{
		__COUT_INFO__ << "Setting to 32KB Max DMA Transfer size!" << __E__;
		dtc.thisDTC_->SetTriggerDMATransferLength(0x8000);
		__COUT_INFO__ << "DTC DMA sizes = " << dtc.thisDTC_->FormatDMATransferLength() << __E__;
		return 0;
	}
	if(numberOfEventWindowMarkers == uint32_t(-4))
	{
		__COUT_INFO__ << "Setting to 64KB Max DMA Transfer size!" << __E__;
		dtc.thisDTC_->SetTriggerDMATransferLength(0xFFF8);
		__COUT_INFO__ << "DTC DMA sizes = " << dtc.thisDTC_->FormatDMATransferLength() << __E__;
		return 0;
	}


    __COUT_INFO__ << "DTC version = " << dtc.thisDTC_->ReadDesignDate() << __E__;
	__COUT_INFO__ << "DTC DMA sizes = " << dtc.thisDTC_->FormatDMATransferLength() << __E__;

	dtc.SetupCFOInterface(
		0, //int forceCFOedge, 
		false, //bool useCFOemulator, 
		true, //bool alsoSetupJA,
		true, //bool cfoRxTxEnable, 
		true); //bool enableAutogenDRP);

	//setup ROCs
	std::string reply;
	for(int i=3;i<argc;++i)
	{
		int sz = atoi(argv[i]);
		__COUT_INFO__ << "ROC #" << i-3 << " size arg[" << i << "] = " << sz << __E__;

		if(sz == -1) //disabled
			reply = dtc.SetupROCs(
				DTCLib::DTC_Link_ID(i-3), //]DTCLib::DTC_Link_ID rocLinkIndex,
				0, 1, 1, //bool rocRxTxEnable, bool rocTimingEnable, bool rocEmulationEnable,
				DTCLib::DTC_ROC_Emulation_Type(0 /* 0: Internal, 1: Fiber-Loopback, 2: External */),// DTCLib::DTC_ROC_Emulation_Type rocEmulationType,
				0// uint32_t size
			);
		else
			reply = dtc.SetupROCs(
				DTCLib::DTC_Link_ID(i-3), //]DTCLib::DTC_Link_ID rocLinkIndex,
				1, 1, 1, //bool rocRxTxEnable, bool rocTimingEnable, bool rocEmulationEnable,
				DTCLib::DTC_ROC_Emulation_Type(0 /* 0: Internal, 1: Fiber-Loopback, 2: External */),// DTCLib::DTC_ROC_Emulation_Type rocEmulationType,
				atoi(argv[i])// uint32_t size
			);
	}
	__COUT_INFO__ << "ROC Setup:\n" << reply << __E__;

	dtc.thisDTC_->SoftReset(); //to reset event window tag starting point handling
	dtc.initDetachedBufferTest(100,//initialEventWindowTag,
			false, // saveBinaryDataToFile, 
			false, //saveSubeventHeadersToDataFile, 
			false); //doNotResetCounters);
    
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
				DTCFrontEndInterface::getDetachedBufferTestStatus(dtc.bufferTestThreadStruct_) << __E__;	
		}

		if((i > 5 && !dtc.bufferTestThreadStruct_->running_) || 
			DTCFrontEndInterface::getDetachedBufferTestReceivedCount(dtc.bufferTestThreadStruct_) >= numberOfEventWindowMarkers - 1)// start mutex scope
		{
			__COUT_INFO__ << "Iteration exit #" << i << " - thread running = " << dtc.bufferTestThreadStruct_->running_ << "\n" << 
				DTCFrontEndInterface::getDetachedBufferTestStatus(dtc.bufferTestThreadStruct_) << __E__;	

			sleep(1);
			std::lock_guard<std::mutex> lock(dtc.bufferTestThreadStruct_->lock_);
			dtc.bufferTestThreadStruct_->exitThread_ = true;

			if(DTCFrontEndInterface::getDetachedBufferTestReceivedCount(dtc.bufferTestThreadStruct_) != numberOfEventWindowMarkers - 1)
				dumpSpy = true;			
			break;
		}
		++i;
	} //end main loop
	sleep(1);
	if(dtc.bufferTestThreadStruct_->running_)
		sleep(1); //give 1 more second for thread

	if(dumpSpy)
		dtc.getDevice()->spy(DTC_DMA_Engine_DAQ, 3 /* for once */ | 8 /* for wide view */ | 16 /* for stack trace */);

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