#include "otsdaq-mu2e/CFOandDTCCore/CFOandDTCCoreVInterface.h"
#include "otsdaq/Macros/BinaryStringMacros.h"
#include "otsdaq/Macros/InterfacePluginMacros.h"
#include "otsdaq/FECore/MakeInterface.h"
#include "otsdaq/TablePlugins/ARTDAQTableBase/ARTDAQTableBase.h"

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "CFOandDTCCoreVInterface"

std::string	CFOandDTCCoreVInterface::CONFIG_MODE_HARDWARE_DEV 	= "HardwareDevMode";
std::string	CFOandDTCCoreVInterface::CONFIG_MODE_EVENT_BUILDING = "EventBuildingMode";
std::string	CFOandDTCCoreVInterface::CONFIG_MODE_LOOPBACK 		= "LoopbackMode";
//=========================================================================================
CFOandDTCCoreVInterface::CFOandDTCCoreVInterface(
    const std::string&       interfaceUID,
    const ConfigurationTree& theXDAQContextConfigTree,
    const std::string&       interfaceConfigurationPath)
    : FEVInterface(interfaceUID, theXDAQContextConfigTree, interfaceConfigurationPath)
{
	__FE_COUT__ << "instantiate CFO-DTC_core... " << getInterfaceUID() << " "
	            << theXDAQContextConfigTree << " " << interfaceConfigurationPath << __E__;

	universalAddressSize_ = sizeof(dtc_address_t);
	universalDataSize_    = sizeof(dtc_data_t);

	configure_clock_ = getSelfNode().getNode("ConfigureClock").getValue<bool>();
	__FE_COUTV__(configure_clock_);

	artdaqMode_ = ARTDAQTableBase::isARTDAQEnabled(getConfigurationManager());
	__FE_COUTV__(artdaqMode_);

	operatingMode_ = "HardwareDevMode";  // choose default
	try
	{
		auto mu2eGlobalRecords =
			getConfigurationManager()->getNode("/Mu2eGlobalsTable").getChildren();
		if(mu2eGlobalRecords.size())  // take first record
			operatingMode_ = mu2eGlobalRecords[0]
									.second.getNode("GlobalOperatingMode")
									.getValue<std::string>();
	}
	catch(...)
	{
		__FE_COUT_WARN__ << "Ignoring missing Mu2eGlobalsTable definition, defaulting operating mode = " << operatingMode_ << __E__;
	}
	__FE_COUTV__(operatingMode_);

	skipInit_ = true; //default to skipping configure unless Mu2e Globals is set. This way MacroMaker mode will skip configure when the global table is missing.
	try
	{
		auto mu2eGlobalRecords =
			getConfigurationManager()->getNode("/Mu2eGlobalsTable").getChildren();
		if(mu2eGlobalRecords.size())  // take first record
			skipInit_ = mu2eGlobalRecords[0]
									.second.getNode("SkipCFOandDTCConfigureSteps")
									.getValue<bool>();
		
	}
	catch(const std::runtime_error& e)
	{
		__FE_COUT_WARN__ << "Ignoring missing Mu2eGlobalsTable definition, defaulting skipInit_ = " << skipInit_ << __E__;
	}
	__FE_COUTV__(skipInit_);

	
	// PCIe index to communicate with
	deviceIndex_ = getSelfNode().getNode("DeviceIndex").getValue<unsigned int>();
	__FE_COUTV__(deviceIndex_);

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

	// if(!emulatorMode_)
	// {
	// 	snprintf(devfile_, 11, "/dev/" MU2E_DEV_FILE, device_);
	// 	fd_ = open(devfile_, O_RDONLY);
	// }

	__FE_COUT__ << "Constructed." << __E__;
}  // end constructor()

//==========================================================================================
CFOandDTCCoreVInterface::~CFOandDTCCoreVInterface(void)
{
	// if(regWriteMonitorStream_.is_open()) 
	// {
	// 	regWriteMonitorStream_ << "Timestamp: " << std::dec << time(0) << 
	// 		", \t ---------- Destructed: " << device_name_ << "\n";
	// 	regWriteMonitorStream_.close();
	// }
	// if(runDataFile_.is_open()) runDataFile_.close();
	// if(fd_) close(fd_);	
	__FE_COUT__ << "Destructed." << __E__;
}  // end destructor()

