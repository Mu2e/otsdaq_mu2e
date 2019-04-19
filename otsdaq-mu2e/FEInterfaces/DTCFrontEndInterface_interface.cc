#include "otsdaq-core/Macros/InterfacePluginMacros.h"
#include "otsdaq-core/PluginMakers/MakeInterface.h"
#include "otsdaq-mu2e/FEInterfaces/DTCFrontEndInterface.h"

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "DTCFrontEndInterface"

//=========================================================================================
DTCFrontEndInterface::DTCFrontEndInterface(
    const std::string&       interfaceUID,
    const ConfigurationTree& theXDAQContextConfigTree,
    const std::string&       interfaceConfigurationPath)
    : FEVInterface(interfaceUID, theXDAQContextConfigTree, interfaceConfigurationPath)
    , thisDTC_(0)
{
	__FE_COUT__ << "instantiate DTC... " << interfaceUID << " "
	            << theXDAQContextConfigTree << " " << interfaceConfigurationPath << __E__;


	// theFrontEndHardware_ = new FrontEndHardwareTemplate();
	// theFrontEndFirmware_ = new FrontEndFirmwareTemplate();
	universalAddressSize_ = 4;
	universalDataSize_    = 4;

	configure_clock_ = getSelfNode().getNode("ConfigureClock").getValue<bool>();
	emulate_cfo_ = getSelfNode().getNode("EmulateCFO").getValue<bool>();

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

	if(emulatorMode_)
	{
		__FE_COUT__ << "Emulator DTC mode starting up..." << __E__;
		createROCs();
		registerFEMacros();
		return;
	}
	// else not emulator mode

	snprintf(devfile_, 11, "/dev/" MU2E_DEV_FILE, dtc_);
	fd_ = open(devfile_, O_RDONLY);

	unsigned dtc_class_roc_mask = 0;
	// create roc mask for DTC
	{
		std::vector<std::pair<std::string, ConfigurationTree>> rocChildren =
		    Configurable::getSelfNode().getNode("LinkToROCGroupTable").getChildren();

		roc_mask_ = 0;

		for(auto& roc : rocChildren)
		{
			__FE_COUT__ << "roc uid " << roc.first << __E__;
			bool enabled = roc.second.getNode("Status").getValue<bool>();
			__FE_COUT__ << "roc enable " << enabled << __E__;

			if(enabled)
			{
				int linkID = roc.second.getNode("linkID").getValue<int>();
				roc_mask_ |= (0x1 << linkID);
				dtc_class_roc_mask |=
				    (0x1 << (linkID * 4));  // the DTC class instantiation expects each
				                            // ROC has its own hex nibble
			}
		}

		__FE_COUT__ << "DTC roc_mask_ = 0x" << std::hex << roc_mask_ << std::dec << __E__;
		__FE_COUT__ << "roc_mask to instantiate DTC class = 0x" << std::hex
		            << dtc_class_roc_mask << std::dec << __E__;

	}  // end create roc mask

	// instantiate DTC with the appropriate ROCs enabled
	std::string expectedDesignVersion = "";
	auto        mode                  = DTCLib::DTC_SimMode_NoCFO;
	thisDTC_ = new DTCLib::DTC(mode, dtc_, dtc_class_roc_mask, expectedDesignVersion);

	createROCs();
	registerFEMacros();

	// DTC-specific info
	dtc_location_in_chain_ =
	    getSelfNode().getNode("LocationInChain").getValue<unsigned int>();



	//check if any ROCs should be DTC-hardware emulated ROCs
	{
		std::vector<std::pair<std::string, ConfigurationTree>> rocChildren =
				Configurable::getSelfNode().getNode("LinkToROCGroupTable").getChildren();

		int dtcHwEmulateROCmask = 0;
		for(auto& roc : rocChildren)
		{
			bool enabled = roc.second.getNode("EmulateInDTCHardware").getValue<bool>();

			if(enabled)
			{
				int linkID = roc.second.getNode("linkID").getValue<int>();
				__FE_COUT__ << "roc uid '" << roc.first <<
						"' at link=" << linkID << " is DTC-hardware emulated!" << __E__;
				dtcHwEmulateROCmask |= (1<<linkID);
			}
		}

		__FE_COUT__ << "Writing DTC-hardware emulation mask: 0x" <<
				std::hex << dtcHwEmulateROCmask << std::dec << __E__;
		registerWrite(0x9110, dtcHwEmulateROCmask);
		__FE_COUT__ << "End check for DTC-hardware emulated ROCs." << __E__;
	}  // end check if any ROCs should be DTC-hardware emulated ROCs


	// done
	__MCOUT_INFO__("DTCFrontEndInterface instantiated with name: "
	               << device_name_ << " dtc_location_in_chain_ = "
	               << dtc_location_in_chain_ << " talking to /dev/mu2e" << dtc_ << __E__);
} //end constructor()

//==========================================================================================
DTCFrontEndInterface::~DTCFrontEndInterface(void)
{
	if(thisDTC_)
		delete thisDTC_;
	// delete theFrontEndHardware_;
	// delete theFrontEndFirmware_;

	__FE_COUT__ << "Destructed." << __E__;
} //end destructor()

