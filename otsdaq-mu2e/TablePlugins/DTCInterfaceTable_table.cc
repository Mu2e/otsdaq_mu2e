#include "otsdaq-mu2e/TablePlugins/DTCInterfaceTable.h"
#include "otsdaq/Macros/TablePluginMacros.h"  //for DEFINE_OTS_TABLE

#include "otsdaq/TablePlugins/XDAQContextTable/XDAQContextTable.h"

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

				std::string location = fePair.first + "_" + rocChildPair.first;

				ConfigurationTree slowControlsLink = rocChildPair.second.getNode(
				    rocColNames_.colLinkToSlowControlsChannelTable_);
				numberOfROCSlowControlsChannels = slowControlsHandler(out,
				                                                      tabStr,
				                                                      commentStr,
				                                                      subsystem,
				                                                      location,
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

//==============================================================================
// return status structures
std::string DTCInterfaceTable::getStructureStatusAsJSON (ConfigurationManager* cfgMgr) const
{
	//Steps:
	//	Get all FE Supervisors
	//		for each FE Supervisor
	//			find DTCs
	//				for each DTC
	//					mark if parent is enabled
	//				 	mark if self is enabled
	//					for each ROC
	//						mark if self is enabled

	std::stringstream json;

	const XDAQContextTable* contextTable = cfgMgr->__GET_CONFIG__(XDAQContextTable);
	const std::vector<XDAQContextTable::XDAQContext>& contexts = contextTable->getContexts();


	const std::string COL_NAME_feGroupLink 	= "LinkToFEInterfaceTable";
	const std::string COL_NAME_fePlugin    	= "FEInterfacePluginName";
	const std::string COL_NAME_feTypeLink  	= "LinkToFETypeTable";
	const std::string COL_NAME_rocGroupLink	= "LinkToROCGroupTable";

	const std::string PLUGIN_TYPE_dtc  		= "DTCFrontEndInterface";

	__COUTV__(contexts.size());

	json << "{\"apps\": [";

	bool firstApp = true;
	for(auto& context : contexts)
	{
		for(auto& app : context.applications_) //App loop
		{
			if(XDAQContextTable::FETypeClassNames_.find(app.class_) == XDAQContextTable::FETypeClassNames_.end())
				continue;

			__COUTV__(app.applicationUID_); //all FE Supervisors

			bool parentEnabled = (context.status_ && app.status_);

			ConfigurationTree appNode = cfgMgr->getNode(
				ConfigurationManager::XDAQ_APPLICATION_TABLE_NAME + "/" +
				app.applicationUID_);

			std::vector<std::pair<std::string, ConfigurationTree>> feChildren = 
				appNode.getNode(XDAQContextTable::colApplication_.colLinkToSupervisorTable_).
					getNode(COL_NAME_feGroupLink).getChildren();

			if(!firstApp) json << ", ";
			firstApp = false;

			json << "{\"name\": \"" << context.contextUID_ << "_" << app.applicationUID_  << "\" ";
			json << ", \"enabled\": \"" << (parentEnabled?"1":"0") << "\"";
			json << ", \"dtcs\": [";

			bool firstDTC = true;

			for(const auto& interface : feChildren) //DTC loop
			{
				if(interface.second.getNode(COL_NAME_fePlugin).getValue<std::string>() !=
					PLUGIN_TYPE_dtc)
					continue;

				__COUTV__(interface.first); //all DTCs
				__COUTV__(parentEnabled);
				__COUTV__(interface.second.status());

				if(!firstDTC) json << ", ";
				firstDTC = false;

				json << "{\"name\": \"" << interface.first << "\" ";
				json << ", \"parentApp\": \"" << 
					context.address_ << ":" << context.port_ << "/" <<
					context.contextUID_ << "/" <<
					app.applicationUID_ << "\"";
				json << ", \"parentEnabled\": \"" << (parentEnabled?"1":"0") << "\"";
				json << ", \"enabled\": \"" << (interface.second.status()?"1":"0") << "\"";
				json << ", \"rocs\": [";

				std::vector<std::pair<std::string, ConfigurationTree>> dtcChildren = 
					interface.second.getNode(COL_NAME_feTypeLink + "/" + COL_NAME_rocGroupLink).getChildren();

				bool firstROC = true;
				for(const auto& roc : dtcChildren) //ROC loop
				{
					if(!firstROC) json << ", ";
					firstROC = false;

					json << "{\"name\": \"" << roc.first << "\" ";
					json << ", \"enabled\": \"" << (roc.second.status()?"1":"0") << "\"";
					json << "}"; //close ROC structure
				} //end ROC loop
				json << "]}"; //end ROC array
			} //end DTC loop
			json << "]}"; //end DTC array
		} //end primary application loop
	} //end primary context loop
	json << "]}"; //end primary application structure
	__COUTV__(json.str());
	return json.str();
}  // end getStructureStatusAsJSON()

DEFINE_OTS_TABLE(DTCInterfaceTable)
