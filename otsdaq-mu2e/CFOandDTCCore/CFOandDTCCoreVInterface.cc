#include "otsdaq-core/Macros/InterfacePluginMacros.h"
#include "otsdaq-core/PluginMakers/MakeInterface.h"
#include "otsdaq-mu2e/CFOandDTCCore/CFOandDTCCoreVInterface.h"
#include "otsdaq-core/Macros/BinaryStringMacros.h"

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "CFOandDTCCoreVInterface"

//=========================================================================================
CFOandDTCCoreVInterface::CFOandDTCCoreVInterface(
    const std::string&       interfaceUID,
    const ConfigurationTree& theXDAQContextConfigTree,
    const std::string&       interfaceConfigurationPath)
    : FEVInterface(interfaceUID, theXDAQContextConfigTree, interfaceConfigurationPath)
 //   , thisDTC_(0)
 //   , EmulatedCFO_(0)
{
	__FE_COUT__ << "instantiate DTC... " << interfaceUID << " "
	            << theXDAQContextConfigTree << " " << interfaceConfigurationPath << __E__;

	universalAddressSize_ = sizeof(dtc_address_t);
	universalDataSize_    = sizeof(dtc_data_t);

	configure_clock_ = getSelfNode().getNode("ConfigureClock").getValue<bool>();
	//emulate_cfo_     = getSelfNode().getNode("EmulateCFO").getValue<bool>();

	// label
	device_name_ = interfaceUID;

	// linux file to communicate with
	dtc_ = getSelfNode().getNode("DeviceIndex").getValue<unsigned int>();

	try
	{
		emulatorMode_ = getSelfNode().getNode("EmulatorMode").getValue<bool>();
	}
	catch(...)
	{
		__FE_COUT__ << "Assuming NOT emulator mode." << __E__;
		emulatorMode_ = false;
	}
	__FE_COUTV__(emulatorMode_);
//
//	if(emulatorMode_)
//	{
//		__FE_COUT__ << "Emulator DTC mode starting up..." << __E__;
//		createROCs();
//		registerFEMacros();
//		return;
//	}
	// else not emulator mode

	snprintf(devfile_, 11, "/dev/" MU2E_DEV_FILE, dtc_);
	fd_ = open(devfile_, O_RDONLY);

	__FE_COUT__ << "Constructed." << __E__;
	return;
//	unsigned dtc_class_roc_mask = 0;
//	// create roc mask for DTC
//	{
//		std::vector<std::pair<std::string, ConfigurationTree>> rocChildren =
//		    Configurable::getSelfNode().getNode("LinkToROCGroupTable").getChildren();
//
//		roc_mask_ = 0;
//
//		for(auto& roc : rocChildren)
//		{
//			__FE_COUT__ << "roc uid " << roc.first << __E__;
//			bool enabled = roc.second.getNode("Status").getValue<bool>();
//			__FE_COUT__ << "roc enable " << enabled << __E__;
//
//			if(enabled)
//			{
//				int linkID = roc.second.getNode("linkID").getValue<int>();
//				roc_mask_ |= (0x1 << linkID);
//				dtc_class_roc_mask |=
//				    (0x1 << (linkID * 4));  // the DTC class instantiation expects each
//				                            // ROC has its own hex nibble
//			}
//		}
//
//		__FE_COUT__ << "DTC roc_mask_ = 0x" << std::hex << roc_mask_ << std::dec << __E__;
//		__FE_COUT__ << "roc_mask to instantiate DTC class = 0x" << std::hex
//		            << dtc_class_roc_mask << std::dec << __E__;
//
//	}  // end create roc mask
//
//	// instantiate DTC with the appropriate ROCs enabled
//	std::string expectedDesignVersion = "";
//	auto        mode                  = DTCLib::DTC_SimMode_NoCFO;
//
//	std::cout << "DTC arguments..." << std::endl;
//	std::cout << "dtc_ = " << dtc_ << std::endl;
//	std::cout << "rocMask = " << dtc_class_roc_mask << std::endl;
//	std::cout << "expectedDesignVersion = " << expectedDesignVersion << std::endl;
//	std::cout << "END END DTC arguments..." << std::endl;
//
//	thisDTC_ = new DTCLib::DTC(mode, dtc_, dtc_class_roc_mask, expectedDesignVersion);
//
//	if(emulate_cfo_ == 1)
//	{  // do NOT instantiate the DTCSoftwareCFO here, do it just when you need it
//
//		//	  bool useCFOEmulator = true;
//		//	  uint16_t debugPacketCount = 0;
//		//	  auto debugType = DTCLib::DTC_DebugType_SpecialSequence;
//		//	  bool stickyDebugType = true;
//		//	  bool quiet = false;
//		//	  bool asyncRR = false;
//		//	  bool forceNoDebugMode = true;
//
//		//	  std::cout << "DTCSoftwareCFO arguments..." << std::endl;
//		//	  std::cout << "useCFOEmulator = "  << useCFOEmulator << std::endl;
//		//	  std::cout << "packetCount = "     << debugPacketCount << std::endl;
//		//	  std::cout << "debugType = "       << debugType << std::endl;
//		//	  std::cout << "stickyDebugType = " << stickyDebugType << std::endl;
//		//	  std::cout << "quiet = "           << quiet << std::endl;
//		//	  std::cout << "asyncRR = "           << asyncRR << std::endl;
//		//	  std::cout << "forceNoDebug = "     << forceNoDebugMode << std::endl;
//		//	  std::cout << "END END DTCSoftwareCFO arguments..." << std::endl;
//
//		//	  EmulatedCFO_ = new DTCLib::DTCSoftwareCFO(thisDTC_, useCFOEmulator,
//		//debugPacketCount, 				debugType, stickyDebugType, quiet,  asyncRR,
//		//forceNoDebugMode);
//	}
//
//	createROCs();
//	registerFEMacros();
//
//	// DTC-specific info
//	dtc_location_in_chain_ =
//	    getSelfNode().getNode("LocationInChain").getValue<unsigned int>();
//
//	// check if any ROCs should be DTC-hardware emulated ROCs
//	{
//		std::vector<std::pair<std::string, ConfigurationTree>> rocChildren =
//		    Configurable::getSelfNode().getNode("LinkToROCGroupTable").getChildren();
//
//		int dtcHwEmulateROCmask = 0;
//		for(auto& roc : rocChildren)
//		{
//			bool enabled = roc.second.getNode("EmulateInDTCHardware").getValue<bool>();
//
//			if(enabled)
//			{
//				int linkID = roc.second.getNode("linkID").getValue<int>();
//				__FE_COUT__ << "roc uid '" << roc.first << "' at link=" << linkID
//				            << " is DTC-hardware emulated!" << __E__;
//				dtcHwEmulateROCmask |= (1 << linkID);
//			}
//		}
//
//		__FE_COUT__ << "Writing DTC-hardware emulation mask: 0x" << std::hex
//		            << dtcHwEmulateROCmask << std::dec << __E__;
//		registerWrite(0x9110, dtcHwEmulateROCmask);
//		__FE_COUT__ << "End check for DTC-hardware emulated ROCs." << __E__;
//	}  // end check if any ROCs should be DTC-hardware emulated ROCs
//
//	// done
//	__MCOUT_INFO__("CFOandDTCCoreVInterface instantiated with name: "
//	               << device_name_ << " dtc_location_in_chain_ = "
//	               << dtc_location_in_chain_ << " talking to /dev/mu2e" << dtc_ << __E__);
}  // end constructor()