//========================================================================================================================
void DTCFrontEndInterface::registerFEMacros(void)
{
	mapOfFEMacroFunctions_.clear();

	// clang-format off
	registerFEMacroFunction(
			"ROC_WriteBlock",  // feMacroName
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::WriteROCBlock),  // feMacroFunction
					std::vector<std::string>{"rocLinkIndex", "block", "address", "writeData"},
					std::vector<std::string>{},  // namesOfOutputArgs
					1);                          // requiredUserPermissions

	registerFEMacroFunction("ROC_ReadBlock",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::ReadROCBlock),
				        std::vector<std::string>{"rocLinkIndex", "numberOfWords", "address", "incrementAddress"},
					std::vector<std::string>{"readData"},
					1);  // requiredUserPermissions

	// registration of FEMacro 'DTCStatus' generated, Oct-22-2018 03:16:46, by
	// 'admin' using MacroMaker.
	registerFEMacroFunction(
			"ROC_Write",  // feMacroName
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::WriteROC),  // feMacroFunction
					std::vector<std::string>{"rocLinkIndex", "address", "writeData"},
					std::vector<std::string>{},  // namesOfOutput
					1);                          // requiredUserPermissions

	registerFEMacroFunction(
			"ROC_Read",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::ReadROC),                  // feMacroFunction
					std::vector<std::string>{"rocLinkIndex", "address"},  // namesOfInputArgs
					std::vector<std::string>{"readData"},
					1);  // requiredUserPermissions

	registerFEMacroFunction("DTC_Reset",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::DTCReset),
					std::vector<std::string>{},
					std::vector<std::string>{},
					1);  // requiredUserPermissions

	registerFEMacroFunction("DTC_HighRate_DCS_Check",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::DTCHighRateDCSCheck),
					std::vector<std::string>{"rocLinkIndex","loops","baseAddress",
						"correctRegisterValue0","correctRegisterValue1"},
					std::vector<std::string>{},
					1);  // requiredUserPermissions




	{ //add ROC FE Macros
		__FE_COUT__ << "Getting children ROC FEMacros..." << __E__;
		rocFEMacroMap_.clear();
		for(auto& roc : rocs_)
		{
			auto feMacros = roc.second->getMapOfFEMacroFunctions();
			for(auto& feMacro:feMacros)
			{
				__FE_COUT__ << roc.first << "::" << feMacro.first << __E__;

				//make DTC FEMacro forwarding to ROC FEMacro
				std::string macroName =
						"Link" +
						std::to_string(roc.second->getLinkID()) +
						"_" + roc.first + "_" +
						feMacro.first;
				__FE_COUTV__(macroName);

				std::vector<std::string> inputArgs,outputArgs;

				//inputParams.push_back("ROC_UID");
				//inputParams.push_back("ROC_FEMacroName");
				for(auto& inArg: feMacro.second.namesOfInputArguments_)
					inputArgs.push_back(inArg);
				for(auto& outArg: feMacro.second.namesOfOutputArguments_)
					outputArgs.push_back(outArg);

				__FE_COUTV__(StringMacros::vectorToString(inputArgs));
				__FE_COUTV__(StringMacros::vectorToString(outputArgs));

				rocFEMacroMap_.emplace(std::make_pair(macroName,
						std::make_pair(roc.first,feMacro.first)));

				registerFEMacroFunction(macroName,
						static_cast<FEVInterface::frontEndMacroFunction_t>(
								&DTCFrontEndInterface::RunROCFEMacro),
								inputArgs,
								outputArgs,
								1);  // requiredUserPermissions
			}
		}
	} //end add ROC FE Macros

	// clang-format on

} //end registerFEMacros()

//========================================================================================================================
void DTCFrontEndInterface::createROCs(void)
{
	rocs_.clear();

	std::vector<std::pair<std::string, ConfigurationTree>> rocChildren =
	    Configurable::getSelfNode().getNode("LinkToROCGroupTable").getChildren();

	// instantiate vector of ROCs
	for(auto& roc : rocChildren)
		if(roc.second.getNode("Status").getValue<bool>())
		{
			__FE_COUT__
			    << "ROC Plugin Name: "
			    << roc.second.getNode("ROCInterfacePluginName").getValue<std::string>()
			    << std::endl;
			__FE_COUT__ << "ROC Name: " << roc.first << std::endl;

			try
			{
				__COUTV__(theXDAQContextConfigTree_.getValueAsString());
				__COUTV__(
				    roc.second.getNode("ROCInterfacePluginName").getValue<std::string>());

				// Note: FEVInterface makeInterface returns a unique_ptr
				//	and we want to verify that ROCCoreVInterface functionality
				//	is there, so we do an intermediate dynamic_cast to check
				//	before placing in a new unique_ptr of type ROCCoreVInterface.
				std::unique_ptr<FEVInterface> tmpVFE = makeInterface(
				    roc.second.getNode("ROCInterfacePluginName").getValue<std::string>(),
				    roc.first,
				    theXDAQContextConfigTree_,
				    (theConfigurationPath_ + "/LinkToROCGroupTable/" + roc.first));

				// setup parent supervisor of FEVinterface (for backwards compatibility,
				// left out of constructor)
				tmpVFE->parentSupervisor_ = parentSupervisor_;

				ROCCoreVInterface& tmpRoc = dynamic_cast<ROCCoreVInterface&>(
				    *tmpVFE);  // dynamic_cast<ROCCoreVInterface*>(tmpRoc.get());

				// setup other members of ROCCore (for interface plug-in compatibility,
				// left out of constructor)

				__COUTV__(tmpRoc.emulatorMode_);
				tmpRoc.emulatorMode_ = emulatorMode_;
				__COUTV__(tmpRoc.emulatorMode_);

				if(emulatorMode_)
				{
					__FE_COUT__ << "Creating ROC in emulator mode..." << __E__;

					// try
					{
						// all ROCs support emulator mode

						//						// verify ROCCoreVEmulator class
						//functionality  with  dynamic_cast
						// ROCCoreVEmulator&  tmpEmulator =
						// dynamic_cast<ROCCoreVEmulator&>(
						//						    tmpRoc);  //
						// dynamic_cast<ROCCoreVInterface*>(tmpRoc.get());

						// start emulator thread
						std::thread(
						    [](ROCCoreVInterface* rocEmulator) {
							    __COUT__ << "Starting ROC emulator thread..." << __E__;
							    ROCCoreVInterface::emulatorThread(rocEmulator);
						    },
						    &tmpRoc)
						    .detach();
					}
					//					catch(const std::bad_cast& e)
					//					{
					//						__SS__ << "Cast to ROCCoreVEmulator failed!
					//Verify  the  emulator " 						          "plugin
					// inherits
					// from  ROCCoreVEmulator."
					//						       << __E__;
					//						ss << "Failed to instantiate plugin named '"
					//<<  roc.first
					//						   << "' of type '"
					//						   <<
					// roc.second.getNode("ROCInterfacePluginName")
					//						          .getValue<std::string>()
					//						   << "' due to the following error: \n"
					//						   << e.what() << __E__;
					//
					//						__SS_THROW__;
					//					}
				}
				else
				{
					tmpRoc.thisDTC_ = thisDTC_;
				}

				rocs_.emplace(std::pair<std::string, std::unique_ptr<ROCCoreVInterface>>(
				    roc.first, &tmpRoc));
				tmpVFE.release();  // release the FEVInterface unique_ptr, so we are left
				                   // with just one

				__COUTV__(rocs_[roc.first]->emulatorMode_);
			}
			catch(const cet::exception& e)
			{
				__SS__ << "Failed to instantiate plugin named '" << roc.first
				       << "' of type '"
				       << roc.second.getNode("ROCInterfacePluginName")
				              .getValue<std::string>()
				       << "' due to the following error: \n"
				       << e.what() << __E__;
				__FE_COUT_ERR__ << ss.str();
				__MOUT_ERR__ << ss.str();
				__SS_ONLY_THROW__;
			}
			catch(const std::bad_cast& e)
			{
				__SS__ << "Cast to ROCCoreVInterface failed! Verify the plugin inherits "
				          "from ROCCoreVInterface."
				       << __E__;
				ss << "Failed to instantiate plugin named '" << roc.first << "' of type '"
				   << roc.second.getNode("ROCInterfacePluginName").getValue<std::string>()
				   << "' due to the following error: \n"
				   << e.what() << __E__;

				__FE_COUT_ERR__ << ss.str();
				__MOUT_ERR__ << ss.str();

				__SS_ONLY_THROW__;
			}
		}

	__FE_COUT__ << "Done creating " << rocs_.size() << " ROC(s)" << std::endl;



}  // end createROCs

