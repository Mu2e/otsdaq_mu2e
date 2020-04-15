#include "otsdaq-mu2e/TablePlugins/DTCInterfaceTable.h"
#include "otsdaq/Macros/TablePluginMacros.h"  //for DEFINE_OTS_TABLE

#include <sys/stat.h>  //for mkdir
#include <fstream>     // std::fstream
#include <iostream>

using namespace ots;

// clang-format off

#define EPICS_PV_FILE_PATH \
		std::string( \
			getenv("OTSDAQ_EPICS_DATA")? \
				(std::string(getenv("OTSDAQ_EPICS_DATA")) + "/" + __ENV__("MU2E_OWNER") + "_otsdaq_dtc-ai.dbg"): \
				(EPICS_CONFIG_PATH + "/otsdaq_dtc-ai.dbg")  )

// clang-format on

//==============================================================================
DTCInterfaceTable::DTCInterfaceTable(void)
    : TableBase("DTCInterfaceTable")
    , SlowControlsTableBase("DTCInterfaceTable")
    //, isFirstAppInContext_(false)
    //, channelListHasChanged_(false)
    //, lastConfigManager_	(nullptr)
    {}

    //==============================================================================
    DTCInterfaceTable::~DTCInterfaceTable(void)
{
}

//==============================================================================
// init
//	Generates EPICS PV config file
void DTCInterfaceTable::init(ConfigurationManager* configManager)
{
	
	lastConfigManager_ = configManager;
	
	// use isFirstAppInContext to only run once per context, for example to avoid
	//	generating files on local disk multiple times.
	isFirstAppInContext_ = configManager->isOwnerFirstAppInContext();
	
	channelListHasChanged_ = false;
	
	//__COUTV__(isFirstAppInContext);
	if(!isFirstAppInContext_)
		return;

	// make directory just in case
	mkdir(EPICS_CONFIG_PATH.c_str(), 0755);

	// check for valid data types
	__COUT__ << "*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*" << __E__;
	__COUT__ << configManager->__SELF_NODE__ << __E__;

	//outputEpicsPVFile(configManager);
}  // end init()