//===========================================================================================
void CFOandDTCCoreVInterface::registerCFOandDTCFEMacros(void)
{	
	// clang-format off
	// registerFEMacroFunction(
	// 	"Get Firmware Version",  // feMacroName
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&CFOandDTCCoreVInterface::GetFirmwareVersion),  // feMacroFunction
	// 				std::vector<std::string>{},
	// 				std::vector<std::string>{"Firmware Version Date"},  // namesOfOutputArgs
	// 				1);//"allUsers:0 | TDAQ:255");
					
	// registerFEMacroFunction(
	// 	"Flash_LEDs",  // feMacroName
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&CFOandDTCCoreVInterface::FlashLEDs),  // feMacroFunction
	// 				std::vector<std::string>{},
	// 				std::vector<std::string>{},  // namesOfOutputArgs
	// 				1);                          // requiredUserPermissions
    
	// registerFEMacroFunction(
	// 	"Get Status",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&CFOandDTCCoreVInterface::GetStatus),            // feMacroFunction
	// 				std::vector<std::string>{},  // namesOfInputArgs
	// 				std::vector<std::string>{"Status"},
	// 				1);  // requiredUserPermissions

	// registerFEMacroFunction(
	// 	"Check Link Loss-of-Light",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&CFOandDTCCoreVInterface::GetLinkLossOfLight),            // feMacroFunction
	// 				std::vector<std::string>{},  // namesOfInputArgs
	// 				std::vector<std::string>{"Link Status"},
	// 				1);  // requiredUserPermissions

	// registerFEMacroFunction(
	// 	"Check Firefly Temperature",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&CFOandDTCCoreVInterface::GetFireflyTemperature),            // feMacroFunction
	// 				std::vector<std::string>{},  // namesOfInputArgs
	// 				std::vector<std::string>{"Temperature"},
	// 				1);  // requiredUserPermissions

	// registerFEMacroFunction(
	// 	"Reset Link Rx",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&CFOandDTCCoreVInterface::ResetLinkRx),
	// 			        std::vector<std::string>{"Link to Reset (0-7, 6/CFO, 7/EVB)"},
	// 					std::vector<std::string>{"Register Write Results"},
	// 				1);  // requiredUserPermissions
					
	// registerFEMacroFunction(
	// 	"Shutdown Link Tx",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&CFOandDTCCoreVInterface::ShutdownLinkTx),
	// 			        std::vector<std::string>{"Link to Shutdown (0-7, 6/CFO, 7/EVB)"},
	// 				std::vector<std::string>{						
	// 					"Reset Status",
	// 					"Link Reset Register"},
	// 				1);  // requiredUserPermissions
	// registerFEMacroFunction(
	// 	"Startup Link Tx",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&CFOandDTCCoreVInterface::StartupLinkTx),
	// 				std::vector<std::string>{"Link to Startup (0-7, 6/CFO, 7/EVB)"},
	// 				std::vector<std::string>{						
	// 					"Reset Status",
	// 					"Link Reset Register"},
	// 				1);  // requiredUserPermissions

	// registerFEMacroFunction(
	// 	"Shutdown Firefly Tx",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&CFOandDTCCoreVInterface::ShutdownFireflyTx),
	// 				std::vector<std::string>{"Link to Shutdown (0-7, 6/CFO, 7/EVB)"},
	// 				std::vector<std::string>{						
	// 					"Shutdown Status"},
	// 				1);  // requiredUserPermissions
	// registerFEMacroFunction(
	// 	"Startup Firefly Tx",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&CFOandDTCCoreVInterface::StartupFireflyTx),
	// 				std::vector<std::string>{"Link to Startup (0-7, 6/CFO, 7/EVB)"},
	// 				std::vector<std::string>{						
	// 					"Startup Status"},
	// 				1);  // requiredUserPermissions

	// clang-format on

	
} //end registerCFOandDTCFEMacros()

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
			returnValue[i] = (0xF0 | i) + rand() % 100;
		return;
	}

	// (*((dtc_data_t*)returnValue)) = thisDTC_->ReadRegister(*((dtc_address_t*)address));

	int errorCode = getDevice()->read_register(*((dtc_address_t*)address), 100, ((dtc_data_t*)returnValue));
	if (errorCode != 0)
	{
		__FE_SS__ << "Error reading register 0x" << std::hex << static_cast<uint32_t>(*((dtc_address_t*)address)) << " " << errorCode;
		__SS_THROW__;
	}
}  // end universalRead()

// //===========================================================================================
// // registerRead: return = value read from register at address "address"
// //
// dtc_data_t CFOandDTCCoreVInterface::registerRead(dtc_address_t address)
// { 	// lock mutex scope
// 	std::lock_guard<std::mutex> lock(readWriteOperationMutex_);
// 	reg_access_.access_type = 0;  // 0 = read, 1 = write
// 	reg_access_.reg_offset  = address;
// 	// __COUTV__(reg_access.reg_offset);

// 	if(ioctl(fd_, M_IOC_REG_ACCESS, &reg_access_))
// 	{
// 		__FE_SS__ << "ERROR: DTC register read - Does file exist? -> /dev/mu2e" << device_
// 		       << __E__;
// 		__SS_THROW__;
// 	}

// 	__FE_COUT__	<< time(0) << " READ  address: 0x" 	<< std::setw(4) << std::setprecision(4) << std::hex << reg_access_.reg_offset 
// 				<< " \t read-value:  0x" << std::setw(8) << std::setprecision(8) << std::hex << reg_access_.val << __E__;

// 	// if(reg_access_.val == 0xbf80c0c0)
// 	// 	__FE_COUT__ << StringMacros::stackTrace() << __E__;
// 	return reg_access_.val;
// }  // end registerRead() and unlock mutex scope


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

	int errorCode = getDevice()->write_register( *((dtc_address_t*)address), 100, *((dtc_data_t*)writeValue));
	if (errorCode != 0)
	{
		__FE_SS__ << "Error writing register 0x" << std::hex << static_cast<uint32_t>(*((dtc_address_t*)address)) << " " << errorCode;
		__SS_THROW__;
	}
}  // end universalWrite()

// //===============================================================================================
// // registerWrite: return = value readback from register at address "address"
// dtc_data_t CFOandDTCCoreVInterface::registerWrite(dtc_address_t address, dtc_data_t dataToWrite)
// {

// 	{// lock mutex scope
// 		std::lock_guard<std::mutex> lock(readWriteOperationMutex_);
// 		reg_access_.access_type = 1;  // 0 = read, 1 = write
// 		reg_access_.reg_offset  = address;
// 		reg_access_.val = dataToWrite;