//==========================================================================================
CFOandDTCCoreVInterface::~CFOandDTCCoreVInterface(void)
{
//	if(thisDTC_)
//		delete thisDTC_;
	// delete theFrontEndHardware_;
	// delete theFrontEndFirmware_;

	__FE_COUT__ << "Destructed." << __E__;
}  // end destructor()
//
////========================================================================================================================
//void CFOandDTCCoreVInterface::configureSlowControls(void)
//{
//	__FE_COUT__ << "Configuring slow controls..." << __E__;
//
//	// parent configure adds DTC slow controls channels
//	FEVInterface::configureSlowControls();  // also resets DTC-proper channels
//
//	__FE_COUT__ << "CFO-DTC '" << getInterfaceUID()
//	         << "' slow controls channel count (BEFORE considering ROCs): "
//	         << mapOfSlowControlsChannels_.size() << __E__;
//
//	mapOfROCSlowControlsChannels_.clear();  // reset ROC channels
//
//	// now add ROC slow controls channels
//	try
//	{
//		ConfigurationTree ROCLink =
//				Configurable::getSelfNode().getNode("LinkToROCGroupTable");
//		if(!ROCLink.isDisconnected())
//		{
//			std::vector<std::pair<std::string, ConfigurationTree>> rocChildren =
//					ROCLink.getChildren();
//
//			unsigned int initialChannelCount;
//
//			for(auto& rocChildPair : rocChildren)
//			{
//				initialChannelCount = mapOfROCSlowControlsChannels_.size();
//
//				FEVInterface::addSlowControlsChannels(
//						rocChildPair.second.getNode("LinkToSlowControlsChannelTable"),
//						"/" + rocChildPair.first /*subInterfaceID*/,
//						&mapOfROCSlowControlsChannels_);
//
//				__FE_COUT__ << "ROC '" << getInterfaceUID() << "/" << rocChildPair.first
//						<< "' slow controls channel count: "
//						<< mapOfROCSlowControlsChannels_.size() - initialChannelCount
//						<< __E__;
//
//			}  // end ROC children loop
//
//		}  // end ROC channel handling
//		else
//			__FE_COUT__ << "ROC link disconnected, assuming no ROCs" << __E__;
//	}
//	catch(...)
//	{
//		__FE_COUT__ << "ROC link disconnected, assuming no ROCs" << __E__;
//	}
//
//	__FE_COUT__ << "DTC '" << getInterfaceUID()
//	         << "' slow controls channel count (AFTER considering ROCs): "
//	         << mapOfSlowControlsChannels_.size() + mapOfROCSlowControlsChannels_.size()
//	         << __E__;
//
//	__FE_COUT__ << "Done configuring slow controls." << __E__;
//
//}  // end configureSlowControls()
//
////========================================================================================================================
//// virtual in case channels are handled in multiple maps, for example
//void CFOandDTCCoreVInterface::resetSlowControlsChannelIterator(void)
//{
//	// call parent
//	FEVInterface::resetSlowControlsChannelIterator();
//
//	currentChannelIsInROC_ = false;
//}  // end resetSlowControlsChannelIterator()
//
////========================================================================================================================
//// virtual in case channels are handled in multiple maps, for example
//FESlowControlsChannel* CFOandDTCCoreVInterface::getNextSlowControlsChannel(void)
//{
//	// if finished with DTC slow controls channels, move on to ROC list
//	if(slowControlsChannelsIterator_ == mapOfSlowControlsChannels_.end())
//	{
//		slowControlsChannelsIterator_ = mapOfROCSlowControlsChannels_.begin();
//		currentChannelIsInROC_        = true;  // switch to ROC mode
//	}
//
//	// if finished with ROC list, then done
//	if(slowControlsChannelsIterator_ == mapOfROCSlowControlsChannels_.end())
//		return nullptr;
//
//	if(currentChannelIsInROC_)
//	{
//		std::vector<std::string> uidParts;
//		StringMacros::getVectorFromString(
//				slowControlsChannelsIterator_->second.interfaceUID_,
//				uidParts,{'/'} /*delimiters*/);
//		if(uidParts.size() != 2)
//		{
//			__FE_SS__ << "Illegal ROC slow controls channel name '" <<
//					slowControlsChannelsIterator_->second.interfaceUID_ <<
//					".' Format should be DTC/ROC." << __E__;
//		}
//		currentChannelROCUID_ = uidParts[1]; //format DTC/ROC names, take 2nd part as ROC UID
//	}
//	return &(
//	    (slowControlsChannelsIterator_++)->second);  // return iterator, then increment
//}  // end getNextSlowControlsChannel()
//
////========================================================================================================================
//// virtual in case channels are handled in multiple maps, for example
//unsigned int CFOandDTCCoreVInterface::getSlowControlsChannelCount(void)
//{
//	return mapOfSlowControlsChannels_.size() + mapOfROCSlowControlsChannels_.size();
//}  // end getSlowControlsChannelCount()
//
////========================================================================================================================
//// virtual in case read should be different than universalread
//void CFOandDTCCoreVInterface::getSlowControlsValue(FESlowControlsChannel& channel,
//                                                std::string&           readValue)
//{
//	__FE_COUTV__(currentChannelIsInROC_);
//	__FE_COUTV__(currentChannelROCUID_);
//	__FE_COUTV__(universalDataSize_);
//	if(!currentChannelIsInROC_)
//	{
//		readValue.resize(universalDataSize_);
//		universalRead(channel.getUniversalAddress(), &readValue[0]);
//	}
//	else
//	{
//		auto rocIt = rocs_.find(currentChannelROCUID_);
//		if(rocIt == rocs_.end())
//		{
//			__FE_SS__ << "ROC UID '" << currentChannelROCUID_ <<
//					"' was not found in ROC map." << __E__;
//			ss << "Here are the existing ROCs: ";
//			bool first = true;
//			for(auto& rocPair:rocs_)
//				if(!first) ss << ", " << rocPair.first;
//				else { ss << rocPair.first; first = false; }
//			ss << __E__;
//			__FE_SS_THROW__;
//		}
//		readValue.resize(universalDataSize_);
//		*((uint16_t*)(&readValue[0])) = rocIt->second->readRegister(
//								    *((uint16_t*)channel.getUniversalAddress()));
//	}
//
//	__FE_COUTV__(readValue.size());
//}  // end getNextSlowControlsChannel()
//
////========================================================================================================================
//void CFOandDTCCoreVInterface::registerFEMacros(void)
//{
//	mapOfFEMacroFunctions_.clear();
//
//	// clang-format off
//	registerFEMacroFunction(
//			"ROC_WriteBlock",  // feMacroName
//			static_cast<FEVInterface::frontEndMacroFunction_t>(
//					&CFOandDTCCoreVInterface::WriteROCBlock),  // feMacroFunction
//					std::vector<std::string>{"rocLinkIndex", "block", "address", "writeData"},
//					std::vector<std::string>{},  // namesOfOutputArgs
//					1);                          // requiredUserPermissions
//
//	registerFEMacroFunction("ROC_ReadBlock",
//			static_cast<FEVInterface::frontEndMacroFunction_t>(
//					&CFOandDTCCoreVInterface::ReadROCBlock),
//				        std::vector<std::string>{"rocLinkIndex", "numberOfWords", "address", "incrementAddress"},
//					std::vector<std::string>{"readData"},
//					1);  // requiredUserPermissions
//
//
//
//	// registration of FEMacro 'DTCStatus' generated, Oct-22-2018 03:16:46, by
//	// 'admin' using MacroMaker.
//	registerFEMacroFunction(
//			"ROC_Write",  // feMacroName
//			static_cast<FEVInterface::frontEndMacroFunction_t>(
//					&CFOandDTCCoreVInterface::WriteROC),  // feMacroFunction
//					std::vector<std::string>{"rocLinkIndex", "address", "writeData"},
//					std::vector<std::string>{},  // namesOfOutput
//					1);                          // requiredUserPermissions
//
//	registerFEMacroFunction(
//			"ROC_Read",
//			static_cast<FEVInterface::frontEndMacroFunction_t>(
//					&CFOandDTCCoreVInterface::ReadROC),                  // feMacroFunction
//					std::vector<std::string>{"rocLinkIndex", "address"},  // namesOfInputArgs
//					std::vector<std::string>{"readData"},
//					1);  // requiredUserPermissions
//
//	registerFEMacroFunction("DTC_Reset",
//			static_cast<FEVInterface::frontEndMacroFunction_t>(
//					&CFOandDTCCoreVInterface::DTCReset),
//					std::vector<std::string>{},
//					std::vector<std::string>{},
//					1);  // requiredUserPermissions
//
//	registerFEMacroFunction("DTC_HighRate_DCS_Check",
//			static_cast<FEVInterface::frontEndMacroFunction_t>(
//					&CFOandDTCCoreVInterface::DTCHighRateDCSCheck),
//					std::vector<std::string>{"rocLinkIndex","loops","baseAddress",
//						"correctRegisterValue0","correctRegisterValue1"},
//					std::vector<std::string>{},
//					1);  // requiredUserPermissions
//
//	registerFEMacroFunction("DTC_HighRate_DCS_Block_Check",
//			static_cast<FEVInterface::frontEndMacroFunction_t>(
//					&CFOandDTCCoreVInterface::DTCHighRateBlockCheck),
//					std::vector<std::string>{"rocLinkIndex","loops","baseAddress",
//						"correctRegisterValue0","correctRegisterValue1"},
//					std::vector<std::string>{},
//					1);  // requiredUserPermissions
//
//	registerFEMacroFunction("DTC_SendHeartbeatAndDataRequest",
//			static_cast<FEVInterface::frontEndMacroFunction_t>(
//					&CFOandDTCCoreVInterface::DTCSendHeartbeatAndDataRequest),
//					std::vector<std::string>{"numberOfRequests","timestampStart"},
//					std::vector<std::string>{},
//					1);  // requiredUserPermissions
//
//
//	{ //add ROC FE Macros
//		__FE_COUT__ << "Getting children ROC FEMacros..." << __E__;
//		rocFEMacroMap_.clear();
//		for(auto& roc : rocs_)
//		{
//			auto feMacros = roc.second->getMapOfFEMacroFunctions();
//			for(auto& feMacro:feMacros)
//			{
//				__FE_COUT__ << roc.first << "::" << feMacro.first << __E__;
//
//				//make DTC FEMacro forwarding to ROC FEMacro
//				std::string macroName =
//						"Link" +
//						std::to_string(roc.second->getLinkID()) +
//						"_" + roc.first + "_" +
//						feMacro.first;
//				__FE_COUTV__(macroName);
//
//				std::vector<std::string> inputArgs,outputArgs;
//
//				//inputParams.push_back("ROC_UID");
//				//inputParams.push_back("ROC_FEMacroName");
//				for(auto& inArg: feMacro.second.namesOfInputArguments_)
//					inputArgs.push_back(inArg);
//				for(auto& outArg: feMacro.second.namesOfOutputArguments_)
//					outputArgs.push_back(outArg);
//
//				__FE_COUTV__(StringMacros::vectorToString(inputArgs));
//				__FE_COUTV__(StringMacros::vectorToString(outputArgs));
//
//				rocFEMacroMap_.emplace(std::make_pair(macroName,
//						std::make_pair(roc.first,feMacro.first)));
//
//				registerFEMacroFunction(macroName,
//						static_cast<FEVInterface::frontEndMacroFunction_t>(
//								&CFOandDTCCoreVInterface::RunROCFEMacro),
//								inputArgs,
//								outputArgs,
//								1);  // requiredUserPermissions
//			}
//		}
//	} //end add ROC FE Macros
//
//	// clang-format on
//
//}  // end registerFEMacros()
//
////========================================================================================================================
//void CFOandDTCCoreVInterface::createROCs(void)
//{
//	rocs_.clear();
//
//	std::vector<std::pair<std::string, ConfigurationTree>> rocChildren =
//	    Configurable::getSelfNode().getNode("LinkToROCGroupTable").getChildren();
//
//	// instantiate vector of ROCs
//	for(auto& roc : rocChildren)
//		if(roc.second.getNode("Status").getValue<bool>())
//		{
//			__FE_COUT__
//			    << "ROC Plugin Name: "
//			    << roc.second.getNode("ROCInterfacePluginName").getValue<std::string>()
//			    << std::endl;
//			__FE_COUT__ << "ROC Name: " << roc.first << std::endl;
//
//			try
//			{
//				__COUTV__(theXDAQContextConfigTree_.getValueAsString());
//				__COUTV__(
//				    roc.second.getNode("ROCInterfacePluginName").getValue<std::string>());
//
//				// Note: FEVInterface makeInterface returns a unique_ptr
//				//	and we want to verify that ROCCoreVInterface functionality
//				//	is there, so we do an intermediate dynamic_cast to check
//				//	before placing in a new unique_ptr of type ROCCoreVInterface.
//				std::unique_ptr<FEVInterface> tmpVFE = makeInterface(
//				    roc.second.getNode("ROCInterfacePluginName").getValue<std::string>(),
//				    roc.first,
//				    theXDAQContextConfigTree_,
//				    (theConfigurationPath_ + "/LinkToROCGroupTable/" + roc.first));
//
//				// setup parent supervisor of FEVinterface (for backwards compatibility,
//				// left out of constructor)
//				tmpVFE->parentSupervisor_ = parentSupervisor_;
//
//				ROCCoreVInterface& tmpRoc = dynamic_cast<ROCCoreVInterface&>(
//				    *tmpVFE);  // dynamic_cast<ROCCoreVInterface*>(tmpRoc.get());
//
//				// setup other members of ROCCore (for interface plug-in compatibility,
//				// left out of constructor)
//
//				__COUTV__(tmpRoc.emulatorMode_);
//				tmpRoc.emulatorMode_ = emulatorMode_;
//				__COUTV__(tmpRoc.emulatorMode_);
//
//				if(emulatorMode_)
//				{
//					__FE_COUT__ << "Creating ROC in emulator mode..." << __E__;
//
//					// try
//					{
//						// all ROCs support emulator mode
//
//						//						// verify ROCCoreVEmulator class
//						// functionality  with  dynamic_cast
//						// ROCCoreVEmulator&  tmpEmulator =
//						// dynamic_cast<ROCCoreVEmulator&>(
//						//						    tmpRoc);  //
//						// dynamic_cast<ROCCoreVInterface*>(tmpRoc.get());
//
//						// start emulator thread
//						std::thread(
//						    [](ROCCoreVInterface* rocEmulator) {
//							    __COUT__ << "Starting ROC emulator thread..." << __E__;
//							    ROCCoreVInterface::emulatorThread(rocEmulator);
//						    },
//						    &tmpRoc)
//						    .detach();
//					}
//					//					catch(const std::bad_cast& e)
//					//					{
//					//						__SS__ << "Cast to ROCCoreVEmulator failed!
//					// Verify  the  emulator " 						          "plugin
//					// inherits
//					// from  ROCCoreVEmulator."
//					//						       << __E__;
//					//						ss << "Failed to instantiate plugin named '"
//					//<<  roc.first
//					//						   << "' of type '"
//					//						   <<
//					// roc.second.getNode("ROCInterfacePluginName")
//					//						          .getValue<std::string>()
//					//						   << "' due to the following error: \n"
//					//						   << e.what() << __E__;
//					//
//					//						__SS_THROW__;
//					//					}
//				}
//				else
//				{
//					tmpRoc.thisDTC_ = thisDTC_;
//				}
//
//				rocs_.emplace(std::pair<std::string, std::unique_ptr<ROCCoreVInterface>>(
//				    roc.first, &tmpRoc));
//				tmpVFE.release();  // release the FEVInterface unique_ptr, so we are left
//				                   // with just one
//
//				__COUTV__(rocs_[roc.first]->emulatorMode_);
//			}
//			catch(const cet::exception& e)
//			{
//				__SS__ << "Failed to instantiate plugin named '" << roc.first
//				       << "' of type '"
//				       << roc.second.getNode("ROCInterfacePluginName")
//				              .getValue<std::string>()
//				       << "' due to the following error: \n"
//				       << e.what() << __E__;
//				__FE_COUT_ERR__ << ss.str();
//				__MOUT_ERR__ << ss.str();
//				__SS_ONLY_THROW__;
//			}
//			catch(const std::bad_cast& e)
//			{
//				__SS__ << "Cast to ROCCoreVInterface failed! Verify the plugin inherits "
//				          "from ROCCoreVInterface."
//				       << __E__;
//				ss << "Failed to instantiate plugin named '" << roc.first << "' of type '"
//				   << roc.second.getNode("ROCInterfacePluginName").getValue<std::string>()
//				   << "' due to the following error: \n"
//				   << e.what() << __E__;
//
//				__FE_COUT_ERR__ << ss.str();
//				__MOUT_ERR__ << ss.str();
//
//				__SS_ONLY_THROW__;
//			}
//		}
//
//	__FE_COUT__ << "Done creating " << rocs_.size() << " ROC(s)" << std::endl;
//
//}  // end createROCs