//==============================================================================
//return channel list if pointer passed
bool DTCInterfaceTable::outputEpicsPVFile(
    ConfigurationManager* configManager,
    std::vector<std::pair<std::string /*channelName*/, std::vector<std::string>>>*
        channelList /*= 0*/) const
{	
	/*
	    the file will look something like this:

	        file name.template {
	          pattern { var1, var2, var3, ... }
	              { sub1_for_set1, sub2_for_set1, sub3_for_set1, ... }
	              { sub1_for_set2, sub2_for_set2, sub3_for_set2, ... }
	              { sub1_for_set3, sub2_for_set3, sub3_for_set3, ... }

	              ...
	          }

	          # for comment lines

	    file "soft_ai.dbt"  -- for floating point ("analog") data

	    file "soft_bi.dbt"  -- for binary values (on/off, good/bad, etc)

	    file "soft_stringin.dbt" -- for string values (e.g. "states")

	    Subsystem names:
	   https://docs.google.com/spreadsheets/d/1SO8R3O5Xm37X0JdaBiVmbg9p9aXy1Gk13uqiWFCchBo/edit#gid=1775059019
	    DTC maps to: CRV, Tracker, EMCal, STM, TEM

	    Example lines:

	    file "soft_ai.dbt" {
	        pattern  { Subsystem, loc, var, PREC, EGU, LOLO, LOW, HIGH, HIHI, MDEL, ADEL,
	   DESC } { "TDAQ", "DataLogger", "RunNumber", "0", "", "-1e23", "-1e23", "1e23",
	   "1e23", "", "", "DataLogger run number" } { "TDAQ", "DataLogger", "AvgEvtSize",
	   "0", "MB/evt", "-1e23", "-1e23", "1e23", "1e23", "", "", "Datalogger avg event
	   size" }
	    }

	    file "soft_bi.dbt" {
	        pattern  { Subsystem, loc, pvar, ZNAM, ONAM, ZSV, OSV, COSV, DESC  }
	             { "Computer", "daq01", "voltages_ok", "NOT_OK", "OK", "MAJOR",
	   "NO_ALARM", "", "voltages_ok daq01"  } { "Computer", "daq02", "voltages_ok",
	   "NOT_OK", "OK", "MAJOR", "NO_ALARM", "", "voltages_ok daq02"  }
	    }
	*/

	std::string filename = EPICS_PV_FILE_PATH;

	__COUT__ << "EPICS PV file: " << filename << __E__;
	
	std::string previousConfigFileContents;
	{
		std::FILE* fp = std::fopen(filename.c_str(), "rb");
		if(fp)
		{
			std::fseek(fp, 0, SEEK_END);
			previousConfigFileContents.resize(std::ftell(fp));
			std::rewind(fp);
			std::fread(&previousConfigFileContents[0], 1, previousConfigFileContents.size(), fp);
			std::fclose(fp);
		}
		else 
			__COUT_WARN__ <<  "Could not open EPICS IOC config file at " << filename << __E__;
			
	} //done reading

	/////////////////////////
	// generate xdaq run parameter file
	
	std::stringstream out;

	std::string tabStr     = "";
	std::string commentStr = "";

	// loop through DTC records starting at FE Interface Table
	std::vector<std::pair<std::string, ConfigurationTree>> feRecords =
		configManager->getNode("FEInterfaceTable").getChildren();

	std::string  rocPluginType;
	unsigned int numberOfDTCs = 0;
	std::string  subsystem    = std::string("TDAQ_") + __ENV__("MU2E_OWNER");

	for(auto& fePair : feRecords)  // start main fe/DTC record loop
	{
		if(!fePair.second.status() ||
				fePair.second.getNode(feColNames_.colFEInterfacePluginName_)
				.getValue<std::string>() != DTC_FE_PLUGIN_TYPE)
			continue;

		++numberOfDTCs;

		// check each row in table
		__COUT__ << "DTC record: " << fePair.first << __E__;

		// loop through each DTC slow controls channel and make entry in EPICS file
		{
			ConfigurationTree slowControlsLink =
				fePair.second.getNode(feColNames_.colLinkToSlowControlsChannelTable_);
			unsigned int numberOfDTCSlowControlsChannels =
			    slowControlsHandler(
						  out
	            		, tabStr
						, commentStr
						, subsystem
			            , fePair.first
			            , slowControlsLink
			            , channelList
				);

			__COUT__ << "DTC '" << fePair.first
				<< "' number of slow controls channels: "
				<< numberOfDTCSlowControlsChannels << __E__;
		}  // end DTC slow controls channel handling

		// loop through ROC records
		//	use plugin type to indicate subsystem type

		ConfigurationTree DTCLink =
			fePair.second.getNode(feColNames_.colLinkToFETypeTable);
		if(DTCLink.isDisconnected())
		{
			__COUT__
				<< "Disconnected DTC type table information. So assuming no ROCs."
				<< __E__;
			continue;
		}
		ConfigurationTree ROCLink =
			DTCLink.getNode(dtcColNames_.colLinkToROCGroupTable_);
		if(ROCLink.isDisconnected())
		{
			__COUT__ << "Disconnected ROC link. So assuming no ROCs." << __E__;
			continue;
		}
		std::vector<std::pair<std::string, ConfigurationTree>> rocChildren =
			ROCLink.getChildren();

		unsigned int numberOfROCSlowControlsChannels;
		for(auto& rocChildPair : rocChildren)
		{
			__COUT__ << "\t"
				<< "ROC record: " << rocChildPair.first << __E__;
			numberOfROCSlowControlsChannels = 0;
			try
			{
				rocPluginType = rocChildPair.second
					.getNode(rocColNames_.colROCInterfacePluginName_)
					.getValue<std::string>();
				__COUTV__(rocPluginType);

				ConfigurationTree slowControlsLink = rocChildPair.second.getNode(
						rocColNames_.colLinkToSlowControlsChannelTable_);
				numberOfROCSlowControlsChannels =
					slowControlsHandler(
							  out
							, tabStr
							, commentStr
							, subsystem
							, rocChildPair.first
							, slowControlsLink
							, channelList
					);
			}
			catch(const std::runtime_error& e)
			{
				__COUT_ERR__ << "Ignoring ROC error: " << e.what() << __E__;
			}

			__COUT__ << "\t"
				<< "ROC '" << rocChildPair.first
				<< "' number of slow controls channels: "
				<< numberOfROCSlowControlsChannels << __E__;

		}  // end ROC record loop
	}      // end main fe/DTC record loop

	__COUTV__(numberOfDTCs);	

	//check if need to restart EPICS ioc
	//	if dbg string has changed, then mark ioc configuration dirty
	if(previousConfigFileContents != out.str())
	{
		__COUT__ << "Configuration has changed! Marking dirty flag..." << __E__;

		//only write files if first app in context AND channelList is not passed, i.e. init() is only time we write!
		//if(isFirstAppInContext_ && channelList == nullptr)
		if(channelList == nullptr)
		{
			std::fstream fout;
			fout.open(filename, std::fstream::out | std::fstream::trunc);
			if(fout.fail())
			{
				__SS__ << "Failed to open EPICS PV file: " << filename << __E__;
				__SS_THROW__;
			}

			fout << out.str();
			fout.close();

			std::FILE* fp = fopen(EPICS_DIRTY_FILE_PATH.c_str(),"w");
			if(fp)
			{			
				fprintf(fp,"1"); //set dirty flag
				fclose(fp);
			}
			else
				__COUT_WARN__ << "Could not open dirty file: " << EPICS_DIRTY_FILE_PATH << __E__;
		}

		//Indicate that PV list has changed 
		//	if otsdaq_epics plugin is listening, then write PV data to archive db:	SQL insert or modify of ROW for PV
		__COUT__ << "outputEpicsPVFile() return true" << __E__;
		return true;
	} //end handling of previous contents
	__COUT__ << "outputEpicsPVFile() return false" << __E__;
	return false;
}  // end outputEpicsPVFile()

DEFINE_OTS_TABLE(DTCInterfaceTable)