// 		if(ioctl(fd_, M_IOC_REG_ACCESS, &reg_access_))
// 			__FE_COUT_ERR__ << "ERROR with register write - Does file exist? /dev/mu2e"
// 							<< device_ << __E__;
// 	} // unlock mutex scope

	
// 	//do DTC- and CFO-specific readback verification in DTCFrontEndInterface::registerWrite() and CFOFrontEndInterface::registerWrite()
// 	//	which should leverage common readback verification defined in CFOandDTCCoreVInterface::readbackVerify()

// 	__FE_COUT__	<< "WRITE address: 0x" 		<< std::setw(4) << std::setprecision(4) << std::hex << address 
// 				<< " \t write-value: 0x"	<< std::setw(8) << std::setprecision(8) << std::hex << dataToWrite << __E__;




// 	//--------------------------------------------------------
// 	//Monitor register writes (for debugging)
// 	regWriteMonitorStream_ << "Timestamp: " << std::dec << time(0) << ", \t " << "address: 0x" << 
// 		std::setw(4) << std::setprecision(4) << std::hex << address <<
// 		"," << " \t dataToWrite: 0x" << 
// 		std::setw(8) << std::setprecision(8) << std::hex << dataToWrite << "\n";
// 	regWriteMonitorStream_.flush();
// 	//end of Monitor register writes
// 	//--------------------------------------------------------

// 	return registerRead(address);
// }  // end registerWrite()

// //===============================================================================================
// // readbackVerify: throw exception if readbackValue does correspond to dataToWrite, for a given address
// void CFOandDTCCoreVInterface::readbackVerify(dtc_address_t address, dtc_data_t dataToWrite, dtc_data_t readbackValue)
// {
// 	switch(address)
// 	{
// 		case 0x9168: // lowest 16-bits are the I2C read value. So ignore in write validation			
// 		case 0x9298:
// 			dataToWrite		&= 0xffff0000; 
// 			readbackValue 	&= 0xffff0000; 
// 			break;
// 		case 0x93a0: // upper 16-bits are part of I2C operation. So ignore in write validation			
// 			dataToWrite		&= 0x0000ffff; 
// 			readbackValue 	&= 0x0000ffff; 
// 			break;
// 		case 0x9100: //bit 31 is reset bit, which is write only 
// 			dataToWrite		&= 0x7fffffff;
// 			readbackValue   &= 0x7fffffff; 
// 			break;
// 		default:; // do direct comparison
// 	} //end readback verification address case handling
	
// 	if(readbackValue != dataToWrite)
// 	{
// 		__FE_SS__ 	<< "write value 0x"	<< std::setw(8) << std::setprecision(8) << std::hex << dataToWrite
// 				<< " to register 0x" 	<< std::setw(4) << std::setprecision(4) << std::hex << address << 
// 				"... read back 0x"	 	<< std::setw(8) << std::setprecision(8) << std::hex << readbackValue <<
// 				"\n\n" << StringMacros::stackTrace() << __E__;
// 		__FE_SS_THROW__;
// 		// __FE_COUT_ERR__ << ss.str(); 
// 	}
// } //end readbackVerify()

// //==================================================================================================
// // turn on LEDs on front of timing card
// void CFOandDTCCoreVInterface::turnOnLED()
// {
// 	// bit[16-20] = 1
// 	registerWrite(0x9100, registerRead(0x9100) | 0x001f0000);
// } //end turnOnLED()

// //==================================================================================================
// // turn off LEDs on front of timing card
// void CFOandDTCCoreVInterface::turnOffLED()
// {
// 	// bit[16-20] = 0
// 	registerWrite(0x9100, registerRead(0x9100) & (~0x001f0000));
// } //end turnOffLED()


//
////==================================================================================================
// int CFOandDTCCoreVInterface::getROCLinkStatus(int ROC_link)
//{
//	int overall_link_status = registerRead(0x9140);
//
//	int ROC_link_status = (overall_link_status >> ROC_link) & 0x1;
//
//	return ROC_link_status;
//}
//
// int CFOandDTCCoreVInterface::getCFOLinkStatus()
//{
//	int overall_link_status = registerRead(0x9140);
//
//	int CFO_link_status = (overall_link_status >> 6) & 0x1;
//
//	return CFO_link_status;
//}

// std::string CFOandDTCCoreVInterface::printVoltages()
// {
// 	int adc_vccint  = registerRead(0x9014);
// 	int adc_vccaux  = registerRead(0x9018);
// 	int adc_vccbram = registerRead(0x901c);

// 	float volt_vccint  = ((float)adc_vccint / 4095.) * 3.0;
// 	float volt_vccaux  = ((float)adc_vccaux / 4095.) * 3.0;
// 	float volt_vccbram = ((float)adc_vccbram / 4095.) * 3.0;

// 	std::stringstream ss;
// 	ss << device_name_ << " VCCINT. = " << volt_vccint << " V" << __E__;
// 	ss << device_name_ << " VCCAUX. = " << volt_vccaux << " V" << __E__;
// 	ss << device_name_ << " VCCBRAM = " << volt_vccbram << " V" << __E__;
// 	__FE_COUT__ << ss.str();