//==========================================================================================
// universalRead
//	Must implement this function for Macro Maker to work with this
// interface. 	When Macro Maker calls:
//		- address will be a [universalAddressSize_] byte long char array
//		- returnValue will be a [universalDataSize_] byte long char
// array
//		- expects exception thrown on failure/timeout
void DTCFrontEndInterface::universalRead(char* address, char* returnValue)
{
	// __FE_COUT__ << "DTC READ" << __E__;

	if(emulatorMode_)
	{
		for(unsigned int i = 0; i < universalDataSize_; ++i)
			returnValue[i] = 0xF0 | i;
		return;
	}

	reg_access_.access_type = 0;  // 0 = read, 1 = write

	reg_access_.reg_offset = *((int*)address);
	// __COUTV__(reg_access.reg_offset);

	if(ioctl(fd_, M_IOC_REG_ACCESS, &reg_access_))
	{
		__SS__ << "ERROR: DTC universal read - Does file exist? -> /dev/mu2e" << dtc_
		       << __E__;
		__SS_THROW__;
	}

	std::memcpy(returnValue, &reg_access_.val, universalDataSize_);
	// __COUTV__(reg_access_.val);
}

//===========================================================================================
// registerRead: return = value read from register at address "address"
//
int DTCFrontEndInterface::registerRead(int address)
{
	uint8_t* addrs = new uint8_t[universalAddressSize_];  // create address buffer
	                                                      // of interface size
	uint8_t* data =
	    new uint8_t[universalDataSize_];  // create data buffer of interface size

	uint8_t macroAddrs[20] =
	    {};  // total hack, assuming we'll never have 200 bytes in an address

	// fill byte-by-byte
	for(unsigned int i = 0; i < universalAddressSize_; i++)
		macroAddrs[i] = 0xff & (address >> i * 8);

	// 0-pad
	for(unsigned int i = 0; i < universalAddressSize_; ++i)
		addrs[i] = (i < 2) ? macroAddrs[i] : 0;

	universalRead((char*)addrs, (char*)data);

	unsigned int readvalue = 0x00000000;

	// unpack byte-by-byte
	for(uint8_t i = universalDataSize_; i > 0; i--)
		readvalue = (readvalue << 8 & 0xffffff00) | data[i - 1];

	// __FE_COUT__ << "DTC: readvalue register 0x" << std::hex << address
	//	<< " is..." << std::hex << readvalue << __E__;

	delete[] addrs;  // free the memory
	delete[] data;   // free the memory

	return readvalue;
}

//=====================================================================================
// universalWrite
//	Must implement this function for Macro Maker to work with this
// interface. 	When Macro Maker calls:
//		- address will be a [universalAddressSize_] byte long char array
//		- writeValue will be a [universalDataSize_] byte long char array
void DTCFrontEndInterface::universalWrite(char* address, char* writeValue)
{
	// __FE_COUT__ << "DTC WRITE" << __E__;
	if(emulatorMode_)
		return;

	reg_access_.access_type = 1;  // 0 = read, 1 = write

	reg_access_.reg_offset = *((int*)address);
	// __COUTV__(reg_access.reg_offset);

	reg_access_.val = *((int*)writeValue);
	// __COUTV__(reg_access.val);

	if(ioctl(fd_, M_IOC_REG_ACCESS, &reg_access_))
		__FE_COUT_ERR__ << "ERROR: DTC universal write - Does file exist? /dev/mu2e"
		                << dtc_ << __E__;

	return;
}

//===============================================================================================
// registerWrite: return = value readback from register at address "address"
//
int DTCFrontEndInterface::registerWrite(int address, int dataToWrite)
{
	uint8_t* addrs = new uint8_t[universalAddressSize_];  // create address buffer
	                                                      // of interface size
	uint8_t* data =
	    new uint8_t[universalDataSize_];  // create data buffer of interface size

	uint8_t macroAddrs[20] = {};  // assume we'll never have 20 bytes in an address
	uint8_t macroData[20] =
	    {};  // assume we'll never have 20 bytes read out from a register

	// fill byte-by-byte
	for(unsigned int i = 0; i < universalAddressSize_; i++)
		macroAddrs[i] = 0xff & (address >> i * 8);

	// 0-pad
	for(unsigned int i = 0; i < universalAddressSize_; ++i)
		addrs[i] = (i < 2) ? macroAddrs[i] : 0;

	// fill byte-by-byte
	for(unsigned int i = 0; i < universalDataSize_; i++)
		macroData[i] = 0xff & (dataToWrite >> i * 8);

	// 0-pad
	for(unsigned int i = 0; i < universalDataSize_; ++i)
		data[i] = (i < 4) ? macroData[i] : 0;

	universalWrite((char*)addrs, (char*)data);

	// usleep(100);

	int readbackValue = registerRead(address);

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

	delete[] addrs;  // free the memory
	delete[] data;   // free the memory

	return readbackValue;
}

//==================================================================================================
void DTCFrontEndInterface::readStatus(void)
{
	__FE_COUT__ << device_name_ << " firmware version (0x9004) = 0x" << std::hex
	            << registerRead(0x9004) << __E__;

	printVoltages();

	__FE_COUT__ << device_name_ << " temperature = " << readTemperature() << " degC"
	            << __E__;

	__FE_COUT__ << device_name_ << " SERDES reset........ (0x9118) = 0x" << std::hex
	            << registerRead(0x9118) << __E__;
	__FE_COUT__ << device_name_ << " SERDES disparity err (0x911c) = 0x" << std::hex
	            << registerRead(0x9118) << __E__;
	__FE_COUT__ << device_name_ << " SERDES unlock error. (0x9124) = 0x" << std::hex
	            << registerRead(0x9124) << __E__;
	__FE_COUT__ << device_name_ << " PLL locked.......... (0x9128) = 0x" << std::hex
	            << registerRead(0x9128) << __E__;
	__FE_COUT__ << device_name_ << " SERDES Rx status.... (0x9134) = 0x" << std::hex
	            << registerRead(0x9134) << __E__;
	__FE_COUT__ << device_name_ << " SERDES reset done... (0x9138) = 0x" << std::hex
	            << registerRead(0x9138) << __E__;
	__FE_COUT__ << device_name_ << " link status......... (0x9140) = 0x" << std::hex
	            << registerRead(0x9140) << __E__;
	__FE_COUT__ << device_name_ << " SERDES ref clk freq. (0x915c) = 0x" << std::hex
	            << registerRead(0x915c) << " = " << std::dec << registerRead(0x915c)
	            << __E__;
	__FE_COUT__ << device_name_ << " control............. (0x9100) = 0x" << std::hex
	            << registerRead(0x9100) << __E__;
	__FE_COUT__ << __E__;

	return;
}