//==========================================================================================
// universalRead
//	Must implement this function for Macro Maker to work with this
// interface. 	When Macro Maker calls:
//		- address will be a [universalAddressSize_] byte long char array
//		- returnValue will be a [universalDataSize_] byte long char
// array
//		- expects exception thrown on failure/timeout
void CFOandDTCCoreVInterface::universalRead(char* address, char* returnValue)
{
	// __FE_COUT__ << "DTC READ" << __E__;

	if(emulatorMode_)
	{
		__FE_COUT__ << "Emulator read " << __E__;
		for(unsigned int i = 0; i < universalDataSize_; ++i)
			returnValue[i] = 0xF0 | i;
		return;
	}
	
	(*((dtc_address_t*)returnValue)) = registerRead(*((dtc_address_t*)address));
	
	// __COUTV__(reg_access_.val);
	
} //end universalRead()

//===========================================================================================
// registerRead: return = value read from register at address "address"
//
dtc_data_t CFOandDTCCoreVInterface::registerRead(const dtc_address_t address)
{	
	reg_access_.access_type = 0;  // 0 = read, 1 = write
	reg_access_.reg_offset = address;
	// __COUTV__(reg_access.reg_offset);

	if(ioctl(fd_, M_IOC_REG_ACCESS, &reg_access_))
	{
		__SS__ << "ERROR: DTC register read - Does file exist? -> /dev/mu2e" << dtc_
		       << __E__;
		__SS_THROW__;
	}

	return reg_access_.val;
	
} // end registerRead()
	
//	
//	uint8_t* addrs = new uint8_t[universalAddressSize_];  // create address buffer
//	                                                      // of interface size
//	uint8_t* data =
//	    new uint8_t[universalDataSize_];  // create data buffer of interface size
//
//	uint8_t macroAddrs[20] =
//	    {};  // total hack, assuming we'll never have 200 bytes in an address
//
//	// fill byte-by-byte
//	for(unsigned int i = 0; i < universalAddressSize_; i++)
//		macroAddrs[i] = 0xff & (address >> i * 8);
//
//	// 0-pad
//	for(unsigned int i = 0; i < universalAddressSize_; ++i)
//		addrs[i] = (i < 2) ? macroAddrs[i] : 0;
//
//	universalRead((char*)addrs, (char*)data);
//
//	unsigned int readvalue = 0x00000000;
//
//	// unpack byte-by-byte
//	for(uint8_t i = universalDataSize_; i > 0; i--)
//		readvalue = (readvalue << 8 & 0xffffff00) | data[i - 1];
//
//	// __FE_COUT__ << "DTC: readvalue register 0x" << std::hex << address
//	//	<< " is..." << std::hex << readvalue << __E__;
//
//	delete[] addrs;  // free the memory
//	delete[] data;   // free the memory
//
//	return readvalue;
//}