// 	return ss.str();
// } //end printVoltages()
//
// int CFOandDTCCoreVInterface::checkLinkStatus()
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
// bool CFOandDTCCoreVInterface::ROCActive(unsigned ROC_link)
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
// void CFOandDTCCoreVInterface::configure(void) try
//{
//	__FE_COUTV__(getIterationIndex());
//	__FE_COUTV__(getSubIterationIndex());
//
//}  // end configure()
// catch(const std::runtime_error& e)
//{
//	__FE_SS__ << "Error caught: " << e.what() << __E__;
//	__FE_SS_THROW__;
//}
// catch(...)
//{
//	__FE_SS__ << "Unknown error caught. Check the printouts!" << __E__;
//	__FE_SS_THROW__;
//}
//
////==============================================================================
// void CFOandDTCCoreVInterface::halt(void)
//{
//}
//
////==============================================================================
// void CFOandDTCCoreVInterface::pause(void)
//{
//}
//
////==============================================================================
// void CFOandDTCCoreVInterface::resume(void)
//{
//}
//
////==============================================================================
// void CFOandDTCCoreVInterface::start(std::string runNumber)
//{
//}
//
////==============================================================================
// void CFOandDTCCoreVInterface::stop(void)
//{
//}
//
////==============================================================================
// bool CFOandDTCCoreVInterface::running(void)
//{
//	return true; //true to end loop
//}

// //=====================================
// void CFOandDTCCoreVInterface::configureJitterAttenuator(void)
// {
// 	// Start configuration preamble
// 	// set page B
// 	registerWrite(0x9168, 0x68010B00);
// 	registerWrite(0x916c, 0x00000001);

// 	// page B registers
// 	registerWrite(0x9168, 0x6824C000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68250000);
// 	registerWrite(0x916c, 0x00000001);

// 	// set page 5
// 	registerWrite(0x9168, 0x68010500);
// 	registerWrite(0x916c, 0x00000001);

// 	// page 5 registers
// 	registerWrite(0x9168, 0x68400100);
// 	registerWrite(0x916c, 0x00000001);

// 	// End configuration preamble
// 	//
// 	// Delay 300 msec
// 	usleep(300000 /*300ms*/); 

// 	// Delay is worst case time for device to complete any calibration
// 	// that is running due to device state change previous to this script
// 	// being processed.
// 	//
// 	// Start configuration registers
// 	// set page 0
// 	registerWrite(0x9168, 0x68010000);
// 	registerWrite(0x916c, 0x00000001);

// 	// page 0 registers
// 	registerWrite(0x9168, 0x68060000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68070000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68080000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680B6800);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68160200);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x6817DC00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68180000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x6819DD00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x681ADF00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682B0200);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682C0F00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682D5500);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682E3700);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682F0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68303700);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68310000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68323700);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68330000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68343700);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68350000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68363700);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68370000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68383700);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68390000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683A3700);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683B0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683C3700);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683D0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683FFF00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68400400);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68410E00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68420E00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68430E00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68440E00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68450C00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68463200);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68473200);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68483200);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68493200);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x684A3200);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x684B3200);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x684C3200);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x684D3200);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x684E5500);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x684F5500);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68500F00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68510300);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68520300);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68530300);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68540300);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68550300);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68560300);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68570300);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68580300);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68595500);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x685AAA00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x685BAA00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x685C0A00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x685D0100);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x685EAA00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x685FAA00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68600A00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68610100);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x6862AA00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x6863AA00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68640A00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68650100);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x6866AA00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x6867AA00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68680A00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68690100);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68920200);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x6893A000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68950000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68968000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68986000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x689A0200);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x689B6000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x689D0800);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x689E4000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68A02000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68A20000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68A98A00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68AA6100);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68AB0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68AC0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68E52100);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68EA0A00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68EB6000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68EC0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68ED0000);
// 	registerWrite(0x916c, 0x00000001);

// 	// set page 1
// 	registerWrite(0x9168, 0x68010100);
// 	registerWrite(0x916c, 0x00000001);

// 	// page 1 registers
// 	registerWrite(0x9168, 0x68020100);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68120600);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68130900);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68143B00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68152800);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68170600);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68180900);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68193B00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x681A2800);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683F1000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68400000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68414000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x6842FF00);
// 	registerWrite(0x916c, 0x00000001);

// 	// set page 2
// 	registerWrite(0x9168, 0x68010200);
// 	registerWrite(0x916c, 0x00000001);

// 	// page 2 registers
// 	registerWrite(0x9168, 0x68060000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68086400);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68090000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680A0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680B0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680C0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680D0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680E0100);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680F0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68100000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68110000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68126400);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68130000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68140000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68150000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68160000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68170000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68180100);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68190000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x681A0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x681B0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x681C6400);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x681D0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x681E0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x681F0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68200000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68210000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68220100);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68230000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68240000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68250000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68266400);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68270000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68280000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68290000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682A0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682B0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682C0100);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682D0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682E0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682F0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68310B00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68320B00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68330B00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68340B00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68350000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68360000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68370000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68388000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x6839D400);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683A0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683B0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683C0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683D0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683EC000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68500000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68510000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68520000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68530000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68540000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68550000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x686B5200);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x686C6500);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x686D7600);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x686E3100);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x686F2000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68702000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68712000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68722000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x688A0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x688B0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x688C0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x688D0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x688E0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x688F0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68900000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68910000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x6894B000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68960200);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68970200);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68990200);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x689DFA00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x689E0100);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x689F0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68A9CC00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68AA0400);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68AB0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68B7FF00);
// 	registerWrite(0x916c, 0x00000001);

// 	// set page 3
// 	registerWrite(0x9168, 0x68010300);
// 	registerWrite(0x916c, 0x00000001);

// 	// page 3 registers
// 	registerWrite(0x9168, 0x68020000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68030000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68040000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68050000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68061100);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68070000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68080000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68090000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680A0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680B8000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680C0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680D0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680E0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680F0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68100000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68110000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68120000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68130000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68140000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68150000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68160000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68170000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68380000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68391F00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683B0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683C0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683D0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683E0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683F0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68400000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68410000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68420000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68430000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68440000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68450000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68460000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68590000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x685A0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x685B0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x685C0000);
// 	registerWrite(0x916c, 0x00000001);