//==================================================================================================
float DTCFrontEndInterface::readTemperature()
{
	int tempadc = registerRead(0x9010);

	float temperature = ((tempadc * 503.975) / 4096.) - 273.15;

	return temperature;
}

//==================================================================================================
// turn on LED on front of timing card
//
void DTCFrontEndInterface::turnOnLED()
{
	int dataInReg = registerRead(0x9100);

	int dataToWrite = dataInReg | 0x001f0000;

	registerWrite(0x9100, dataToWrite);

	return;
}

//==================================================================================================
// turn off LED on front of timing card
//
void DTCFrontEndInterface::turnOffLED()
{
	int dataInReg = registerRead(0x9100);

	int dataToWrite = dataInReg & 0xffe0ffff;

	registerWrite(0x9100, dataToWrite);

	return;
}

//==================================================================================================
int DTCFrontEndInterface::getROCLinkStatus(int ROC_link)
{
	int overall_link_status = registerRead(0x9140);

	int ROC_link_status = (overall_link_status >> ROC_link) & 0x1;

	return ROC_link_status;
}

int DTCFrontEndInterface::getCFOLinkStatus()
{
	int overall_link_status = registerRead(0x9140);

	int CFO_link_status = (overall_link_status >> 6) & 0x1;

	return CFO_link_status;
}

void DTCFrontEndInterface::printVoltages()
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

int DTCFrontEndInterface::checkLinkStatus()
{
	int ROCs_OK = 1;

	for(int i = 0; i < 8; i++)
	{
		//__FE_COUT__ << " check link " << i << " ";

		if(ROCActive(i))
		{
			//__FE_COUT__ << " active... status = " << getROCLinkStatus(i) << __E__;
			ROCs_OK &= getROCLinkStatus(i);
		}
	}

	if((getCFOLinkStatus() == 1) && ROCs_OK == 1)
	{
		//	__FE_COUT__ << "DTC links OK = 0x" << std::hex << registerRead(0x9140)
		//<< std::dec << __E__;
		//	__MOUT__ << "DTC links OK = 0x" << std::hex << registerRead(0x9140) <<
		// std::dec << __E__;

		return 1;
	}
	else
	{
		//	__FE_COUT__ << "DTC links not OK = 0x" << std::hex <<
		// registerRead(0x9140) << std::dec << __E__;
		//	__MOUT__ << "DTC links not OK = 0x" << std::hex << registerRead(0x9140)
		//<< std::dec << __E__;

		return 0;
	}
}