//=====================================================================================
// universalWrite
//	Must implement this function for Macro Maker to work with this
// interface. 	When Macro Maker calls:
//		- address will be a [universalAddressSize_] byte long char array
//		- writeValue will be a [universalDataSize_] byte long char array
void CFOandDTCCoreVInterface::universalWrite(char* address, char* writeValue)
{
	// __FE_COUT__ << "DTC WRITE" << __E__;
	if(emulatorMode_)
	{
		__FE_COUT__ << "Emulator write " << __E__;
		return;
	}
	
	registerWrite(*((dtc_address_t*)address),*((dtc_data_t*) writeValue));
} //end universalWrite()

//===============================================================================================
// registerWrite: return = value readback from register at address "address"
//
dtc_data_t CFOandDTCCoreVInterface::registerWrite(const dtc_address_t address, dtc_data_t dataToWrite)
{
	reg_access_.access_type = 1;  // 0 = read, 1 = write
	reg_access_.reg_offset = address;
	// __COUTV__(reg_access.reg_offset);
	reg_access_.val = dataToWrite;
	// __COUTV__(reg_access.val);

	if(ioctl(fd_, M_IOC_REG_ACCESS, &reg_access_))
		__FE_COUT_ERR__ << "ERROR: DTC universal write - Does file exist? /dev/mu2e"
		                << dtc_ << __E__;
	//return registerRead(address);


	dtc_data_t readbackValue = registerRead(address);

	int i = 0;

	// this is an I2C register, it clears bit0 when transaction finishes
	if((address == 0x916c) && ((dataToWrite & 0x1) == 1))
	{
		// wait for I2C to clear...
		while((readbackValue & 0x1) != 0)
		{
			i++;
			readbackValue = registerRead(address);
			usleep(100);
			if((i % 10) == 0)
				__FE_COUT__ << "DTC I2C waited " << i << " times..." << __E__;
		}
		// if (i > 0) __FE_COUT__ << "DTC I2C waited " << i << " times..." << __E__;
	}

	// lowest 8-bits are the I2C read value. But we aren't reading anything back
	// for the moment...
	if(address == 0x9168)
	{
		if((readbackValue & 0xffffff00) != (dataToWrite & 0xffffff00))
		{
			__FE_COUT_ERR__ << "DTC: write value 0x" << std::hex << dataToWrite
					<< " to register 0x" << std::hex << address
					<< "... read back 0x" << std::hex << readbackValue << __E__;
		}
	}

	// if it is not 0x9168 or 0x916c, make sure read = write
	if(readbackValue != dataToWrite && address != 0x9168 && address != 0x916c)
	{
		__FE_COUT_ERR__ << "DTC: write value 0x" << std::hex << dataToWrite
				<< " to register 0x" << std::hex << address << "... read back 0x"
				<< std::hex << readbackValue << __E__;
	}

	return readbackValue;
} //end registerWrite()
//	uint8_t* addrs = new uint8_t[universalAddressSize_];  // create address buffer
//	                                                      // of interface size
//	uint8_t* data =
//	    new uint8_t[universalDataSize_];  // create data buffer of interface size
//
//	uint8_t macroAddrs[20] = {};  // assume we'll never have 20 bytes in an address
//	uint8_t macroData[20] =
//	    {};  // assume we'll never have 20 bytes read out from a register
//
//	// fill byte-by-byte
//	for(unsigned int i = 0; i < universalAddressSize_; i++)
//		macroAddrs[i] = 0xff & (address >> i * 8);
//
//	// 0-pad
//	for(unsigned int i = 0; i < universalAddressSize_; ++i)
//		addrs[i] = (i < 2) ? macroAddrs[i] : 0;
//
//	// fill byte-by-byte
//	for(unsigned int i = 0; i < universalDataSize_; i++)
//		macroData[i] = 0xff & (dataToWrite >> i * 8);
//
//	// 0-pad
//	for(unsigned int i = 0; i < universalDataSize_; ++i)
//		data[i] = (i < 4) ? macroData[i] : 0;
//
//	universalWrite((char*)addrs, (char*)data);
//
//	// usleep(100);
//
//	int readbackValue = registerRead(address);
//
//	int i = 0;
//
//	// this is an I2C register, it clears bit0 when transaction finishes
//	if((address == 0x916c) && ((dataToWrite & 0x1) == 1))
//	{
//		// wait for I2C to clear...
//		while((readbackValue & 0x1) != 0)
//		{
//			i++;
//			readbackValue = registerRead(address);
//			usleep(100);
//			if((i % 10) == 0)
//				__FE_COUT__ << "DTC I2C waited " << i << " times..." << __E__;
//		}
//		// if (i > 0) __FE_COUT__ << "DTC I2C waited " << i << " times..." << __E__;
//	}
//
//	// lowest 8-bits are the I2C read value. But we aren't reading anything back
//	// for the moment...
//	if(address == 0x9168)
//	{
//		if((readbackValue & 0xffffff00) != (dataToWrite & 0xffffff00))
//		{
//			__FE_COUT_ERR__ << "DTC: write value 0x" << std::hex << dataToWrite
//			                << " to register 0x" << std::hex << address
//			                << "... read back 0x" << std::hex << readbackValue << __E__;
//		}
//	}
//
//	// if it is not 0x9168 or 0x916c, make sure read = write
//	if(readbackValue != dataToWrite && address != 0x9168 && address != 0x916c)
//	{
//		__FE_COUT_ERR__ << "DTC: write value 0x" << std::hex << dataToWrite
//		                << " to register 0x" << std::hex << address << "... read back 0x"
//		                << std::hex << readbackValue << __E__;
//	}
//
//	delete[] addrs;  // free the memory
//	delete[] data;   // free the memory
//
//	return readbackValue;
//}
//
////==================================================================================================
//void CFOandDTCCoreVInterface::readStatus(void)
//{
//	__FE_COUT__ << device_name_ << " firmware version (0x9004) = 0x" << std::hex
//	            << registerRead(0x9004) << __E__;
//
//	printVoltages();
//
//	__FE_COUT__ << device_name_ << " temperature = " << readTemperature() << " degC"
//	            << __E__;
//
//	__FE_COUT__ << device_name_ << " SERDES reset........ (0x9118) = 0x" << std::hex
//	            << registerRead(0x9118) << __E__;
//	__FE_COUT__ << device_name_ << " SERDES disparity err (0x911c) = 0x" << std::hex
//	            << registerRead(0x9118) << __E__;
//	__FE_COUT__ << device_name_ << " SERDES unlock error. (0x9124) = 0x" << std::hex
//	            << registerRead(0x9124) << __E__;
//	__FE_COUT__ << device_name_ << " PLL locked.......... (0x9128) = 0x" << std::hex
//	            << registerRead(0x9128) << __E__;
//	__FE_COUT__ << device_name_ << " SERDES Rx status.... (0x9134) = 0x" << std::hex
//	            << registerRead(0x9134) << __E__;
//	__FE_COUT__ << device_name_ << " SERDES reset done... (0x9138) = 0x" << std::hex
//	            << registerRead(0x9138) << __E__;
//	__FE_COUT__ << device_name_ << " link status......... (0x9140) = 0x" << std::hex
//	            << registerRead(0x9140) << __E__;
//	__FE_COUT__ << device_name_ << " SERDES ref clk freq. (0x915c) = 0x" << std::hex
//	            << registerRead(0x915c) << " = " << std::dec << registerRead(0x915c)
//	            << __E__;
//	__FE_COUT__ << device_name_ << " control............. (0x9100) = 0x" << std::hex
//	            << registerRead(0x9100) << __E__;
//	__FE_COUT__ << __E__;
//
//	return;
//}

//==================================================================================================
float CFOandDTCCoreVInterface::readTemperature()
{
	int tempadc = registerRead(0x9010);

	float temperature = ((tempadc * 503.975) / 4096.) - 273.15;

	return temperature;
}

//==================================================================================================
// turn on LED on front of timing card
//
void CFOandDTCCoreVInterface::turnOnLED()
{
	int dataInReg   = registerRead(0x9100);
	int dataToWrite = dataInReg | 0x001f0000;  // bit[16-20] = 1
	registerWrite(0x9100, dataToWrite);

	return;
}