// 	// set page 4
// 	registerWrite(0x9168, 0x68010400);
// 	registerWrite(0x916c, 0x00000001);

// 	// page 4 registers
// 	registerWrite(0x9168, 0x68870100);
// 	registerWrite(0x916c, 0x00000001);

// 	// set page 5
// 	registerWrite(0x9168, 0x68010500);
// 	registerWrite(0x916c, 0x00000001);

// 	// page 5 registers
// 	registerWrite(0x9168, 0x68081000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68091F00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680A0C00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680B0B00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680C3F00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680D3F00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680E1300);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680F2700);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68100900);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68110800);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68123F00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68133F00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68150000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68160000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68170000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68180000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x6819A800);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x681A0200);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x681B0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x681C0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x681D0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x681E0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x681F8000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68212B00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682A0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682B0100);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682C8700);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682D0300);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682E1900);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682F1900);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68310000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68324200);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68330300);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68340000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68350000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68360000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68370000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68380000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68390000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683A0200);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683B0300);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683C0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683D1100);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683E0600);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68890D00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x688A0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x689BFA00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x689D1000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x689E2100);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x689F0C00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68A00B00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68A13F00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68A23F00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68A60300);
// 	registerWrite(0x916c, 0x00000001);

// 	// set page 8
// 	registerWrite(0x9168, 0x68010800);
// 	registerWrite(0x916c, 0x00000001);

// 	// page 8 registers
// 	registerWrite(0x9168, 0x68023500);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68030500);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68040000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68050000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68060000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68070000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68080000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68090000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680A0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680B0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680C0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680D0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680E0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x680F0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68100000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68110000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68120000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68130000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68140000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68150000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68160000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68170000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68180000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68190000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x681A0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x681B0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x681C0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x681D0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x681E0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x681F0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68200000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68210000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68220000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68230000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68240000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68250000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68260000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68270000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68280000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68290000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682A0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682B0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682C0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682D0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682E0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x682F0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68300000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68310000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68320000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68330000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68340000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68350000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68360000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68370000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68380000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68390000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683A0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683B0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683C0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683D0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683E0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x683F0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68400000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68410000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68420000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68430000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68440000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68450000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68460000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68470000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68480000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68490000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x684A0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x684B0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x684C0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x684D0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x684E0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x684F0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68500000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68510000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68520000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68530000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68540000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68550000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68560000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68570000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68580000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68590000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x685A0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x685B0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x685C0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x685D0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x685E0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x685F0000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68600000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68610000);
// 	registerWrite(0x916c, 0x00000001);

// 	// set page 9
// 	registerWrite(0x9168, 0x68010900);
// 	registerWrite(0x916c, 0x00000001);

// 	// page 9 registers
// 	registerWrite(0x9168, 0x680E0200);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68430100);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68490F00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x684A0F00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x684E4900);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x684F0200);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x685E0000);
// 	registerWrite(0x916c, 0x00000001);

// 	// set page A
// 	registerWrite(0x9168, 0x68010A00);
// 	registerWrite(0x916c, 0x00000001);

// 	// page A registers
// 	registerWrite(0x9168, 0x68020000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68030100);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68040100);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68050100);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68140000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x681A0000);
// 	registerWrite(0x916c, 0x00000001);

// 	// set page B
// 	registerWrite(0x9168, 0x68010B00);
// 	registerWrite(0x916c, 0x00000001);

// 	// page B registers
// 	registerWrite(0x9168, 0x68442F00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68460000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68470000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68480000);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x684A0200);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68570E00);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68580100);
// 	registerWrite(0x916c, 0x00000001);

// 	// End configuration registers
// 	//
// 	// Start configuration postamble
// 	// set page 5
// 	registerWrite(0x9168, 0x68010500);
// 	registerWrite(0x916c, 0x00000001);

// 	// page 5 registers
// 	registerWrite(0x9168, 0x68140100);
// 	registerWrite(0x916c, 0x00000001);

// 	// set page 0
// 	registerWrite(0x9168, 0x68010000);
// 	registerWrite(0x916c, 0x00000001);

// 	// page 0 registers
// 	registerWrite(0x9168, 0x681C0100);
// 	registerWrite(0x916c, 0x00000001);

// 	// set page 5
// 	registerWrite(0x9168, 0x68010500);
// 	registerWrite(0x916c, 0x00000001);

// 	// page 5 registers
// 	registerWrite(0x9168, 0x68400000);
// 	registerWrite(0x916c, 0x00000001);

// 	// set page B
// 	registerWrite(0x9168, 0x68010B00);
// 	registerWrite(0x916c, 0x00000001);

// 	// page B registers
// 	registerWrite(0x9168, 0x6824C300);
// 	registerWrite(0x916c, 0x00000001);

// 	registerWrite(0x9168, 0x68250200);
// 	registerWrite(0x916c, 0x00000001);

// 	// End configuration postamble

// 	return;
// }


// //==============================================================================
// // GetFirmwareVersion
// std::string CFOandDTCCoreVInterface::GetFirmwareVersion()
// {
// 	return ReadDesignDate
// 	dtc_data_t readData = registerRead(0x9004); 
// 	// __FE_COUTV__(readData);
	
