#ifndef _ots_DTCInterfaceTable_h_
#define _ots_DTCInterfaceTable_h_

#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq/TablePlugins/SlowControlsTableBase/SlowControlsTableBase.h"

namespace ots
{
class DTCInterfaceTable : public SlowControlsTableBase
{
	// clang-format off

  public:
	DTCInterfaceTable						(void);
	virtual ~DTCInterfaceTable				(void);

	// Methods
	void 			init					(ConfigurationManager* configManager) override;
	
	// Getters
	virtual bool 	slowControlsChannelListHasChanged 	(void) const override;
	virtual void	getSlowControlsChannelList 			(std::vector<std::string /*channelName*/>& channelList) const override;

  private:

	bool 			outputEpicsPVFile					(ConfigurationManager* configManager, std::vector<std::string /*channelName*/>* channelList = 0) const;

	// Column names
	struct ColFE
	{
		std::string const colFEInterfaceGroupID_ 	= "FEInterfaceGroupID";
		std::string const colStatus_				= "Status";
		std::string const colFEInterfacePluginName_ = "FEInterfacePluginName";
		std::string const colLinkToFETypeTable		= "LinkToFETypeTable";
		std::string const colLinkToSlowControlsChannelTable_ 	= "LinkToSlowControlsChannelTable";
	} feColNames_;

	struct ColDTC
	{
		std::string const colEmulatorMode_ 			= "EmulatorMode";
		std::string const colLocationInCFOChain_	= "LocationInChain";
		std::string const colPCIDeviceIndex_ 		= "DeviceIndex";
		std::string const colEmulateCFO_ 			= "EmulateCFO";
		std::string const colConfigureClock_ 		= "ConfigureClock";
		std::string const colLinkToROCGroupTable_ 	= "LinkToROCGroupTable";
		std::string const colLinkToROCGroupID_ 		= "LinkToROCGroupID";
	} dtcColNames_;

	struct ColROC
	{
		std::string const colStatus_ 				= "Status";
		std::string const colROCGroupID_			= "ROCGroupID";
		std::string const colROCInterfacePluginName_= "ROCInterfacePluginName";
		std::string const colLinkID_ 				= "linkID";
		std::string const colEventWindowDelayOffset_= "EventWindowDelayOffset";
		std::string const colEmulateInDTCHardware_ 	= "EmulateInDTCHardware";
		std::string const colROCTypeLinkTable_ 		= "ROCTypeLinkTable";
		std::string const colLinkToSlowControlsChannelTable_ 	= "LinkToSlowControlsChannelTable";
	} rocColNames_;

	struct ColChannel
	{
		std::string const colStatus_ 				= "Status";
		std::string const colUnits_ 				= "Units";
		std::string const colChannelDataType_		= "ChannelDataType";
		std::string const colLowLowThreshold_		= "LowLowThreshold";
		std::string const colLowThreshold_ 			= "LowThreshold";
		std::string const colHighThreshold_			= "HighThreshold";
		std::string const colHighHighThreshold_ 	= "HighHighThreshold";
	} channelColNames_;

	std::string const 		DTC_FE_PLUGIN_TYPE 			= "DTCFrontEndInterface";

	bool					isFirstAppInContext_, channelListHasChanged_; //for managing if PV list has changed
	ConfigurationManager* 	lastConfigManager_;
	// clang-format on
};
}  // namespace ots
#endif