//==================================================================================================
// turn off LED on front of timing card
//
void CFOandDTCCoreVInterface::turnOffLED()
{
	int dataInReg   = registerRead(0x9100);
	int dataToWrite = dataInReg & 0xffe0ffff;  // bit[16-20] = 0
	registerWrite(0x9100, dataToWrite);

	return;
}
//
////==================================================================================================
//int CFOandDTCCoreVInterface::getROCLinkStatus(int ROC_link)
//{
//	int overall_link_status = registerRead(0x9140);
//
//	int ROC_link_status = (overall_link_status >> ROC_link) & 0x1;
//
//	return ROC_link_status;
//}
//
//int CFOandDTCCoreVInterface::getCFOLinkStatus()
//{
//	int overall_link_status = registerRead(0x9140);
//
//	int CFO_link_status = (overall_link_status >> 6) & 0x1;
//
//	return CFO_link_status;
//}

void CFOandDTCCoreVInterface::printVoltages()
{
	int adc_vccint  = registerRead(0x9014);
	int adc_vccaux  = registerRead(0x9018);
	int adc_vccbram = registerRead(0x901c);

	float volt_vccint  = ((float)adc_vccint / 4095.) * 3.0;
	float volt_vccaux  = ((float)adc_vccaux / 4095.) * 3.0;
	float volt_vccbram = ((float)adc_vccbram / 4095.) * 3.0;

	__FE_COUT__ << device_name_ << " VCCINT. = " << volt_vccint << " V" << __E__;
	__FE_COUT__ << device_name_ << " VCCAUX. = " << volt_vccaux << " V" << __E__;
	__FE_COUT__ << device_name_ << " VCCBRAM = " << volt_vccbram << " V" << __E__;

	return;
}
//
//int CFOandDTCCoreVInterface::checkLinkStatus()
//{
//	int ROCs_OK = 1;
//
//	for(int i = 0; i < 8; i++)
//	{
//		//__FE_COUT__ << " check link " << i << " ";
//
//		if(ROCActive(i))
//		{
//			//__FE_COUT__ << " active... status = " << getROCLinkStatus(i) << __E__;
//			ROCs_OK &= getROCLinkStatus(i);
//		}
//	}
//
//	if((getCFOLinkStatus() == 1) && ROCs_OK == 1)
//	{
//		//	__FE_COUT__ << "DTC links OK = 0x" << std::hex << registerRead(0x9140)
//		//<< std::dec << __E__;
//		//	__MOUT__ << "DTC links OK = 0x" << std::hex << registerRead(0x9140) <<
//		// std::dec << __E__;
//
//		return 1;
//	}
//	else
//	{
//		//	__FE_COUT__ << "DTC links not OK = 0x" << std::hex <<
//		// registerRead(0x9140) << std::dec << __E__;
//		//	__MOUT__ << "DTC links not OK = 0x" << std::hex << registerRead(0x9140)
//		//<< std::dec << __E__;
//
//		return 0;
//	}
//}
//
//bool CFOandDTCCoreVInterface::ROCActive(unsigned ROC_link)
//{
//	// __FE_COUTV__(roc_mask_);
//	// __FE_COUTV__(ROC_link);
//
//	if(((roc_mask_ >> ROC_link) & 0x01) == 1)
//	{
//		return true;
//	}
//	else
//	{
//		return false;
//	}
//}
//
////==================================================================================================
//void CFOandDTCCoreVInterface::configure(void) try
//{
//	__FE_COUTV__(getIterationIndex());
//	__FE_COUTV__(getSubIterationIndex());
//
//}  // end configure()
//catch(const std::runtime_error& e)
//{
//	__FE_SS__ << "Error caught: " << e.what() << __E__;
//	__FE_SS_THROW__;
//}
//catch(...)
//{
//	__FE_SS__ << "Unknown error caught. Check the printouts!" << __E__;
//	__FE_SS_THROW__;
//}
//
////========================================================================================================================
//void CFOandDTCCoreVInterface::halt(void)
//{
//}
//
////========================================================================================================================
//void CFOandDTCCoreVInterface::pause(void)
//{
//}
//
////========================================================================================================================
//void CFOandDTCCoreVInterface::resume(void)
//{
//}
//
////========================================================================================================================
//void CFOandDTCCoreVInterface::start(std::string runNumber)
//{
//}
//
////========================================================================================================================
//void CFOandDTCCoreVInterface::stop(void)
//{
//}
//
////========================================================================================================================
//bool CFOandDTCCoreVInterface::running(void)
//{
//	return true; //true to end loop
//}