// 	std::stringstream dateSs;
// 	std::vector<std::string> months({"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"});
// 	int mon =  ((readData>>20)&0xF)*10 + ((readData>>16)&0xF);
// 	dateSs << months[mon-1] << "/" << 
// 		((readData>>12)&0xF) << ((readData>>8)&0xF) << "/20" << 
// 		((readData>>28)&0xF) << ((readData>>24)&0xF) << " " <<
// 		((readData>>4)&0xF) << ((readData>>0)&0xF) << ":00   raw-data: 0x" << std::hex << readData << __E__;
	
// 	return dateSs.str();	 
// }  // end GetFirmwareVersion()
// //========================================================================
// void CFOandDTCCoreVInterface::GetStatus(__ARGS__)
// {	
// 	//call virtual readStatus
// 	__SET_ARG_OUT__("Status",readStatus());
// } //end GetStatus()

// //========================================================================
// void CFOandDTCCoreVInterface::GetLinkLossOfLight(__ARGS__)
// {	
// 	std::stringstream rd;


// 	//do initial set of writes to get the live read of loss-of-light status (because it is latched value from last read)

// 	// #Read Firefly RX LOS registers
// 	// #enable IIC on Firefly
// 	// my_cntl write 0x93a0 0x00000200
// 	registerWrite(0x93a0,0x00000200);
// 	// #Device address, register address, null, null
// 	// my_cntl write 0x9298 0x54080000
// 	registerWrite(0x9298,0x54080000);
// 	// #read enable
// 	// my_cntl write 0x929c 0x00000002
// 	registerWrite(0x929c,0x00000002);
// 	// #disable IIC on Firefly
// 	// my_cntl write 0x93a0 0x00000000
// 	registerWrite(0x93a0,0x00000000);
// 	// #read data: Device address, register address, null, value
// 	// my_cntl read 0x9298

// 	// #{EVB, ROC4, ROC1, CFO, unused, ROC5, unused, unused}
// 	usleep(1000*100);

// 	// #Read Firefly RX LOS registers
// 	// my_cntl write 0x93a0 0x00000200
// 	registerWrite(0x93a0,0x00000200);
// 	// my_cntl write 0x9298 0x54070000
// 	registerWrite(0x9298,0x54070000);
// 	// my_cntl write 0x929c 0x00000002
// 	registerWrite(0x929c,0x00000002);
// 	// my_cntl write 0x93a0 0x00000000
// 	registerWrite(0x93a0,0x00000000);
// 	// my_cntl read 0x9298

// 	//END do initial set of writes to get the live read of loss-of-light status (because it is latched value from last read)

// 	dtc_data_t val=0, val2=0;
// 	for(int i=0;i<5;++i)
// 	{
// 		usleep(1000*100 /* 100 ms */);
// 		registerWrite(0x93a0,0x00000200);
// 		registerWrite(0x9298,0x54080000);
// 		registerWrite(0x929c,0x00000002);
// 		registerWrite(0x93a0,0x00000000);
// 		val |= registerRead(0x9298); //OR := if ever 1, mark dead

	
		
// 		usleep(1000*100 /* 100 ms */);
// 		//repeat set of writes, do the live read of loss-of-light status (because it is latched value from last read)
// 		registerWrite(0x93a0,0x00000200);
// 		registerWrite(0x9298,0x54070000);
// 		registerWrite(0x929c,0x00000002);
// 		registerWrite(0x93a0,0x00000000);
// 	 	val2 |= registerRead(0x9298); //OR := if ever 1, mark dead
// 	} //end multi-read to check for strange value changing

// 	// #ROC0 bit 3
// 	rd << "{0:" << (((val2>>(0+3))&1)?"DEAD":"OK");
// 	// Link 1 #ROC1 bit 5
// 	rd << ", 1: " << (((val>>(0+5))&1)?"DEAD":"OK");
// 	// #ROC2 bit 2
// 	rd << ", 2:" << (((val2>>(0+2))&1)?"DEAD":"OK");
// 	// #ROC3 bit 0
// 	rd << ", 3:" << (((val2>>(0+0))&1)?"DEAD":"OK");
// 	// #ROC4 bit 6
// 	rd << ", 4: " << (((val>>(0+6))&1)?"DEAD":"OK");
// 	// #ROC5 bit 1
// 	rd << ", 5: " << (((val>>(0+1))&1)?"DEAD":"OK");
// 	// #CFO bit 4
// 	rd << ", 6/CFO: " << (((val>>(0+4))&1)?"DEAD":"OK");
// 	// #EVB bit 7  Are EVB and CFO reversed?
// 	rd << ", 7/EVB: " << (((val>>(0+7))&1)?"DEAD":"OK") << "}";





// 	__SET_ARG_OUT__("Link Status",rd.str());
// } //end GetLinkLossOfLight()

// //========================================================================
// void CFOandDTCCoreVInterface::GetFireflyTemperature(__ARGS__)
// {	
// 	std::stringstream rd;

// 	// #Read Firefly RX temp registers
// 	// #enable IIC on Firefly
// 	// my_cntl write 0x93a0 0x00000200
// 	registerWrite(0x93a0,0x00000200);
// 	// #Device address, register address, null, null
// 	// my_cntl write 0x9298 0x54160000
// 	registerWrite(0x9298,0x54160000);
// 	// #read enable
// 	// my_cntl write 0x929c 0x00000002
// 	registerWrite(0x929c,0x00000002);
// 	// #disable IIC on Firefly
// 	// my_cntl write 0x93a0 0x00000000
// 	registerWrite(0x93a0,0x00000000);
// 	// #read data: Device address, register address, null, temp in 2's compl.
// 	// my_cntl read 0x9298
// 	dtc_data_t val = registerRead(0x9298) & 0x0FF;
// 	rd << "Celsius: " << val << ", Fahrenheit: " << val*9/5 + 32 << ", " << (val < 65?"GOOD":"BAD");

