#include "otsdaq-mu2e/FEInterfaces/ROCDTCHardwareEmulated.h"
#include "otsdaq/Macros/InterfacePluginMacros.h"

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "FE-ROCDTCHardwareEmulated"

//=========================================================================================
ROCDTCHardwareEmulated::ROCDTCHardwareEmulated(
    const std::string&       rocUID,
    const ConfigurationTree& theXDAQContextConfigTree,
    const std::string&       theConfigurationPath)
    : ROCCoreVInterface(rocUID, theXDAQContextConfigTree, theConfigurationPath)
{
	INIT_MF("." /*directory used is USER_DATA/LOG/.*/);

	__MCOUT_INFO__("ROCDTCHardwareEmulated instantiated with link: "
	               << linkID_ << " and EventWindowDelayOffset = " << delay_ << __E__);
}  // end constructor()

//==========================================================================================
ROCDTCHardwareEmulated::~ROCDTCHardwareEmulated(void)
{
	// NOTE:: be careful not to call __FE_COUT__ decoration because it uses the
	// tree and it may already be destructed partially
	__COUT__ << FEVInterface::interfaceUID_ << "Destructed." << __E__;
}  // end destructor()

//==================================================================================================
uint16_t ROCDTCHardwareEmulated::readEmulatorRegister(uint16_t address)
{
	__FE_COUT__ << "Calling read emulator ROC register: link number " << std::dec
	            << linkID_ << ", address = " << address << __E__;
	if(address == 6)
		return 4860;
	else if(address == 7)
		return delay_;
	else
		return address + (time(NULL)&0xF);
	return -1;
}  // end readEmulatorRegister
//==================================================================================================
void ROCDTCHardwareEmulated::readEmulatorBlock(std::vector<uint16_t>& data,
                                               uint16_t               address,
                                               uint16_t               numberOfReads,
                                               bool                   incrementAddress)
{
	__FE_COUT__ << "Calling read emulator BLOCK: link number " << std::dec << linkID_
	            << ", address = " << address << ", numberOfReads = " << numberOfReads
	            << __E__;
	//	if(address == 6)
	//		return 4860;
	//	else if(address == 7)
	//		return delay_;
	return;
}  // end readEmulatorBlock

//==================================================================================================
void ROCDTCHardwareEmulated::writeROCRegister(uint16_t address, uint16_t data_to_write)
{
	__FE_COUT__ << "Calling write ROC register: link number " << std::dec << linkID_
	            << ", address = " << address << ", write data = " << data_to_write
	            << __E__;

	bool acknowledge_request = false;

	thisDTC_->WriteROCRegister(linkID_, address, data_to_write, acknowledge_request, 0);

}  // end writeROCRegister()

//==================================================================================================
uint16_t ROCDTCHardwareEmulated::readROCRegister(uint16_t address)
{
	__FE_COUT__ << "Calling read ROC register: link number " << std::dec << linkID_
	            << ", address = " << address << __E__;

	uint16_t read_data = 0;

	try
	{
		read_data = thisDTC_->ReadROCRegister(linkID_, address, 1);
	}
	catch(...)
	{
		__FE_COUT_ERR__ << "DTC failed DCS read" << __E__;
		read_data = -999;
	}

	return read_data;
}  // end readROCRegister()

//==================================================================================================
int ROCDTCHardwareEmulated::readTimestamp() { return this->readRegister(12); }

//==================================================================================================
void ROCDTCHardwareEmulated::writeDelay(uint16_t delay)
{
	this->writeRegister(21, delay);
	return;
}

//==================================================================================================
int ROCDTCHardwareEmulated::readDelay() { return this->readRegister(7); }

//==================================================================================================
int ROCDTCHardwareEmulated::readDTCLinkLossCounter() { return this->readRegister(8); }

//==================================================================================================
void ROCDTCHardwareEmulated::resetDTCLinkLossCounter()
{
	this->writeRegister(24, 0x1);
	return;
}

//==================================================================================================
void ROCDTCHardwareEmulated::configure(void) try
{
	if(emulatorMode_)
	{
		__FE_COUT__ << "Emulator ROC configuring..." << __E__;
		return;
	}

}  // end configure()
catch(const std::runtime_error& e)
{
	__FE_MOUT__ << "Error caught: " << e.what() << __E__;
	throw;
}
catch(...)
{
	__FE_SS__ << "Unknown error caught. Check printouts!" << __E__;
	__FE_MOUT__ << ss.str();
	__FE_SS_THROW__;
}  // end configure() catch

//==============================================================================
void ROCDTCHardwareEmulated::halt(void) {}

//==============================================================================
void ROCDTCHardwareEmulated::pause(void) {}

//==============================================================================
void ROCDTCHardwareEmulated::resume(void) {}

//==============================================================================
void ROCDTCHardwareEmulated::start(std::string)  // runNumber)
{
}

//==============================================================================
void ROCDTCHardwareEmulated::stop(void) {}

//==============================================================================
bool ROCDTCHardwareEmulated::running(void) { return false; }

DEFINE_OTS_INTERFACE(ROCDTCHardwareEmulated)
