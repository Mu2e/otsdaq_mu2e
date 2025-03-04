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
	__COUT_INFO__ << "DCS Test main()";

	if(TTEST(1))
	{
		__COUTTV__(argc);
		for(int i = 0; i < argc; ++i)
		{
			__COUTT__ << "arg[" << i << "] = " << argv[i] << __E__;
		}
	}
	std::stringstream usage;
	usage << "\n\n\tUsage = Need at least 4 arguments: DCS_test <DTC device index> "
	         "<w/r/s> <target ROC link> <ROC address> <for write, ROC data>\n\n"
	      << __E__;
	usage << "\n\n\t\t 2 arguments for ROC setup (s, or se for emulated ROC), 4 "
	         "arguments for ROC reads (r) and 5 arguments for ROC writes (w).\n\n"
	      << __E__;

	uint32_t    deviceIndex = atoi(argv[1]);
	std::string rwOp        = argv[2];
	int         roc_link    = -1;   //atoi(argv[3]);
	std::string roc_addr    = "0";  //atoi(argv[4]);
	std::string roc_wdata   = "0";

	bool rocSetup         = false;
	bool rocEmulatorSetup = false;
	bool rocRead          = false;
	bool rocWrite         = false;

	if(rwOp == "s" || rwOp == "S")
	{
		if(argc < 3)  //show usage
		{
			__COUT_ERR__ << "Missing arguments for ROC Setup!\n\n" << usage.str();
			return 0;
		}
		__COUT_INFO__ << "SETUP ROC Operation selected! DTC=" << deviceIndex
		              << " ROC=" << roc_link << __E__;
		rocSetup = true;
	}
	else if(rwOp == "se" || rwOp == "SE")
	{
		if(argc < 3)  //show usage
		{
			__COUT_ERR__ << "Missing arguments for Emulated ROC Setup!\n\n"
			             << usage.str();
			return 0;
		}
		__COUT_INFO__ << "SETUP Emulated ROC Operation selected! DTC=" << deviceIndex
		              << " ROC=" << roc_link << __E__;
		rocEmulatorSetup = true;
	}
	else if(rwOp == "r" || rwOp == "R")
	{
		if(argc < 5)  //show usage
		{
			__COUT_ERR__ << "Missing arguments for READ!\n\n" << usage.str();
			return 0;
		}
		roc_link = atoi(argv[3]);
		roc_addr = argv[4];
		__COUT_INFO__ << "READ ROC Operation selected! DTC=" << deviceIndex
		              << " ROC=" << roc_link << " addr=" << roc_addr << __E__;
		rocRead = true;
	}
	else if(rwOp == "w" || rwOp == "W")
	{
		if(argc < 6)  //show usage
		{
			__COUT_ERR__ << "Missing arguments for WRITE!\n\n" << usage.str();
			return 0;
		}
		roc_link  = atoi(argv[3]);
		roc_addr  = argv[4];
		roc_wdata = argv[5];

		__COUT_INFO__ << "WRITE ROC Operation selected! DTC=" << deviceIndex
		              << " ROC=" << roc_link << " addr=" << roc_addr
		              << " data=" << roc_wdata << __E__;
		rocWrite = true;
	}
	else
	{
		__COUT_ERR__ << "Invalid operation '" << rwOp << "'\n\n" << usage.str();
		return 0;
	}

	//==============================================================================
	// Define environment variables
	//	Note: normally these environment variables are set by StartOTS.sh

	// These are needed by
	// otsdaq/otsdaq/ConfigurationDataFormats/ConfigurationInfoReader.cc [207]
	setenv("CONFIGURATION_TYPE", "File", 1);  // Can be File, Database, DatabaseTest
	setenv("CONFIGURATION_DATA_PATH",
	       (std::string(getenv("USER_DATA")) + "/ConfigurationDataExamples").c_str(),
	       1);
	setenv(
	    "TABLE_INFO_PATH", (std::string(getenv("USER_DATA")) + "/TableInfo").c_str(), 1);
	////////////////////////////////////////////////////

	// Some configuration plug-ins use __ENV__("OTSDAQ_LIB") and
	// __ENV__("OTSDAQ_UTILITIES_LIB") in init() so define it 	to a non-sense place is ok
	setenv("OTSDAQ_LIB", (std::string(getenv("USER_DATA")) + "/").c_str(), 1);
	setenv("OTSDAQ_UTILITIES_LIB", (std::string(getenv("USER_DATA")) + "/").c_str(), 1);

	// Some configuration plug-ins use __ENV__("OTS_MAIN_PORT") in init() so define it
	setenv("OTS_MAIN_PORT", "2015", 1);

	// also xdaq envs for XDAQContextTable
	setenv("XDAQ_CONFIGURATION_DATA_PATH",
	       (std::string(getenv("USER_DATA")) + "/XDAQConfigurations").c_str(),
	       1);
	setenv("XDAQ_CONFIGURATION_XML", "otsConfigurationNoRU_CMake", 1);

	if(getenv("OTSDAQ_LOG_FHICL") == NULL)
		setenv("OTSDAQ_LOG_FHICL",
		       (std::string(__ENV__("USER_DATA")) +
		        "/MessageFacilityConfigurations/MessageFacilityWithCout.fcl")
		           .c_str(),
		       1);

	if(getenv("OTSDAQ_LOG_ROOT") == NULL)
		setenv(
		    "OTSDAQ_LOG_ROOT", (std::string(__ENV__("USER_DATA")) + "/Logs").c_str(), 1);

	////////////////////////////////////////////////////

	// // Variables
	std::string supervisorContextUID_ = "calo_01_FEContext";
	std::string supervisorApplicationUID_ =
	    "Calo01FEContext";  //not is misnomer, should be 'Calo01FESupervisor'
	std::string feUID_ =
	    deviceIndex == 0 ? "caloDTC0"
	                     : "caloDTC1";  //caloDTC0 for Device0 and caloDTC1 for Device1
	std::string theConfigurationPath_ =
	    supervisorContextUID_ + "/LinkToApplicationTable/" + supervisorApplicationUID_ +
	    "/LinkToSupervisorTable/LinkToFEInterfaceTable/" + feUID_ + "/LinkToFETypeTable";

	ConfigurationManager cfgMgr;

	if(0)
	{
		//need to activate configure group
		cfgMgr.restoreActiveTableGroups(
		    true,  //bool                                throwErrors /*=false*/,
		    "",    //const std::string&                  pathToActiveGroupsFile /*=""*/,
		    ConfigurationManager::LoadGroupType::
		        ALL_TYPES  //ConfigurationManager::LoadGroupType onlyLoadIfBackboneOrContext /*= ConfigurationManager::LoadGroupType::ALL_TYPES */,
		    //std::string*                        accumulatedWarnings /*=0*/)
		);
	}
	cfgMgr.loadTableGroup("MC2CaloContext", TableGroupKey(21), true);
	cfgMgr.loadTableGroup("MC2CaloConfig", TableGroupKey(26), true);

	// std::string name = cfgMgr.getNode(ConfigurationManager::XDAQ_CONTEXT_TABLE_NAME).getBackNode(theConfigurationPath_)
	// 	    .getNode("FEInterfacePluginName")
	// 	    .getValue<std::string>();
	// __COUTV__(name);

	__COUTV__(deviceIndex);
	__COUTV__(rwOp);
	__COUTV__(roc_link);
	__COUTV__(roc_addr);
	__COUTV__(roc_wdata);

	DTCFrontEndInterface dtc(
	    feUID_,
	    cfgMgr.getNode(ConfigurationManager::XDAQ_CONTEXT_TABLE_NAME),
	    theConfigurationPath_);

	__COUT_INFO__ << "DTC version = " << dtc.getDTC()->ReadDesignDate() << __E__;

	if(rocSetup)
	{
		dtc.SetupCFOInterface(0,      //int forceCFOedge,
		                      true,   //bool useCFOemulator,
		                      true,   //bool alsoSetupJA,
		                      true,   //bool cfoRxTxEnable,
		                      true);  //bool enableAutogenDRP);

		std::string reply = dtc.SetupROCs(
		    DTCLib::DTC_Link_ID(roc_link),  //DTCLib::DTC_Link_ID rocLinkIndex,
		    1,                              //bool rocRxTxEnable
		    0,                              //bool rocTimingEnable
		    0,                              //bool rocEmulationEnable
		    DTCLib::DTC_ROC_Emulation_Type(
		        0 /* 0: Internal, 1: Fiber-Loopback, 2: External */),  // DTCLib::DTC_ROC_Emulation_Type rocEmulationType,
		    0                                                          // uint32_t size
		);
		__COUT_INFO__ << "result: \n" << reply << __E__;
	}
	else if(rocEmulatorSetup)
	{
		dtc.SetupCFOInterface(0,      //int forceCFOedge,
		                      true,   //bool useCFOemulator,
		                      true,   //bool alsoSetupJA,
		                      true,   //bool cfoRxTxEnable,
		                      true);  //bool enableAutogenDRP);

		std::string reply = dtc.SetupROCs(
		    DTCLib::DTC_Link_ID(roc_link),  //DTCLib::DTC_Link_ID rocLinkIndex,
		    1,                              //bool rocRxTxEnable
		    0,                              //bool rocTimingEnable
		    1,                              //bool rocEmulationEnable
		    DTCLib::DTC_ROC_Emulation_Type(
		        0 /* 0: Internal, 1: Fiber-Loopback, 2: External */),  // DTCLib::DTC_ROC_Emulation_Type rocEmulationType,
		    16                                                         // uint32_t size
		);
		__COUT_INFO__ << "result: " << reply << __E__;
	}
	else if(rocRead)
	{
		std::vector<FEVInterface::frontEndMacroArg_t> argsOut;
		std::vector<FEVInterface::frontEndMacroArg_t> argsIn;
		__SET_ARG_IN__("rocLinkIndex", roc_link);
		__SET_ARG_IN__("address", roc_addr);

		__COUTV__(StringMacros::vectorToString(argsIn));
		dtc.runSelfFrontEndMacro(
		    "ROC Read",  //const std::string& feMacroName,
		    argsIn,      //const std::vector<FEVInterface::frontEndMacroArg_t>& inputArgs,
		    argsOut);  //std::vector<FEVInterface::frontEndMacroArg_t>& outputArgs) const;

		if(roc_link != -1)  //treat as number from one ROC
		{
			uint32_t readData = __GET_ARG_OUT__("readData", uint32_t);

			__COUT_INFO__ << "---> read data = " << std::dec << readData << " (0x"
			              << std::hex << readData << ")" << __E__;
		}
		else  //treat as string from all ROCs
		{
			std::string readData = __GET_ARG_OUT__("readData", std::string);

			__COUT_INFO__ << "---> read data = " << readData << __E__;
		}
	}
	else if(rocWrite)
	{
		std::vector<FEVInterface::frontEndMacroArg_t> argsOut;
		std::vector<FEVInterface::frontEndMacroArg_t> argsIn;
		__SET_ARG_IN__("rocLinkIndex", roc_link);
		__SET_ARG_IN__("address", roc_addr);
		__SET_ARG_IN__("writeData", roc_wdata);

		__COUTV__(StringMacros::vectorToString(argsIn));
		dtc.runSelfFrontEndMacro(
		    "ROC Write",  //const std::string& feMacroName,
		    argsIn,    //const std::vector<FEVInterface::frontEndMacroArg_t>& inputArgs,
		    argsOut);  //std::vector<FEVInterface::frontEndMacroArg_t>& outputArgs) const;

		__COUT_INFO__ << "result: " << __E__;
	}
	else
	{
		__COUT_ERR__ << "Invalid operation '" << rwOp << "'\n\n" << usage.str();
		return 0;
	}

	__COUT_INFO__ << "test complete!" << __E__;
	return 0;
}  //end main()
catch(const std::runtime_error& e)
{
	//Note: __COUT_ERR__ will truncate large error messages (with stack traces, etc.)
	std::cout << "Exception caught:\n\n" << e.what();
}
catch(...)
{
	__COUT_ERR__ << "Unknown exception caught.";
}