// 	__SET_ARG_OUT__("Temperature",rd.str());
// } //end GetFireflyTemperature()


// //========================================================================
// void CFOandDTCCoreVInterface::ResetLinkRx(__ARGS__)
// {	
// 	uint32_t link = __GET_ARG_IN__(argsIn[0].first /* first arg name */, uint32_t);
// 	link %= 8;
// 	__FE_COUTV__((unsigned int)link);

// 	// 0x9118 controls link resets
// 	//	bit-7:0 SERDES reset
// 	//	bit-15:8 PLL reset
// 	//	bit-23:16 RX reset
// 	//	bit-31:24 TX reset

// 	char reg_0x9118[100];
// 	uint32_t val = registerRead(0x9118);
// 	std::stringstream results;

// 	sprintf(reg_0x9118,"0x%8.8X",val); __FE_COUTV__(reg_0x9118);
// 	results << "reg_0x9118 Starting Value: " << reg_0x9118 << __E__;

// 	val |= 1 << (16 + link);
// 	sprintf(reg_0x9118,"0x%8.8X",val); __FE_COUTV__(reg_0x9118);
// 	results << "Link " << link << " RX reset: " << reg_0x9118 << __E__;
// 	registerWrite(0x9118, val);  // RX reset

// 	sleep(1);

// 	val &= ~(1 << (16 + link));
// 	sprintf(reg_0x9118,"0x%8.8X",val); __FE_COUTV__(reg_0x9118);
// 	results << "Link " << link << " RX unreset: " << reg_0x9118 << __E__;
// 	registerWrite(0x9118, val);  // RX unreset

// 	sleep(1);

// 	val |= 1 << (8 + link);
// 	sprintf(reg_0x9118,"0x%8.8X",val); __FE_COUTV__(reg_0x9118);
// 	results << "Link " << link << " PLL reset: " << reg_0x9118 << __E__;
// 	registerWrite(0x9118, val);  // PLL reset

// 	sleep(1);

// 	val &= ~(1 << (8 + link));
// 	sprintf(reg_0x9118,"0x%8.8X",val); __FE_COUTV__(reg_0x9118);
// 	results << "Link " << link << " PLL unreset: " << reg_0x9118 << __E__;
// 	registerWrite(0x9118, val);  // PLL unreset

// 	__SET_ARG_OUT__("Register Write Results",results.str());
// 	__FE_COUT__ << "Done with reset link Rx: " << link << __E__;

// } //end ResetLinkRx()

// //========================================================================
// // first arg must be link index or '*'
// void CFOandDTCCoreVInterface::ShutdownLinkTx(__ARGS__)
// {	
// 	uint32_t link = __GET_ARG_IN__(argsIn[0].first /* first arg name */, uint32_t);
// 	link %= 8;

// 	std::string linkStr = __GET_ARG_IN__(argsIn[0].first /* first arg name */, std::string);
// 	if(linkStr == "*")
// 	{
// 		//do all links!
// 		__FE_COUT__ << "* found, so doing all links!" << __E__;
// 		link = (0xFF<<24);
// 	}
// 	else
// 	{
// 		__FE_COUTV__((unsigned int)link);
// 		link = (1<<(24+link));
// 	}
	
// 	//0x9118 controls link resets
// 	//	bit-7:0 SERDES reset
// 	//	bit-15:8 PLL reset
// 	//	bit-23:16 RX reset
// 	//	bit-31:24 TX reset
	
// 	registerWrite(0x9118,link);  
	
// 	uint32_t val = registerRead(0x9118); 
	
// 	std::stringstream rd;
// 	rd << "Link " << link << " SERDES reset (" << (((val>>(0+link))&1)?"RESET":"Not Reset");
// 	rd << "), \nLink " << link << "PLL reset (" << (((val>>(8+link))&1)?"RESET":"Not Reset");
// 	rd << "), \nLink " << link << "RX reset (" << (((val>>(16+link))&1)?"RESET":"Not Reset");
// 	rd << "), \nLink " << link << "TX reset (" << (((val>>(24+link))&1)?"RESET":"Not Reset");	
// 	rd << ")";
// 	__SET_ARG_OUT__("Reset Status",rd.str());
	
// 	char readDataStr[100];
// 	sprintf(readDataStr,"0x%8.8X",val);
// 	__SET_ARG_OUT__("Link Reset Register",readDataStr);
	
// } //end ShutdownLinkTx()

// //========================================================================
// // first arg must be link index or '*'
// void CFOandDTCCoreVInterface::StartupLinkTx(__ARGS__)
// {	
// 	uint32_t link = __GET_ARG_IN__(argsIn[0].first /* first arg name */, uint32_t);
// 	link %= 8;

// 	std::string linkStr = __GET_ARG_IN__(argsIn[0].first /* first arg name */, std::string);
// 	if(linkStr == "*")
// 	{
// 		//do all links!
// 		__FE_COUT__ << "* found, so doing all links!" << __E__;
// 		link = (0xFF<<24);
// 	}
// 	else
// 	{
// 		__FE_COUTV__((unsigned int)link);
// 		link = (1<<(24+link));
// 	}
	
// 	//0x9118 controls link resets
// 	//	bit-7:0 SERDES reset
// 	//	bit-15:8 PLL reset
// 	//	bit-23:16 RX reset
// 	//	bit-31:24 TX reset
	
