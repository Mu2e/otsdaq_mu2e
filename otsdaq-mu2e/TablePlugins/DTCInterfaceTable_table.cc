#include "otsdaq-mu2e/TablePlugins/DTCInterfaceTable.h"
#include "otsdaq/Macros/TablePluginMacros.h"  //for DEFINE_OTS_TABLE

#include <sys/stat.h>  //for mkdir
#include <iostream>

using namespace ots;

// clang-format off

#define SLOWCONTROL_PV_FILE_PATH \
		std::string( \
			getenv("OTSDAQ_EPICS_DATA")? \
				(std::string(getenv("OTSDAQ_EPICS_DATA")) + "/" + __ENV__("MU2E_OWNER") + "_otsdaq_dtc-ai.dbg"): \
				(EPICS_CONFIG_PATH + "/otsdaq_dtc-ai.dbg")  )

// clang-format on

//==============================================================================
DTCInterfaceTable::DTCInterfaceTable(void)
    : TableBase("DTCInterfaceTable")
    , SlowControlsTableBase("DTCInterfaceTable")
{
}

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
// Configuruing and start of slowControlsHandler function
unsigned int	DTCInterfaceTable::slowControlsHandlerConfig	(
														  std::stringstream& out
														, ConfigurationManager* configManager
														, std::vector<std::pair<std::string /*channelName*/, std::vector<std::string>>>* channelList /*= 0*/
											) const
{
	/////////////////////////
	// generate xdaq run parameter file

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
			    slowControlsHandler(out,
			                        tabStr,
			                        commentStr,
			                        subsystem,
			                        fePair.first,
			                        slowControlsLink,
			                        channelList);

			__COUT__ << "DTC '" << fePair.first << "' number of slow controls channels: "
			         << numberOfDTCSlowControlsChannels << __E__;
		}  // end DTC slow controls channel handling

		// loop through ROC records
		//	use plugin type to indicate subsystem type

		ConfigurationTree DTCLink =
		    fePair.second.getNode(feColNames_.colLinkToFETypeTable);
		if(DTCLink.isDisconnected())
		{
			__COUT__ << "Disconnected DTC type table information. So assuming no ROCs."
			         << __E__;
			continue;
		}
		ConfigurationTree ROCLink = DTCLink.getNode(dtcColNames_.colLinkToROCGroupTable_);
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
				rocPluginType =
				    rocChildPair.second.getNode(rocColNames_.colROCInterfacePluginName_)
				        .getValue<std::string>();
				__COUTV__(rocPluginType);

				ConfigurationTree slowControlsLink = rocChildPair.second.getNode(
				    rocColNames_.colLinkToSlowControlsChannelTable_);
				numberOfROCSlowControlsChannels = slowControlsHandler(out,
				                                                      tabStr,
				                                                      commentStr,
				                                                      subsystem,
				                                                      rocChildPair.first,
				                                                      slowControlsLink,
				                                                      channelList);
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

	return numberOfDTCs;
}  // end slowControlsHandlerConfig()

//==============================================================================
// return out file path
std::string DTCInterfaceTable::setFilePath() const { return SLOWCONTROL_PV_FILE_PATH; }

DEFINE_OTS_TABLE(DTCInterfaceTable)