bool DTCFrontEndInterface::ROCActive(unsigned ROC_link)
{
	// __FE_COUTV__(roc_mask_);
	// __FE_COUTV__(ROC_link);

	if(((roc_mask_ >> ROC_link) & 0x01) == 1)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//==================================================================================================
void DTCFrontEndInterface::configure(void) try
{
	__FE_COUTV__(getIterationIndex());
	__FE_COUTV__(getSubIterationIndex());

	if(emulatorMode_)
	{
		__FE_COUT__ << "Emulator DTC configuring... # of ROCs = " << rocs_.size()
		            << __E__;
		for(auto& roc : rocs_)
			roc.second->configure();
		return;
	}
	__FE_COUT__ << "DTC configuring... # of ROCs = " << rocs_.size() << __E__;





	// NOTE: otsdaq/xdaq has a soap reply timeout for state transitions.
	// Therefore, break up configuration into several steps so as to reply before
	// the time out As well, there is a specific order in which to configure the
	// links in the chain of CFO->DTC0->DTC1->...DTCN

	const int number_of_system_configs =
	    -1;  // if < 0, keep trying until links are OK.
	         // If > 0, go through configuration steps this many times

	const int reset_fpga               = 1;                 // 1 = yes, 0 = no
	const bool config_clock             = configure_clock_;  // 1 = yes, 0 = no
	const bool config_jitter_attenuator = configure_clock_;  // 1 = yes, 0 = no
	const int reset_rx                 = 0;                 // 1 = yes, 0 = no

	const int number_of_dtc_config_steps = 7;

	const int max_number_of_tries = 3;  // max number to wait for links OK

	int number_of_total_config_steps =
	    number_of_system_configs * number_of_dtc_config_steps;

	int config_step    = getIterationIndex();
	int config_substep = getSubIterationIndex();

	if(number_of_system_configs > 0)
	{
		if(config_step >= number_of_total_config_steps)  // done, exit system config
			return;
	}

	// waiting for link loop
	if(config_substep > 0 && config_substep < max_number_of_tries)
	{  // wait a maximum of 30 seconds

		const int number_of_link_checks = 10;

		int link_ok = 0;

		for(int i = 0; i < number_of_link_checks; i++)
		{
			if(checkLinkStatus() == 1)
			{
				// links OK,  continue with the rest of the configuration

				indicateIterationWork();
				turnOffLED();
				return;
			}
			else if(getCFOLinkStatus() == 0)
			{
				// in this case, links will never get to be OK, stop waiting for them

				__FE_COUT__ << device_name_ << " CFO Link Status is bad = 0x" << std::hex
				            << registerRead(0x9140) << std::dec << __E__;
				sleep(1);

				indicateIterationWork();
				turnOffLED();
				return;
			}
			else
			{
				// links still not OK, keep checking...

				__FE_COUT__ << "Waiting for DTC Link Status = 0x" << std::hex
				            << registerRead(0x9140) << std::dec << __E__;
				sleep(1);
			}
		}

		indicateSubIterationWork();
		return;
	}
	else if(config_substep > max_number_of_tries)
	{
		// wait a maximum of 30 seconds, then stop waiting for them

		__FE_COUT__ << "Links still bad = 0x" << std::hex << registerRead(0x9140)
		            << std::dec << "... continue" << __E__;
		indicateIterationWork();
		turnOffLED();
		return;
	}

	turnOnLED();

	if((config_step % number_of_dtc_config_steps) == 0)
	{
		if(reset_fpga == 1 && config_step < number_of_dtc_config_steps)
		{
			// only reset the FPGA the first time through

			__MCOUT_INFO__("Step " << config_step << ": " << device_name_
			                       << " reset FPGA...");

			registerWrite(0x9100, 0x80000000);  // bit 31 = DTC Reset FPGA
			sleep(3);

			__MCOUT_INFO__("............. firmware version "
			               << std::hex << registerRead(0x9004) << std::dec << __E__);

			DTCReset();
		}

		if (emulate_cfo_ == 1) 
		{
		  __MCOUT_INFO__("..... enable CFO emulation...");
		  registerWrite(0x9100, 0x40000000);

		  __FE_COUT__ << "CFO emulation: turn off Event Windows" << __E__;
		  registerWrite(0x91f0, 0x00000000);

		  __FE_COUT__ << "CFO emulation: turn off 40MHz marker interval" << __E__;
		  registerWrite(0x91f4, 0x00000000);
		}

	}
	else if((config_step % number_of_dtc_config_steps) == 1)
	{
		if(config_clock == 1 && config_step < number_of_dtc_config_steps)
		{
			// only configure the clock/crystal the first loop through...

			//  registerWrite(0x9100,0x10000000);	// bit 31 = DTC Reset FPGA

			__MCOUT_INFO__("Step " << config_step << ": " << device_name_
			                       << " reset clock..." << __E__);

			__FE_COUT__ << "DTC - set crystal frequency to 156.25 MHz" << __E__;
			registerWrite(0x915c, 0x09502F90);

			// set RST_REG bit
			registerWrite(0x9168, 0x5d870100);
			registerWrite(0x916c, 0x00000001);

			sleep(5);

			int targetFrequency = 200000000;

			//--- begin code snippet pulled from: mu2eUtil program_clock -C 2 -F
			// 200000000

			// auto oscillator = DTCLib::DTC_OscillatorType_SERDES; //-C 0 = CFO (main
			// board SERDES clock) auto oscillator = DTCLib::DTC_OscillatorType_DDR;
			// //-C 1 (DDR clock)
			auto oscillator =
			    DTCLib::DTC_OscillatorType_Timing;  //-C 2 = DTC (with timing card)

			__FE_COUT__ << "DTC - set oscillator frequency to " << std::dec
			            << targetFrequency << " MHz" << __E__;

			thisDTC_->SetNewOscillatorFrequency(oscillator, targetFrequency);

			//--- end code snippet pulled from: mu2eUtil program_clock -C 2 -F
			// 200000000

			sleep(5);
		}
		else
		{
			__MCOUT_INFO__("Step " << config_step << ": " << device_name_
			                       << " do NOT reset clock..." << __E__);
		}
	}
	else if((config_step % number_of_dtc_config_steps) == 2)
	{
		// configure Jitter Attenuator to recover clock

		if(config_jitter_attenuator == 1 && config_step < number_of_dtc_config_steps)
		{
			__MCOUT_INFO__("Step " << config_step << ": " << device_name_
			                       << " configure Jitter Attenuator..." << __E__);

			configureJitterAttenuator();

			sleep(5);
		}
		else
		{
			__MCOUT_INFO__("Step " << config_step << ": " << device_name_
			                       << " do NOT configure Jitter Attenuator..." << __E__);
		}
	}
	else if((config_step % number_of_dtc_config_steps) == 3)
	{
		// reset CFO links, first what is received from upstream, then what is
		// transmitted downstream

		// THIS SHOULD NOT BE NECESSARY with firmware version 20181024
		if(reset_rx == 1)
		{
			__FE_COUT__ << "DTC reset CFO link CPLL" << __E__;
			registerWrite(0x9118, 0x00004000);
			registerWrite(0x9118, 0x00000000);

			sleep(3);

			__FE_COUT__ << "DTC reset CFO link RX" << __E__;
			registerWrite(0x9118, 0x00400000);
			registerWrite(0x9118, 0x00000000);

			sleep(3);
		}
		else
		{
			__FE_COUT__ << "DTC do NOT reset PLL and CFO RX" << __E__;
		}
		//
		// 	 __FE_COUT__ << "DTC reset CFO link TX" << __E__;
		// 	 registerWrite(0x9118,0x40000000);
		// 	 registerWrite(0x9118,0x00000000);
		//
		// 	 sleep(6);
	}
	else if((config_step % number_of_dtc_config_steps) == 4)
	{
		// reset ROC links, first what is sent out, then what is received, then
		// check links

		// RESETTING THE LINKS SHOULD NOT BE NECESSARY with firmware version
		// 20181024, however, we DO want to confirm that the links are OK...

		// 	 __FE_COUT__ << "DTC reset ROC link SERDES CPLLs" << __E__;
		// 	 registerWrite(0x9118,0x00003f00);
		// 	 registerWrite(0x9118,0x00000000);
		//
		//	 sleep(3);
		//
		// 	 __FE_COUT__ << "DTC reset ROC link SERDES TX" << __E__;
		// 	 registerWrite(0x9118,0x3f000000);
		// 	 registerWrite(0x9118,0x00000000);
		//
		// 	 sleep(3);
		//
		// 	 // WILL NEED TO CONFIGURE THE ROC LINKS HERE BEFORE RESETTING WHAT IS
		// RECEIVED
		//
		// 	 __FE_COUT__ << "DTC reset ROC link SERDES RX" << __E__;
		// 	 registerWrite(0x9118,0x003f0000);
		// 	 registerWrite(0x9118,0x00000000);
		//
		// 	 sleep(6);

		__MCOUT_INFO__("Step " << config_step << ": " << device_name_
		                       << " wait for links..." << __E__);

		indicateSubIterationWork();  // tell state machine to stay in configure state
		                             // ("come back to me")

		return;
	}
	else if((config_step % number_of_dtc_config_steps) == 5)
	{
		__MCOUT_INFO__("Step " << config_step << ": " << device_name_
		                       << " enable markers, Tx, Rx" << __E__);

		// enable markers, tx and rx

		int data_to_write = (roc_mask_ << 8) | roc_mask_;
		__FE_COUT__ << "DTC enable markers - enabled ROC links 0x" << std::hex
		            << data_to_write << std::dec << __E__;
		registerWrite(0x91f8, data_to_write);

		data_to_write = 0x4040 | (roc_mask_ << 8) | roc_mask_;
		__FE_COUT__ << "DTC enable tx and rx - CFO and enabled ROC links 0x" << std::hex
		            << data_to_write << std::dec << __E__;
		registerWrite(0x9114, data_to_write);

		// put DTC CFO link output into loopback mode
		__FE_COUT__ << "DTC set CFO link output loopback mode ENABLE" << __E__;
		registerWrite(0x9100, 0x00000000);

		__MCOUT_INFO__("Step " << config_step << ": " << device_name_ << " configure ROCs"
		                       << __E__);
		for(auto& roc : rocs_)
			roc.second->configure();

		sleep(1);
	}
	else if((config_step % number_of_dtc_config_steps) == 6)
	{
	  if (emulate_cfo_ == 1) 
	  {
		__MCOUT_INFO__("Step " << config_step
		                       << ": CFO emulation enable Event start characters "
		                          "and event window interval"
		                       << __E__);

		__FE_COUT__ << "CFO emulation:  set Event Window interval time" << __E__;
		//    registerWrite(0x91f0,0x154);   //1.7us
		registerWrite(0x91f0, 0x1f40);  // 40us
		// 	registerWrite(0x91f0,0x00000000); 	// for NO markers, write these
		// values

		__FE_COUT__ << "CFO emulation:  set 40MHz marker interval" << __E__;
		registerWrite(0x91f4, 0x0800);
		// 	registerWrite(0x91f4,0x00000000); 	// for NO markers, write these
		// values

	  }
		__MCOUT_INFO__("Step " << config_step << ": " << device_name_ << " configured"
		                       << __E__);

		if(checkLinkStatus() == 1)
		{
			__MCOUT_INFO__(device_name_ << " links OK 0x" << std::hex
			                            << registerRead(0x9140) << std::dec << __E__);

			sleep(1);
			turnOffLED();

			if(number_of_system_configs < 0)
			{
				return;  // links OK, kick out of configure
			}
		}
		else if(config_step > max_number_of_tries)
		{
			__MCOUT_INFO__(device_name_ << " links not OK 0x" << std::hex
			                            << registerRead(0x9140) << std::dec << __E__);
			return;
		}
		else
		{
			__MCOUT_INFO__(device_name_ << " links not OK 0x" << std::hex
			                            << registerRead(0x9140) << std::dec << __E__);
		}
	}

	readStatus();  // spit out link status at every step

	indicateIterationWork();  // otherwise, tell state machine to stay in configure
	                          // state ("come back to me")

	turnOffLED();
	return;
}  // end configure()
catch(const std::runtime_error& e)
{
	__FE_SS__ << "Error caught: " << e.what() << __E__;
	__FE_SS_THROW__;
}
catch(...)
{
	__FE_SS__ << "Unknown error caught. Check the printouts!" << __E__;
	__FE_SS_THROW__;
}

//========================================================================================================================
void DTCFrontEndInterface::halt(void)
{
	//	__FE_COUT__ << "HALT: DTC status" << __E__;
	//	readStatus();
}

//========================================================================================================================
void DTCFrontEndInterface::pause(void)
{
	//	__FE_COUT__ << "PAUSE: DTC status" << __E__;
	//	readStatus();
}

//========================================================================================================================
void DTCFrontEndInterface::resume(void)
{
	//	__FE_COUT__ << "RESUME: DTC status" << __E__;
	//	readStatus();
}

//========================================================================================================================
void DTCFrontEndInterface::start(std::string)  // runNumber)
{
	if(emulatorMode_)
	{
		__FE_COUT__ << "Emulator DTC starting..." << __E__;
		return;
	}

	const int numberOfChains       = 1;
	int       link[numberOfChains] = {0};

	const int numberOfDTCsPerChain = 1;

	const int numberOfROCsPerDTC = 1;  // assume these are ROC0 and ROC1

	// To do loopbacks on all CFOs, first have to setup all DTCs, then the CFO
	// (this method) work per iteration.  Loop back done on all chains (in this
	// method), assuming the following order: i  DTC0  DTC1  ...  DTCN 0  ROC0
	// none  ...  none 1  ROC1  none  ...  none 2  none  ROC0  ...  none 3  none
	// ROC1  ...  none
	// ...
	// N-1  none  none  ...  ROC0
	// N  none  none  ...  ROC1

	int totalNumberOfMeasurements =
	    numberOfChains * numberOfDTCsPerChain * numberOfROCsPerDTC;

	int loopbackIndex = getIterationIndex();

	if(loopbackIndex == 0) //start
	{
		initial_9100_ = registerRead(0x9100);
		initial_9114_ = registerRead(0x9114);
		indicateIterationWork();
		return;
	}

	if(loopbackIndex > totalNumberOfMeasurements) //finish
	{
		//    __MCOUT_INFO__(device_name_ << " loopback DONE" << __E__);

		if(checkLinkStatus() == 1)
		{
			//      __MCOUT_INFO__(device_name_ << " links OK 0x" << std::hex <<
			//      registerRead(0x9140) << std::dec << __E__);
		}
		else
		{
			//      __MCOUT_INFO__(device_name_ << " links not OK 0x" << std::hex <<
			//      registerRead(0x9140) << std::dec << __E__);
		}

		for(auto& roc : rocs_)
		{
			__MCOUT_INFO__(".... ROC" << roc.second->getLinkID() << "-DTC link lost "
			                          << roc.second->readDTCLinkLossCounter()
			                          << " times");
		}

		registerWrite(0x9100, initial_9100_);
		registerWrite(0x9114, initial_9114_);

		return;
	}

	//=========== Perform loopback=============

	// where are we in the procedure?
	unsigned int activeROC = (loopbackIndex-1) % numberOfROCsPerDTC;

	int activeDTC = -1;

	for(int nDTC = 0; nDTC < numberOfDTCsPerChain; nDTC++)
	{
		if((loopbackIndex-1) >= (nDTC * numberOfROCsPerDTC) &&
		   (loopbackIndex-1) < ((nDTC + 1) * numberOfROCsPerDTC))
		{
			activeDTC = nDTC;
		}
	}

	// __FE_COUT__ << "loopback index = " << loopbackIndex
	// 	<< " activeDTC = " << activeDTC
	//	 	<< " activeROC = " << activeROC
	//	 	<< __E__;

	if(activeDTC == dtc_location_in_chain_)
	{
		__FE_COUT__ << "DTC" << activeDTC << "loopback mode ENABLE" << __E__;
		registerWrite(0x9100, 0x00000000);
	}
	else
	{
		// this DTC is lower in chain than the one being looped.  Pass the loopback
		// signal through
		__FE_COUT__ << "active DTC = " << activeDTC
		            << " is NOT this DTC = " << dtc_location_in_chain_
		            << "... pass signal through" << __E__;
		registerWrite(0x9100, 0x10000000);
	}

	int ROCToEnable =
	    0x00004040 |
	    (0x101 << activeROC);  // enables TX and Rx to CFO (bit 6) and appropriate ROC
	__FE_COUT__ << "enable ROC " << activeROC << " --> 0x" << std::hex << ROCToEnable
	            << std::dec << __E__;

	registerWrite(0x9114, ROCToEnable);

	// Re-align the links for the activeROC
	for(auto& roc : rocs_)
	{
		if(roc.second->getLinkID() == activeROC)
		{
			__FE_COUT__ << "... ROC realign link... " << __E__;
			roc.second->writeRegister(22, 0);
			roc.second->writeRegister(22, 1);
		}
	}

	indicateIterationWork();
	return;
}

//========================================================================================================================
void DTCFrontEndInterface::stop(void)
{
	int numberOfCAPTANPulses =
	    getConfigurationManager()
	        ->getNode("/Mu2eGlobalsTable/SyncDemoConfig/NumberOfCAPTANPulses")
	        .getValue<unsigned int>();

	__FE_COUTV__(numberOfCAPTANPulses);

	int stopIndex = getIterationIndex();

	if(numberOfCAPTANPulses == 0)
	{
		return;
	}

	if(stopIndex == 0)
	{
		int i = 0;
		for(auto& roc : rocs_)
		{
			// re-align link
			roc.second->writeRegister(22, 0);
			roc.second->writeRegister(22, 1);

			std::stringstream filename;
			filename << "/home/mu2edaq/sync_demo/ots/" << device_name_ << "_ROC"
			         << roc.second->getLinkID() << "data.txt";
			std::string filenamestring = filename.str();
			datafile_[i].open(filenamestring);
			i++;
		}
	}

	if(stopIndex > numberOfCAPTANPulses)
	{
		int i = 0;
		for(auto& roc : rocs_)
		{
			__MCOUT_INFO__(".... ROC" << roc.second->getLinkID() << "-DTC link lost "
			                          << roc.second->readDTCLinkLossCounter()
			                          << " times");
			datafile_[i].close();
			i++;
		}
		return;
	}

	int i = 0;
	__FE_COUT__ << "Entering read timestamp loop..." << __E__;
	for(auto& roc : rocs_)
	{
		int timestamp_data = roc.second->readTimestamp();

		__FE_COUT__ << "Read " << stopIndex << " -> " << device_name_ << " timestamp "
		            << timestamp_data << __E__;

		datafile_[i] << stopIndex << " " << timestamp_data << std::endl;
		i++;
	}

	indicateIterationWork();
	return;
}

//========================================================================================================================
bool DTCFrontEndInterface::running(void)
{
	while(WorkLoop::continueWorkLoop_)
	{
		break;
	}

	return false;
}

//=====================================
void DTCFrontEndInterface::configureJitterAttenuator(void)
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

//========================================================================================================================
// rocRead
void DTCFrontEndInterface::ReadROC(__ARGS__)
{
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	DTCLib::DTC_Link_ID rocLinkIndex =
	    DTCLib::DTC_Link_ID(__GET_ARG_IN__("rocLinkIndex", uint8_t));
	uint8_t address = __GET_ARG_IN__("address", uint8_t);
	__FE_COUTV__(rocLinkIndex);
	__FE_COUTV__((unsigned int)address);

	// DTCLib::DTC* tmpDTC = new
	// DTCLib::DTC(DTCLib::DTC_SimMode_NoCFO,dtc_,roc_mask_,"");

	uint16_t readData = -999;

	for(auto& roc : rocs_)
	{
		__FE_COUT__ << "Found link ID " << roc.second->getLinkID() << " looking for "
		            << rocLinkIndex << __E__;

		if(rocLinkIndex == roc.second->getLinkID())
		{
			readData = roc.second->readRegister(address);

			// uint16_t readData = thisDTC_->ReadROCRegister(rocLinkIndex, address);
			// delete tmpDTC;

			//  char readDataStr[100];
			//  sprintf(readDataStr,"0x%X",readData);
			//  __SET_ARG_OUT__("readData",readDataStr);
			__SET_ARG_OUT__("readData", readData);

			// for(auto &argOut:argsOut)
			__FE_COUT__ << "readData"
			            << ": " << std::hex << readData << std::dec << __E__;
			return;
		}
	}

	__FE_SS__ << "ROC link ID " << rocLinkIndex << " not found!" << __E__;
	__FE_SS_THROW__;

}  // end ReadROC()

//========================================================================================================================
// DTCStatus
//	FEMacro 'DTCStatus' generated, Oct-22-2018 03:16:46, by 'admin' using
// MacroMaker. 	Macro Notes:
void DTCFrontEndInterface::WriteROC(__ARGS__)
{
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	DTCLib::DTC_Link_ID rocLinkIndex =
	    DTCLib::DTC_Link_ID(__GET_ARG_IN__("rocLinkIndex", uint8_t));
	uint8_t  address   = __GET_ARG_IN__("address", uint8_t);
	uint16_t writeData = __GET_ARG_IN__("writeData", uint16_t);

	__FE_COUTV__(rocLinkIndex);
	__FE_COUTV__((unsigned int)address);
	__FE_COUTV__(writeData);

	__FE_COUT__ << "ROCs size = " << rocs_.size() << __E__;


	for(auto& roc : rocs_)
	{
		__FE_COUT__ << "Found link ID " << roc.second->getLinkID() << " looking for "
		            << rocLinkIndex << __E__;

		if(rocLinkIndex == roc.second->getLinkID())
		{
			roc.second->writeRegister(address, writeData);

			for(auto& argOut : argsOut)
				__FE_COUT__ << argOut.first << ": " << argOut.second << __E__;

			return;
		}
	}

	__FE_SS__ << "ROC link ID " << rocLinkIndex << " not found!" << __E__;
	__FE_SS_THROW__;
}

//========================================================================================================================
void DTCFrontEndInterface::WriteROCBlock(__ARGS__)
{
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	// macro commands section

	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;

	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	DTCLib::DTC_Link_ID rocLinkIndex =
	    DTCLib::DTC_Link_ID(__GET_ARG_IN__("rocLinkIndex", uint8_t));
	uint8_t  address   = __GET_ARG_IN__("address", uint8_t);
	uint16_t writeData = __GET_ARG_IN__("writeData", uint16_t);
	uint8_t  block     = __GET_ARG_IN__("block", uint8_t);
	__FE_COUTV__(rocLinkIndex);
	__FE_COUT__ << "block = " << std::dec << (unsigned int)block << __E__;
	__FE_COUT__ << "address = 0x" << std::hex << (unsigned int)address << std::dec
	            << __E__;
	__FE_COUT__ << "writeData = 0x" << std::hex << writeData << std::dec << __E__;

	bool acknowledge_request = false;

	thisDTC_->WriteExtROCRegister(rocLinkIndex, block, address, writeData, acknowledge_request);

	for(auto& argOut : argsOut)
		__FE_COUT__ << argOut.first << ": " << argOut.second << __E__;
}

//========================================================================
void DTCFrontEndInterface::ReadROCBlock(__ARGS__)
{
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	// macro commands section
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;

	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	DTCLib::DTC_Link_ID rocLinkIndex =
	    DTCLib::DTC_Link_ID(__GET_ARG_IN__("rocLinkIndex", uint8_t));
	uint8_t address = __GET_ARG_IN__("address", uint8_t);
	uint8_t number_of_words   = __GET_ARG_IN__("numberOfWords", uint8_t);
	bool incrementAddress    = bool(__GET_ARG_IN__("incrementAddress", uint8_t));

	__FE_COUTV__(rocLinkIndex);
	__FE_COUT__ << "address = 0x" << std::hex << (unsigned int)address << std::dec << __E__;
	__FE_COUT__ << "numberOfWords = " << std::dec << (unsigned int)number_of_words << __E__;
	__FE_COUTV__(incrementAddress);

	//	uint16_t readData = thisDTC_->ReadExtROCRegister(rocLinkIndex, block, address);
	
	auto readData = thisDTC_->ReadROCBlock(rocLinkIndex, address, number_of_words, incrementAddress);

	std::ofstream datafile;

	std::stringstream filename;
	filename << "/home/mu2ehwdev/test_stand/ots/ReadROCBlock_data.txt";
	std::string filenamestring = filename.str();
	datafile.open(filenamestring);

	datafile << "link " << std::dec << rocLinkIndex << std::endl;
	datafile << "address 0x" << std::hex << address << std::endl;
	datafile << "increment address " << std::dec << incrementAddress << std::endl;
	datafile << "read " << std::dec << number_of_words << " words..." << std::endl;

	for (int i=0; i<number_of_words; i++) {
	        datafile << "read data [" << std::dec << i << "]  = 0x" << std::hex << readData[i] << std::endl;
		__FE_COUT__ << "read data [" << std::dec << i << "]  = 0x" << std::hex << readData[i] << __E__;
	}

	datafile.close();

	//__SET_ARG_OUT__("readData", readData);

	//for(auto& argOut : argsOut)
	//  __FE_COUT__ << argOut.first << ": " << argOut.second << __E__;
}

//========================================================================
void DTCFrontEndInterface::DTCHighRateDCSCheck(__ARGS__)
{
	unsigned int linkIndex = 	__GET_ARG_IN__("rocLinkIndex", unsigned int);
	unsigned int loops = 		__GET_ARG_IN__("loops",unsigned int);
	unsigned int baseAddress = 	__GET_ARG_IN__("baseAddress",unsigned int);
	unsigned int correctRegisterValue0 = 	__GET_ARG_IN__("correctRegisterValue0",unsigned int);
	unsigned int correctRegisterValue1 = 	__GET_ARG_IN__("correctRegisterValue1",unsigned int);

	__FE_COUTV__(linkIndex);
	__FE_COUTV__(loops);
	__FE_COUTV__(baseAddress);
	__FE_COUTV__(correctRegisterValue0);
	__FE_COUTV__(correctRegisterValue1);

	for(auto& roc : rocs_)
		if(roc.second->getLinkID() == linkIndex)
		{
			roc.second->highRateCheck(loops,baseAddress,correctRegisterValue0,correctRegisterValue1);
			return;
		}

	__FE_SS__ << "Error! Could not find ROC at link index " << linkIndex << __E__;
	__FE_SS_THROW__;

} //end DTCHighRateDCSCheck()

//========================================================================
void DTCFrontEndInterface::DTCReset(__ARGS__) { DTCReset(); }

//========================================================================
void DTCFrontEndInterface::DTCReset()
{
	{
		char* address = new char[universalAddressSize_]{
		    0};  //create address buffer of interface size and init to all 0
		char* data = new char[universalDataSize_]{
		    0};                 //create data buffer of interface size and init to all 0
		uint64_t macroAddress;  // create macro address buffer (size 8 bytes)
		uint64_t macroData;     // create macro address buffer (size 8 bytes)
		std::map<std::string /*arg name*/, uint64_t /*arg val*/>
		    macroArgs;  // create map from arg name to 64-bit number

		// command-#0: Write(0x9100 /*address*/,0xa0000000 /*data*/);
		macroAddress = 0x9100;
		memcpy(address, &macroAddress, 8);  // copy macro address to buffer
		macroData = 0xa0000000;
		memcpy(data, &macroData, 8);  // copy macro data to buffer
		universalWrite(address, data);

		// command-#1: Write(0x9118 /*address*/,0x0000003f /*data*/);
		macroAddress = 0x9118;
		memcpy(address, &macroAddress, 8);  // copy macro address to buffer
		macroData = 0x0000003f;
		memcpy(data, &macroData, 8);  // copy macro data to buffer
		universalWrite(address, data);

		// command-#2: Write(0x9100 /*address*/,0x00000000 /*data*/);
		macroAddress = 0x9100;
		memcpy(address, &macroAddress, 8);  // copy macro address to buffer
		macroData = 0x00000000;
		memcpy(data, &macroData, 8);  // copy macro data to buffer
		universalWrite(address, data);

		// command-#3: Write(0x9100 /*address*/,0x10000000 /*data*/);
		macroAddress = 0x9100;
		memcpy(address, &macroAddress, 8);  // copy macro address to buffer
		macroData = 0x10000000;
		memcpy(data, &macroData, 8);  // copy macro data to buffer
		universalWrite(address, data);

		// command-#4: Write(0x9100 /*address*/,0x30000000 /*data*/);
		macroAddress = 0x9100;
		memcpy(address, &macroAddress, 8);  // copy macro address to buffer
		macroData = 0x30000000;
		memcpy(data, &macroData, 8);  // copy macro data to buffer
		universalWrite(address, data);

		// command-#5: Write(0x9100 /*address*/,0x10000000 /*data*/);
		macroAddress = 0x9100;
		memcpy(address, &macroAddress, 8);  // copy macro address to buffer
		macroData = 0x10000000;
		memcpy(data, &macroData, 8);  // copy macro data to buffer
		universalWrite(address, data);

		// command-#6: Write(0x9118 /*address*/,0x00000000 /*data*/);
		macroAddress = 0x9118;
		memcpy(address, &macroAddress, 8);  // copy macro address to buffer
		macroData = 0x00000000;
		memcpy(data, &macroData, 8);  // copy macro data to buffer
		universalWrite(address, data);

		delete[] address;  // free the memory
		delete[] data;     // free the memory
	}
}

//========================================================================
void DTCFrontEndInterface::RunROCFEMacro(__ARGS__)
{
//	std::string feMacroName = __GET_ARG_IN__("ROC_FEMacroName", std::string);
//	std::string rocUID = __GET_ARG_IN__("ROC_UID", std::string);
//
//	__FE_COUTV__(feMacroName);
//	__FE_COUTV__(rocUID);

	auto feMacroIt = rocFEMacroMap_.find(feMacroStruct.feMacroName_);
	if(feMacroIt == rocFEMacroMap_.end())
	{
		__FE_SS__ << "Fatal error - ROC FE Macro name '" <<
				feMacroStruct.feMacroName_ <<
				"' not found in DTC's map!" << __E__;
		__FE_SS_THROW__;
	}

	const std::string& rocUID = feMacroIt->second.first;
	const std::string& rocFEMacroName = feMacroIt->second.second;

	__FE_COUTV__(rocUID);
	__FE_COUTV__(rocFEMacroName);

	auto rocIt = rocs_.find(rocUID);
	if(rocIt == rocs_.end())
	{
		__FE_SS__ << "Fatal error - ROC name '" <<
				rocUID <<
				"' not found in DTC's map!" << __E__;
		__FE_SS_THROW__;
	}

	rocIt->second->runSelfFrontEndMacro(rocFEMacroName,
			argsIn,argsOut);


} //end RunROCFEMacro()

DEFINE_OTS_INTERFACE(DTCFrontEndInterface)
