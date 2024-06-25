#include "otsdaq-mu2e/FEInterfaces/ROCPolarFireCoreInterface.h"

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "FE-ROCPolarFireCoreInterface"

//=========================================================================================
ROCPolarFireCoreInterface::ROCPolarFireCoreInterface(
    const std::string&       rocUID,
    const ConfigurationTree& theXDAQContextConfigTree,
    const std::string&       theConfigurationPath)
    : ROCCoreVInterface(rocUID, theXDAQContextConfigTree, theConfigurationPath)
{
	INIT_MF("." /*directory used is USER_DATA/LOG/.*/);

	__MCOUT_INFO__("ROCPolarFireCoreInterface instantiated with link: "
	               << linkID_ << " and EventWindowDelayOffset = " << delay_ << __E__);

	

	registerFEMacroFunction("Setup for Pattern Data Taking",
	                        static_cast<FEVInterface::frontEndMacroFunction_t>(
	                            &ROCPolarFireCoreInterface::SetupForPatternDataTaking),
	                        std::vector<std::string>{}, //inputs parameters
	                        std::vector<std::string>{}, //output parameters
	                        1);  // requiredUserPermissions

}  // end constructor()

//==========================================================================================
ROCPolarFireCoreInterface::~ROCPolarFireCoreInterface(void)
{
	// NOTE:: be careful not to call __FE_COUT__ decoration because it uses the
	// tree and it may already be destructed partially
	// Instead use __GEN_COUT__ which decorates using mfSubject_
	__GEN_COUT__ << "Destructed." << __E__;
}  // end destructor()

//==================================================================================================
uint16_t ROCPolarFireCoreInterface::readEmulatorRegister(uint16_t address)
{
	__FE_COUT__ << "Calling read emulator ROC register: link number " << std::dec
	            << linkID_ << ", address = " << address << __E__;
	if(address == 6)
		return 4860;
	else if(address == 7)
		return delay_;
	return -1;
}  // end readEmulatorRegister()


//==================================================================================================
void ROCPolarFireCoreInterface::readEmulatorBlock(std::vector<DTCLib::roc_data_t>& 	data,
                                             DTCLib::roc_address_t  	   	address,
                                             uint16_t               		numberOfReads,
                                             bool                   		incrementAddress)
{
	__FE_COUT__ << "Calling read emulator block: link number " << std::dec << linkID_
	            << ", address = " << address << ", numberOfReads = " << numberOfReads
	            << ", incrementAddress = " << incrementAddress << __E__;

	for(unsigned int i = 0; i < numberOfReads; ++i)
		data.push_back(address + (incrementAddress ? i : 0));
}  // end readEmulatorBlock()

//==================================================================================================
void ROCPolarFireCoreInterface::GetStatus(__ARGS__) { __SS__ << "TODO"; __SS_THROW__; }

//==================================================================================================
std::string ROCPolarFireCoreInterface::getFirmwareVersion() { __SS__ << "TODO"; __SS_THROW__; }

//==================================================================================================
int ROCPolarFireCoreInterface::readInjectedPulseTimestamp() { return this->readRegister(12); }

//==================================================================================================
void ROCPolarFireCoreInterface::writeDelay(uint16_t delay)
{
	this->writeRegister(21, delay);
	return;
}

//==================================================================================================
int ROCPolarFireCoreInterface::readDelay() { return this->readRegister(7); }

//==================================================================================================
int ROCPolarFireCoreInterface::readDTCLinkLossCounter() { return this->readRegister(8); }

//==================================================================================================
void ROCPolarFireCoreInterface::resetDTCLinkLossCounter()
{
	this->writeRegister(24, 0x1);
	return;
}

//==================================================================================================
void ROCPolarFireCoreInterface::configure(void) try
{
	if(emulatorMode_)
	{
		__FE_COUT__ << "Emulator ROC configuring..." << __E__;
		return;
	}
	 __MCOUT_INFO__("......... Clear DCS FIFOs" << __E__);
	// this->writeRegister(0,1);
	//this->writeRegister(0,0);  // MT: in DracMonitor, write ANY to addr 0 to issue TOP_SERDES reset. Self-clearing.  

	// setup needToResetAlignment using rising edge of register 22
	// (i.e., force synchronization of ROC clock with 40MHz system clock)
	__MCOUT_INFO__("......... setup to synchronize ROC clock with 40 MHz clock edge"
	               << __E__);
	//this->writeRegister(22, 0);
	//this->writeRegister(22, 1);
	//this->writeRegister(4, 1); // MT: in DracMonitor, DCS_ALIGNMENT is addr 4.  Self-clearing


	this->writeDelay(delay_);

	__MCOUT_INFO__("........."
	               << " Set delay = " << delay_ << ", readback = " << this->readDelay()
	               << "... ");

	__FE_COUT__ << "Debugging ROC-DCS" << __E__;

	unsigned int val;

	// read 6 should read back 0x12fc
	for(int i = 0; i < 1; i++)
	{
		val = this->readRegister(6);

		//__MCOUT_INFO__(i << " read register 6 = " << val << __E__);
		if(val != 4860)
		{
			__FE_SS__ << "Bad read not 4860! val = " << val << __E__;
			//__FE_SS_THROW__;   disable for the moment, so we can debug
		}

		val = this->readDelay();
		//__MCOUT_INFO__(i << " read register 7 = " << val << __E__);
		if(val != delay_)
		{
			__FE_SS__ << "Bad read not " << delay_ << "! val = " << val << __E__;
			//__FE_SS_THROW__;   disable for the moment, so we can debug
		}
	}

	__MCOUT_INFO__("......... reset DTC link loss counter ... ");
	resetDTCLinkLossCounter();
}  // end configure()
catch(const std::runtime_error& e)
{
	__FE_MOUT__ << "Error caught: " << e.what() << __E__;
	throw;
}
catch(...)
{
	__FE_SS__ << "Unknown error caught. Check printouts!" << __E__;
	try	{ throw; } //one more try to printout extra info
	catch(const std::exception &e)
	{
		ss << "Exception message: " << e.what();
	}
	catch(...){}
	__FE_SS_THROW__;
}  // end configure() catch

//==============================================================================
void ROCPolarFireCoreInterface::halt(void)
{
	// Call Core Halt() first to stop emulator threads properly
	ROCCoreVInterface::halt();

	// do specifics here:
	// ...
}  // end halt()

//==============================================================================
void ROCPolarFireCoreInterface::pause(void) {}

//==============================================================================
void ROCPolarFireCoreInterface::resume(void) {}

//==============================================================================
void ROCPolarFireCoreInterface::start(std::string)  // runNumber)
{
}

//==============================================================================
void ROCPolarFireCoreInterface::stop(void) {}

//==============================================================================
bool ROCPolarFireCoreInterface::running(void) { return false; }



//==================================================================================================
void ROCPolarFireCoreInterface::SetupForPatternDataTaking(__ARGS__)
{
	__COUT_INFO__ << "SetupForPatternDataTaking()" << __E__;

	//For future, to get link ID of this ROC:
	__FE_COUTV__(getLinkID());

	writeRegister(14,1);  //ROC reset
	writeRegister(8,1 << 4);
	writeRegister(30,0);
	writeRegister(29,1);

	__COUT_INFO__ << "end SetupForPatternDataTaking()" << __E__;


	// __SET_ARG_OUT__("readValue",GetTemperature(channelnumber));
} //end SetupForPatternDataTaking()