//=====================================
void CFOandDTCCoreVInterface::configureJitterAttenuator(void)
{
	// Start configuration preamble
	// set page B
	registerWrite(0x9168, 0x68010B00);
	registerWrite(0x916c, 0x00000001);

	// page B registers
	registerWrite(0x9168, 0x6824C000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68250000);
	registerWrite(0x916c, 0x00000001);

	// set page 5
	registerWrite(0x9168, 0x68010500);
	registerWrite(0x916c, 0x00000001);

	// page 5 registers
	registerWrite(0x9168, 0x68400100);
	registerWrite(0x916c, 0x00000001);

	// End configuration preamble
	//
	// Delay 300 msec
	// Delay is worst case time for device to complete any calibration
	// that is running due to device state change previous to this script
	// being processed.
	//
	// Start configuration registers
	// set page 0
	registerWrite(0x9168, 0x68010000);
	registerWrite(0x916c, 0x00000001);

	// page 0 registers
	registerWrite(0x9168, 0x68060000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68070000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68080000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680B6800);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68160200);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x6817DC00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68180000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x6819DD00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x681ADF00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682B0200);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682C0F00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682D5500);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682E3700);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682F0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68303700);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68310000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68323700);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68330000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68343700);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68350000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68363700);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68370000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68383700);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68390000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683A3700);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683B0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683C3700);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683D0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683FFF00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68400400);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68410E00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68420E00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68430E00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68440E00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68450C00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68463200);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68473200);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68483200);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68493200);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x684A3200);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x684B3200);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x684C3200);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x684D3200);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x684E5500);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x684F5500);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68500F00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68510300);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68520300);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68530300);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68540300);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68550300);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68560300);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68570300);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68580300);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68595500);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x685AAA00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x685BAA00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x685C0A00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x685D0100);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x685EAA00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x685FAA00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68600A00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68610100);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x6862AA00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x6863AA00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68640A00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68650100);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x6866AA00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x6867AA00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68680A00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68690100);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68920200);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x6893A000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68950000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68968000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68986000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x689A0200);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x689B6000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x689D0800);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x689E4000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68A02000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68A20000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68A98A00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68AA6100);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68AB0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68AC0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68E52100);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68EA0A00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68EB6000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68EC0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68ED0000);
	registerWrite(0x916c, 0x00000001);

	// set page 1
	registerWrite(0x9168, 0x68010100);
	registerWrite(0x916c, 0x00000001);

	// page 1 registers
	registerWrite(0x9168, 0x68020100);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68120600);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68130900);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68143B00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68152800);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68170600);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68180900);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68193B00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x681A2800);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683F1000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68400000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68414000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x6842FF00);
	registerWrite(0x916c, 0x00000001);

	// set page 2
	registerWrite(0x9168, 0x68010200);
	registerWrite(0x916c, 0x00000001);

	// page 2 registers
	registerWrite(0x9168, 0x68060000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68086400);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68090000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680A0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680B0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680C0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680D0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680E0100);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680F0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68100000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68110000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68126400);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68130000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68140000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68150000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68160000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68170000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68180100);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68190000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x681A0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x681B0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x681C6400);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x681D0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x681E0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x681F0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68200000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68210000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68220100);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68230000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68240000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68250000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68266400);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68270000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68280000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68290000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682A0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682B0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682C0100);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682D0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682E0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682F0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68310B00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68320B00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68330B00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68340B00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68350000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68360000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68370000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68388000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x6839D400);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683A0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683B0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683C0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683D0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683EC000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68500000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68510000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68520000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68530000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68540000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68550000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x686B5200);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x686C6500);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x686D7600);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x686E3100);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x686F2000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68702000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68712000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68722000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x688A0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x688B0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x688C0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x688D0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x688E0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x688F0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68900000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68910000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x6894B000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68960200);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68970200);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68990200);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x689DFA00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x689E0100);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x689F0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68A9CC00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68AA0400);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68AB0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68B7FF00);
	registerWrite(0x916c, 0x00000001);

	// set page 3
	registerWrite(0x9168, 0x68010300);
	registerWrite(0x916c, 0x00000001);

	// page 3 registers
	registerWrite(0x9168, 0x68020000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68030000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68040000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68050000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68061100);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68070000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68080000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68090000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680A0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680B8000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680C0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680D0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680E0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680F0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68100000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68110000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68120000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68130000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68140000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68150000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68160000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68170000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68380000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68391F00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683B0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683C0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683D0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683E0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683F0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68400000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68410000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68420000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68430000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68440000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68450000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68460000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68590000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x685A0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x685B0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x685C0000);
	registerWrite(0x916c, 0x00000001);

	// set page 4
	registerWrite(0x9168, 0x68010400);
	registerWrite(0x916c, 0x00000001);

	// page 4 registers
	registerWrite(0x9168, 0x68870100);
	registerWrite(0x916c, 0x00000001);

	// set page 5
	registerWrite(0x9168, 0x68010500);
	registerWrite(0x916c, 0x00000001);

	// page 5 registers
	registerWrite(0x9168, 0x68081000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68091F00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680A0C00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680B0B00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680C3F00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680D3F00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680E1300);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680F2700);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68100900);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68110800);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68123F00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68133F00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68150000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68160000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68170000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68180000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x6819A800);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x681A0200);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x681B0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x681C0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x681D0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x681E0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x681F8000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68212B00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682A0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682B0100);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682C8700);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682D0300);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682E1900);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682F1900);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68310000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68324200);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68330300);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68340000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68350000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68360000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68370000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68380000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68390000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683A0200);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683B0300);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683C0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683D1100);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683E0600);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68890D00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x688A0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x689BFA00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x689D1000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x689E2100);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x689F0C00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68A00B00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68A13F00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68A23F00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68A60300);
	registerWrite(0x916c, 0x00000001);

	// set page 8
	registerWrite(0x9168, 0x68010800);
	registerWrite(0x916c, 0x00000001);

	// page 8 registers
	registerWrite(0x9168, 0x68023500);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68030500);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68040000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68050000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68060000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68070000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68080000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68090000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680A0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680B0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680C0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680D0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680E0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x680F0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68100000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68110000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68120000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68130000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68140000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68150000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68160000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68170000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68180000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68190000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x681A0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x681B0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x681C0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x681D0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x681E0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x681F0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68200000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68210000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68220000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68230000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68240000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68250000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68260000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68270000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68280000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68290000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682A0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682B0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682C0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682D0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682E0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x682F0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68300000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68310000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68320000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68330000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68340000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68350000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68360000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68370000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68380000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68390000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683A0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683B0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683C0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683D0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683E0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x683F0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68400000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68410000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68420000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68430000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68440000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68450000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68460000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68470000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68480000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68490000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x684A0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x684B0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x684C0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x684D0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x684E0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x684F0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68500000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68510000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68520000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68530000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68540000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68550000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68560000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68570000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68580000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68590000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x685A0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x685B0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x685C0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x685D0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x685E0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x685F0000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68600000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68610000);
	registerWrite(0x916c, 0x00000001);

	// set page 9
	registerWrite(0x9168, 0x68010900);
	registerWrite(0x916c, 0x00000001);

	// page 9 registers
	registerWrite(0x9168, 0x680E0200);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68430100);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68490F00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x684A0F00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x684E4900);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x684F0200);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x685E0000);
	registerWrite(0x916c, 0x00000001);

	// set page A
	registerWrite(0x9168, 0x68010A00);
	registerWrite(0x916c, 0x00000001);

	// page A registers
	registerWrite(0x9168, 0x68020000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68030100);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68040100);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68050100);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68140000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x681A0000);
	registerWrite(0x916c, 0x00000001);

	// set page B
	registerWrite(0x9168, 0x68010B00);
	registerWrite(0x916c, 0x00000001);

	// page B registers
	registerWrite(0x9168, 0x68442F00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68460000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68470000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68480000);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x684A0200);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68570E00);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68580100);
	registerWrite(0x916c, 0x00000001);

	// End configuration registers
	//
	// Start configuration postamble
	// set page 5
	registerWrite(0x9168, 0x68010500);
	registerWrite(0x916c, 0x00000001);

	// page 5 registers
	registerWrite(0x9168, 0x68140100);
	registerWrite(0x916c, 0x00000001);

	// set page 0
	registerWrite(0x9168, 0x68010000);
	registerWrite(0x916c, 0x00000001);

	// page 0 registers
	registerWrite(0x9168, 0x681C0100);
	registerWrite(0x916c, 0x00000001);

	// set page 5
	registerWrite(0x9168, 0x68010500);
	registerWrite(0x916c, 0x00000001);

	// page 5 registers
	registerWrite(0x9168, 0x68400000);
	registerWrite(0x916c, 0x00000001);

	// set page B
	registerWrite(0x9168, 0x68010B00);
	registerWrite(0x916c, 0x00000001);

	// page B registers
	registerWrite(0x9168, 0x6824C300);
	registerWrite(0x916c, 0x00000001);

	registerWrite(0x9168, 0x68250200);
	registerWrite(0x916c, 0x00000001);

	// End configuration postamble

	return;
}
//
////========================================================================================================================
//// rocRead
//void CFOandDTCCoreVInterface::ReadROC(__ARGS__)
//{
//	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
//	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
//	for(auto& argIn : argsIn)
//		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;
//
//	DTCLib::DTC_Link_ID rocLinkIndex =
//	    DTCLib::DTC_Link_ID(__GET_ARG_IN__("rocLinkIndex", uint8_t));
//	uint8_t address = __GET_ARG_IN__("address", uint8_t);
//	__FE_COUTV__(rocLinkIndex);
//	__FE_COUTV__((unsigned int)address);
//
//	// DTCLib::DTC* tmpDTC = new
//	// DTCLib::DTC(DTCLib::DTC_SimMode_NoCFO,dtc_,roc_mask_,"");
//
//	uint16_t readData = -999;
//
//	for(auto& roc : rocs_)
//	{
//		__FE_COUT__ << "Found link ID " << roc.second->getLinkID() << " looking for "
//		            << rocLinkIndex << __E__;
//
//		if(rocLinkIndex == roc.second->getLinkID())
//		{
//			readData = roc.second->readRegister(address);
//
//			// uint16_t readData = thisDTC_->ReadROCRegister(rocLinkIndex, address);
//			// delete tmpDTC;
//
//			//  char readDataStr[100];
//			//  sprintf(readDataStr,"0x%X",readData);
//			//  __SET_ARG_OUT__("readData",readDataStr);
//			__SET_ARG_OUT__("readData", readData);
//
//			// for(auto &argOut:argsOut)
//			__FE_COUT__ << "readData"
//			            << ": " << std::hex << readData << std::dec << __E__;
//			return;
//		}
//	}
//
//	__FE_SS__ << "ROC link ID " << rocLinkIndex << " not found!" << __E__;
//	__FE_SS_THROW__;
//
//}  // end ReadROC()
//
////========================================================================================================================
//// DTCStatus
////	FEMacro 'DTCStatus' generated, Oct-22-2018 03:16:46, by 'admin' using
//// MacroMaker. 	Macro Notes:
//void CFOandDTCCoreVInterface::WriteROC(__ARGS__)
//{
//	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
//	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
//	for(auto& argIn : argsIn)
//		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;
//
//	DTCLib::DTC_Link_ID rocLinkIndex =
//	    DTCLib::DTC_Link_ID(__GET_ARG_IN__("rocLinkIndex", uint8_t));
//	uint8_t  address   = __GET_ARG_IN__("address", uint8_t);
//	uint16_t writeData = __GET_ARG_IN__("writeData", uint16_t);
//
//	__FE_COUTV__(rocLinkIndex);
//	__FE_COUTV__((unsigned int)address);
//	__FE_COUTV__(writeData);
//
//	__FE_COUT__ << "ROCs size = " << rocs_.size() << __E__;
//
//	for(auto& roc : rocs_)
//	{
//		__FE_COUT__ << "Found link ID " << roc.second->getLinkID() << " looking for "
//		            << rocLinkIndex << __E__;
//
//		if(rocLinkIndex == roc.second->getLinkID())
//		{
//			roc.second->writeRegister(address, writeData);
//
//			for(auto& argOut : argsOut)
//				__FE_COUT__ << argOut.first << ": " << argOut.second << __E__;
//
//			return;
//		}
//	}
//
//	__FE_SS__ << "ROC link ID " << rocLinkIndex << " not found!" << __E__;
//	__FE_SS_THROW__;
//}
//
////========================================================================================================================
//void CFOandDTCCoreVInterface::WriteROCBlock(__ARGS__)
//{
//	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
//	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
//	for(auto& argIn : argsIn)
//		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;
//
//	// macro commands section
//
//	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
//	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
//
//	for(auto& argIn : argsIn)
//		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;
//
//	DTCLib::DTC_Link_ID rocLinkIndex =
//	    DTCLib::DTC_Link_ID(__GET_ARG_IN__("rocLinkIndex", uint8_t));
//	uint8_t  address   = __GET_ARG_IN__("address", uint8_t);
//	uint16_t writeData = __GET_ARG_IN__("writeData", uint16_t);
//	uint8_t  block     = __GET_ARG_IN__("block", uint8_t);
//	__FE_COUTV__(rocLinkIndex);
//	__FE_COUT__ << "block = " << std::dec << (unsigned int)block << __E__;
//	__FE_COUT__ << "address = 0x" << std::hex << (unsigned int)address << std::dec
//	            << __E__;
//	__FE_COUT__ << "writeData = 0x" << std::hex << writeData << std::dec << __E__;
//
//	bool acknowledge_request = false;
//
//	thisDTC_->WriteExtROCRegister(
//	    rocLinkIndex, block, address, writeData, acknowledge_request);
//
//	for(auto& argOut : argsOut)
//		__FE_COUT__ << argOut.first << ": " << argOut.second << __E__;
//}
//
////========================================================================
//void CFOandDTCCoreVInterface::ReadROCBlock(__ARGS__)
//{
//	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
//	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
//	for(auto& argIn : argsIn)
//		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;
//
//	// macro commands section
//	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
//	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
//
//	for(auto& argIn : argsIn)
//		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;
//
//	DTCLib::DTC_Link_ID rocLinkIndex =
//	    DTCLib::DTC_Link_ID(__GET_ARG_IN__("rocLinkIndex", uint8_t));
//	uint16_t address          = __GET_ARG_IN__("address", uint16_t);
//	uint16_t wordCount  = __GET_ARG_IN__("numberOfWords", uint16_t);
//	bool    incrementAddress = __GET_ARG_IN__("incrementAddress", bool);
//
//	__FE_COUTV__(rocLinkIndex);
//	__FE_COUT__ << "address = 0x" << std::hex << (unsigned int)address << std::dec
//	            << __E__;
//	__FE_COUT__ << "numberOfWords = " << std::dec << (unsigned int)wordCount
//	            << __E__;
//	__FE_COUTV__(incrementAddress);
//
//	for(auto& roc : rocs_)
//	{
//		__FE_COUT__ << "At ROC link ID " << roc.second->getLinkID() << ", looking for "
//		            << rocLinkIndex << __E__;
//
//		if(rocLinkIndex == roc.second->getLinkID())
//		{
//			std::vector<uint16_t> readData;
//
//			roc.second->readBlock(readData, address, wordCount, incrementAddress);
//
//			std::string readDataString = "";
//			{
//				bool first = true;
//				for(const auto& data: readData)
//				{
//					if(!first) readDataString += ", ";
//					else first = false;
//					readDataString += BinaryStringMacros::binaryNumberToHexString(data);
//				}
//			}
//			//StringMacros::vectorToString(readData);
//
//			__SET_ARG_OUT__("readData", readDataString);
//
//			// for(auto &argOut:argsOut)
//			__FE_COUT__ << "readData"
//			            << ": " << readDataString << __E__;
//			return;
//		}
//	}
//
//	__FE_SS__ << "ROC link ID " << rocLinkIndex << " not found!" << __E__;
//	__FE_SS_THROW__;
//
//
//
//
//
//
////	//	uint16_t readData = thisDTC_->ReadExtROCRegister(rocLinkIndex, block, address);
////
////	std::vector<uint16_t> readData;
////	thisDTC_->ReadROCBlock(
////	    readData, rocLinkIndex, address, number_of_words, incrementAddress);
////
////	std::ofstream datafile;
////
////	std::stringstream filename;
////	filename << "/home/mu2etrk/test_stand/ots/ReadROCBlock_data.txt";
////	std::string filenamestring = filename.str();
////	datafile.open(filenamestring);
////
////	datafile << "link " << std::dec << rocLinkIndex << std::endl;
////	datafile << "address 0x" << std::hex << address << std::endl;
////	datafile << "increment address " << std::dec << incrementAddress << std::endl;
////	datafile << "read " << std::dec << number_of_words << " words..." << std::endl;
////
////	for(int i = 0; i < number_of_words; i++)
////	{
////		datafile << "read data [" << std::dec << i << "]  = 0x" << std::hex << readData[i]
////		         << std::endl;
////		__FE_COUT__ << "read data [" << std::dec << i << "]  = 0x" << std::hex
////		            << readData[i] << __E__;
////	}
////
////	datafile.close();
//
//	//__SET_ARG_OUT__("readData", readData);
//
//	// for(auto& argOut : argsOut)
//	//  __FE_COUT__ << argOut.first << ": " << argOut.second << __E__;
//}  // end ReadROCBlock()
//
////========================================================================
//void CFOandDTCCoreVInterface::DTCHighRateBlockCheck(__ARGS__)
//{
//	unsigned int linkIndex   = __GET_ARG_IN__("rocLinkIndex", unsigned int);
//	unsigned int loops       = __GET_ARG_IN__("loops", unsigned int);
//	unsigned int baseAddress = __GET_ARG_IN__("baseAddress", unsigned int);
//	unsigned int correctRegisterValue0 =
//	    __GET_ARG_IN__("correctRegisterValue0", unsigned int);
//	unsigned int correctRegisterValue1 =
//	    __GET_ARG_IN__("correctRegisterValue1", unsigned int);
//
//	__FE_COUTV__(linkIndex);
//	__FE_COUTV__(loops);
//	__FE_COUTV__(baseAddress);
//	__FE_COUTV__(correctRegisterValue0);
//	__FE_COUTV__(correctRegisterValue1);
//
//	for(auto& roc : rocs_)
//		if(roc.second->getLinkID() == linkIndex)
//		{
//			roc.second->highRateBlockCheck(
//			    loops, baseAddress, correctRegisterValue0, correctRegisterValue1);
//			return;
//		}
//
//	__FE_SS__ << "Error! Could not find ROC at link index " << linkIndex << __E__;
//	__FE_SS_THROW__;
//
//}  // end DTCHighRateBlockCheck()
//
////========================================================================
//void CFOandDTCCoreVInterface::DTCHighRateDCSCheck(__ARGS__)
//{
//	unsigned int linkIndex   = __GET_ARG_IN__("rocLinkIndex", unsigned int);
//	unsigned int loops       = __GET_ARG_IN__("loops", unsigned int);
//	unsigned int baseAddress = __GET_ARG_IN__("baseAddress", unsigned int);
//	unsigned int correctRegisterValue0 =
//	    __GET_ARG_IN__("correctRegisterValue0", unsigned int);
//	unsigned int correctRegisterValue1 =
//	    __GET_ARG_IN__("correctRegisterValue1", unsigned int);
//
//	__FE_COUTV__(linkIndex);
//	__FE_COUTV__(loops);
//	__FE_COUTV__(baseAddress);
//	__FE_COUTV__(correctRegisterValue0);
//	__FE_COUTV__(correctRegisterValue1);
//
//	for(auto& roc : rocs_)
//		if(roc.second->getLinkID() == linkIndex)
//		{
//			roc.second->highRateCheck(
//			    loops, baseAddress, correctRegisterValue0, correctRegisterValue1);
//			return;
//		}
//
//	__FE_SS__ << "Error! Could not find ROC at link index " << linkIndex << __E__;
//	__FE_SS_THROW__;
//
//}  // end DTCHighRateDCSCheck()
//
////========================================================================
//void CFOandDTCCoreVInterface::DTCSendHeartbeatAndDataRequest(__ARGS__)
//{
//	unsigned int number         = __GET_ARG_IN__("numberOfRequests", unsigned int);
//	unsigned int timestampStart = __GET_ARG_IN__("timestampStart", unsigned int);
//
//	//	auto start = DTCLib::DTC_Timestamp(static_cast<uint64_t>(timestampStart));
//
//	bool     incrementTimestamp = true;
//	uint32_t cfodelay = 10000;  // have no idea what this is, but 1000 didn't work (don't
//	                            // know if 10000 works, either)
//	int requestsAhead = 0;
//
//	__FE_COUTV__(number);
//	__FE_COUTV__(timestampStart);
//
//	auto device = thisDTC_->GetDevice();
//
//	auto initTime = device->GetDeviceTime();
//	device->ResetDeviceTime();
//	auto afterInit = std::chrono::steady_clock::now();
//
//	if(emulate_cfo_ == 1)
//	{
//		registerWrite(0x9100, 0x40008004);  // bit 30 = CFO emulation enable, bit 15 = CFO
//		                                    // emulation mode, bit 2 = DCS enable
//		sleep(1);
//
//		// set number of null heartbeats
//		registerWrite(0x91BC, 0x0);
//		//	  sleep(1);
//
//		//# Send data
//		//#disable 40mhz marker
//		registerWrite(0x91f4, 0x0);
//		//	  sleep(1);
//
//		//#set num dtcs
//		registerWrite(0x9158, 0x1);
//		//	  sleep(1);
//
//		bool     useCFOEmulator   = true;
//		uint16_t debugPacketCount = 0;
//		auto     debugType        = DTCLib::DTC_DebugType_SpecialSequence;
//		bool     stickyDebugType  = true;
//		bool     quiet            = false;
//		bool     asyncRR          = false;
//		bool     forceNoDebugMode = true;
//
//		//	  std::cout << "DTCSoftwareCFO arguments..." << std::endl;
//		//	  std::cout << "useCFOEmulator = "  << useCFOEmulator << std::endl;
//		//	  std::cout << "packetCount = "     << debugPacketCount << std::endl;
//		//	  std::cout << "debugType = "       << debugType << std::endl;
//		//	  std::cout << "stickyDebugType = " << stickyDebugType << std::endl;
//		//	  std::cout << "quiet = "           << quiet << std::endl;
//		//	  std::cout << "asyncRR = "           << asyncRR << std::endl;
//		//	  std::cout << "forceNoDebug = "     << forceNoDebugMode << std::endl;
//		//	  std::cout << "END END DTCSoftwareCFO arguments..." << std::endl;
//
//		DTCLib::DTCSoftwareCFO* EmulatedCFO_ =
//		    new DTCLib::DTCSoftwareCFO(thisDTC_,
//		                               useCFOEmulator,
//		                               debugPacketCount,
//		                               debugType,
//		                               stickyDebugType,
//		                               quiet,
//		                               asyncRR,
//		                               forceNoDebugMode);
//
//		//	  std::cout << "SendRequestsForRange arguments..." << std::endl;
//		//	  std::cout << "number = "             << number << std::endl;
//		//	  std::cout << "timestampOffset = "    << timestampStart << std::endl;
//		//	  std::cout << "incrementTimestamp = " << incrementTimestamp << std::endl;
//		//	  std::cout << "cfodelay = "           << cfodelay << std::endl;
//		//	  std::cout << "requestsAhead = "      << requestsAhead << std::endl;
//		//	  std::cout << "END END SendRequestsForRange arguments..." << std::endl;
//
//		EmulatedCFO_->SendRequestsForRange(
//		    number,
//		    DTCLib::DTC_Timestamp(static_cast<uint64_t>(timestampStart)),
//		    incrementTimestamp,
//		    cfodelay,
//		    requestsAhead);
//
//		delete EmulatedCFO_;
//
//		auto readoutRequestTime = device->GetDeviceTime();
//		device->ResetDeviceTime();
//		auto afterRequests = std::chrono::steady_clock::now();
//
//		// print out stuff
//		unsigned quietCount = 1;
//		quiet               = true;
//
//		for(unsigned ii = 0; ii < number; ++ii)
//		{
//			std::cout << "Buffer Read " << std::dec << ii << std::endl;
//			mu2e_databuff_t* buffer;
//			auto             tmo_ms = 1500;
//			std::cout << "util - before read for DAQ - ii=" << ii;
//			auto sts = device->read_data(
//			    DTC_DMA_Engine_DAQ, reinterpret_cast<void**>(&buffer), tmo_ms);
//			std::cout << "util - after read for DAQ - ii=" << ii << ", sts=" << sts
//			          << ", buffer=" << (void*)buffer;
//
//			if(sts > 0)
//			{
//				void* readPtr = &buffer[0];
//				auto  bufSize = static_cast<uint16_t>(*static_cast<uint64_t*>(readPtr));
//				readPtr       = static_cast<uint8_t*>(readPtr) + 8;
//				std::cout << "Buffer reports DMA size of " << std::dec << bufSize
//				          << " bytes. Device driver reports read of " << sts << " bytes,"
//				          << std::endl;
//
//				std::cout << "util - bufSize is " << bufSize;
//				outputStream.write(static_cast<char*>(readPtr), sts - 8);
//
//				auto maxLine = static_cast<unsigned>(ceil((sts - 8) / 16.0));
//				for(unsigned line = 0; line < maxLine; ++line)
//				{
//					std::stringstream ostr;
//					ostr << "0x" << std::hex << std::setw(5) << std::setfill('0') << line
//					     << "0: ";
//					for(unsigned byte = 0; byte < 8; ++byte)
//					{
//						if(line * 16 + 2 * byte < sts - 8u)
//						{
//							auto thisWord =
//							    reinterpret_cast<uint16_t*>(buffer)[4 + line * 8 + byte];
//							ostr << std::setw(4) << static_cast<int>(thisWord) << " ";
//						}
//					}
//					std::cout << ostr.str();
//					if(maxLine > quietCount * 2 && quiet && line == (quietCount - 1))
//					{
//						line = static_cast<unsigned>(ceil((sts - 8) / 16.0)) -
//						       (1 + quietCount);
//					}
//				}
//			}
//			device->read_release(DTC_DMA_Engine_DAQ, 1);
//		}
//	}
//	else
//	{
//		__FE_SS__ << "Error! DTC must be in CFOEmulate mode" << __E__;
//		__FE_SS_THROW__;
//	}
//
//}  // end DTCSendHeartbeatAndDataRequest()
//
////========================================================================
//void CFOandDTCCoreVInterface::DTCReset(__ARGS__) { DTCReset(); }
//
////========================================================================
//void CFOandDTCCoreVInterface::DTCReset()
//{
//	{
//		char* address = new char[universalAddressSize_]{
//		    0};  //create address buffer of interface size and init to all 0
//		char* data = new char[universalDataSize_]{
//		    0};                 //create data buffer of interface size and init to all 0
//		uint64_t macroAddress;  // create macro address buffer (size 8 bytes)
//		uint64_t macroData;     // create macro address buffer (size 8 bytes)
//		std::map<std::string /*arg name*/, uint64_t /*arg val*/>
//		    macroArgs;  // create map from arg name to 64-bit number
//
//		// command-#0: Write(0x9100 /*address*/,0xa0000000 /*data*/);
//		macroAddress = 0x9100;
//		memcpy(address, &macroAddress, 8);  // copy macro address to buffer
//		macroData = 0xa0000000;
//		memcpy(data, &macroData, 8);  // copy macro data to buffer
//		universalWrite(address, data);
//
//		// command-#1: Write(0x9118 /*address*/,0x0000003f /*data*/);
//		macroAddress = 0x9118;
//		memcpy(address, &macroAddress, 8);  // copy macro address to buffer
//		macroData = 0x0000003f;
//		memcpy(data, &macroData, 8);  // copy macro data to buffer
//		universalWrite(address, data);
//
//		// command-#2: Write(0x9100 /*address*/,0x00000000 /*data*/);
//		macroAddress = 0x9100;
//		memcpy(address, &macroAddress, 8);  // copy macro address to buffer
//		macroData = 0x00000000;
//		memcpy(data, &macroData, 8);  // copy macro data to buffer
//		universalWrite(address, data);
//
//		// command-#3: Write(0x9100 /*address*/,0x10000000 /*data*/);
//		macroAddress = 0x9100;
//		memcpy(address, &macroAddress, 8);  // copy macro address to buffer
//		macroData = 0x10000000;
//		memcpy(data, &macroData, 8);  // copy macro data to buffer
//		universalWrite(address, data);
//
//		// command-#4: Write(0x9100 /*address*/,0x30000000 /*data*/);
//		macroAddress = 0x9100;
//		memcpy(address, &macroAddress, 8);  // copy macro address to buffer
//		macroData = 0x30000000;
//		memcpy(data, &macroData, 8);  // copy macro data to buffer
//		universalWrite(address, data);
//
//		// command-#5: Write(0x9100 /*address*/,0x10000000 /*data*/);
//		macroAddress = 0x9100;
//		memcpy(address, &macroAddress, 8);  // copy macro address to buffer
//		macroData = 0x10000000;
//		memcpy(data, &macroData, 8);  // copy macro data to buffer
//		universalWrite(address, data);
//
//		// command-#6: Write(0x9118 /*address*/,0x00000000 /*data*/);
//		macroAddress = 0x9118;
//		memcpy(address, &macroAddress, 8);  // copy macro address to buffer
//		macroData = 0x00000000;
//		memcpy(data, &macroData, 8);  // copy macro data to buffer
//		universalWrite(address, data);
//
//		delete[] address;  // free the memory
//		delete[] data;     // free the memory
//	}
//}
//
////========================================================================
//void CFOandDTCCoreVInterface::RunROCFEMacro(__ARGS__)
//{
//	//	std::string feMacroName = __GET_ARG_IN__("ROC_FEMacroName", std::string);
//	//	std::string rocUID = __GET_ARG_IN__("ROC_UID", std::string);
//	//
//	//	__FE_COUTV__(feMacroName);
//	//	__FE_COUTV__(rocUID);
//
//	auto feMacroIt = rocFEMacroMap_.find(feMacroStruct.feMacroName_);
//	if(feMacroIt == rocFEMacroMap_.end())
//	{
//		__FE_SS__ << "Fatal error - ROC FE Macro name '" << feMacroStruct.feMacroName_
//		          << "' not found in DTC's map!" << __E__;
//		__FE_SS_THROW__;
//	}
//
//	const std::string& rocUID         = feMacroIt->second.first;
//	const std::string& rocFEMacroName = feMacroIt->second.second;
//
//	__FE_COUTV__(rocUID);
//	__FE_COUTV__(rocFEMacroName);
//
//	auto rocIt = rocs_.find(rocUID);
//	if(rocIt == rocs_.end())
//	{
//		__FE_SS__ << "Fatal error - ROC name '" << rocUID << "' not found in DTC's map!"
//		          << __E__;
//		__FE_SS_THROW__;
//	}
//
//	rocIt->second->runSelfFrontEndMacro(rocFEMacroName, argsIn, argsOut);
//
//}  // end RunROCFEMacro()