// 	uint32_t val = registerRead(0x9118); 
// 	uint32_t mask = ~link;
	
// 	registerWrite(0x9118, val&mask);  
	
// 	val = registerRead(0x9118); 
	
// 	std::stringstream rd;
// 	rd << "Control Link SERDES reset (" << (((val>>(0+6))&1)?"RESET":"Not Reset");
// 	rd << "), \nControl Link PLL reset (" << (((val>>(8+6))&1)?"RESET":"Not Reset");
// 	rd << "), \nControl Link RX reset (" << (((val>>(16+6))&1)?"RESET":"Not Reset");
// 	rd << "), \nControl Link TX reset (" << (((val>>(24+6))&1)?"RESET":"Not Reset");
// 	rd << ")";
// 	__SET_ARG_OUT__("Reset Status",rd.str());
	
// 	char readDataStr[100];
// 	sprintf(readDataStr,"0x%8.8X",val);
// 	__SET_ARG_OUT__("Link Reset Register",readDataStr);
	
// } //end StartupLinkTx()

// //========================================================================
// // first arg must be link index or '*'
// void CFOandDTCCoreVInterface::ShutdownFireflyTx(__ARGS__)
// {	
// 	uint32_t link = __GET_ARG_IN__(argsIn[0].first /* first arg name */, uint32_t);
// 	link %= 8;

// 	std::string linkStr = __GET_ARG_IN__(argsIn[0].first /* first arg name */, std::string);
// 	if(linkStr == "*")
// 	{
// 		//do all links!
// 		__FE_COUT__ << "* found, so doing all links!" << __E__;
// 		link = (0xFF<<8);
// 	}
// 	else
// 	{
// 		__FE_COUTV__((unsigned int)link);
// 		link = (1<<(8+link));
// 	}
	
// 	// #turn off Firefly TX
// 	// my_cntl write 0x93a0 0x00000100
// 	// my_cntl write 0x9288 0x5052ff00
// 	// my_cntl write 0x928c 0x00000001
// 	// my_cntl write 0x93a0 0x00000000
// 	// #my_cntl read 0x9288
// 	registerWrite(0x93a0,0x00000100);
// 	registerWrite(0x9288,0x50530000 | link);
// 	registerWrite(0x928c,0x00000001);
// 	registerWrite(0x93a0,0x00000000);

// 	// my_cntl write 0x93a0 0x00000100
// 	// my_cntl write 0x9288 0x5053ff00
// 	// my_cntl write 0x928c 0x00000001
// 	// my_cntl write 0x93a0 0x00000000
// 	// #my_cntl read 0x9288
// 	usleep(1000*100 /* 100 ms */);
// 	registerWrite(0x93a0,0x00000100);
// 	registerWrite(0x9288,0x50530000 | link);
// 	registerWrite(0x928c,0x00000001);
// 	registerWrite(0x93a0,0x00000000);
	
// 	usleep(1000*100 /* 100 ms */);
// 	uint32_t val = registerRead(0x9288); 
	
// 	std::stringstream rd;
// 	rd << "Link shutdown 0x" << std::hex << link << 
// 		" result 0x" << (val & 0x0FF) << std::dec; 
// 	__SET_ARG_OUT__("Shutdown Status",rd.str());
	
// } //end ShutdownFireflyTx()

// //========================================================================
// // first arg must be link index or '*'
// void CFOandDTCCoreVInterface::StartupFireflyTx(__ARGS__)
// {	
// 	uint32_t link = __GET_ARG_IN__(argsIn[0].first /* first arg name */, uint32_t);
// 	link %= 8;

// 	std::string linkStr = __GET_ARG_IN__(argsIn[0].first /* first arg name */, std::string);
// 	if(linkStr == "*")
// 	{
// 		//do all links!
// 		__FE_COUT__ << "* found, so doing all links!" << __E__;
// 		link = (0xFF<<8);
// 	}
// 	else
// 	{
// 		__FE_COUTV__((unsigned int)link);
// 		link = (1<<(8+link));
// 	}
	

// 	uint32_t val = registerRead(0x9288); 
// 	uint32_t mask = ~link;	

// 	// #turn on Firefly TX
// 	// my_cntl write 0x93a0 0x00000100
// 	// my_cntl write 0x9288 0x50520000
// 	// my_cntl write 0x928c 0x00000001
// 	// my_cntl write 0x93a0 0x00000000
// 	// #my_cntl read 0x9288
// 	registerWrite(0x93a0,0x00000100);
// 	registerWrite(0x9288,0x50520000 | (val&mask));
// 	registerWrite(0x928c,0x00000001);
// 	registerWrite(0x93a0,0x00000000);

// 	// my_cntl write 0x93a0 0x00000100
// 	// my_cntl write 0x9288 0x50530000
// 	// my_cntl write 0x928c 0x00000001
// 	// my_cntl write 0x93a0 0x00000000
// 	// #my_cntl read 0x9288
// 	usleep(1000*100 /* 100 ms */);
// 	registerWrite(0x93a0,0x00000100);
// 	registerWrite(0x9288,0x50530000 | (val&mask));
// 	registerWrite(0x928c,0x00000001);
// 	registerWrite(0x93a0,0x00000000);
	
// 	usleep(1000*100 /* 100 ms */);
// 	val = registerRead(0x9288); 
	
// 	std::stringstream rd;
// 	rd << "Link startup 0x" << std::hex << link << 
// 		" result 0x" << (val & 0x0FF) << std::dec; 
// 	__SET_ARG_OUT__("Startup Status",rd.str());
	
// } //end StartupFireflyTx()
