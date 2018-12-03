#include "otsdaq-mu2e/FEInterfaces/ROCCoreInterface.h"
#include "otsdaq-core/Macros/CoutMacros.h"

#include "otsdaq-core/Macros/InterfacePluginMacros.h"
#include "otsdaq-core/PluginMakers/MakeInterface.h"


using namespace ots;

#undef 	__MF_SUBJECT__
#define __MF_SUBJECT__ "FE-ROCCoreInterface"

//=========================================================================================
ROCCoreInterface::ROCCoreInterface(
			   const std::string& rocUID,
			   const ConfigurationTree& theXDAQContextConfigTree,
			   const std::string& theConfigurationPath)
: FEVInterface(rocUID,theXDAQContextConfigTree,theConfigurationPath)
, thisDTC_		(0)
{
  INIT_MF("ROCCoreInterface");

  linkID_  = DTCLib::DTC_Link_ID( getSelfNode().getNode("linkID").getValue<unsigned int>() );  
  delay_   = getSelfNode().getNode("EventWindowDelayOffset").getValue<unsigned int>();
	
  __MCOUT_INFO__("ROCCoreInterface instantiated with link: " << linkID_
		 << " and EventWindowDelayOffset = " << delay_
		 << __E__);
}

//==========================================================================================
ROCCoreInterface::~ROCCoreInterface(void)
{
	__CFG_COUT__ << "Destructor" << __E__;
}

//==================================================================================================
void ROCCoreInterface::writeRegister(unsigned address, unsigned data_to_write)
{
	if(emulatorMode_)
	{
		__CFG_COUT__ << "Emulator mode write." << __E__;
		return;
	}

	thisDTC_->WriteROCRegister(linkID_,address,data_to_write);

	return;

}

//==================================================================================================
int ROCCoreInterface::readRegister(unsigned address)
{

	if(emulatorMode_)
	{
		__CFG_COUT__ << "Emulator mode read." << __E__;
		return -1;
	}

  return thisDTC_->ReadROCRegister(linkID_,address);

}

//==================================================================================================
int ROCCoreInterface::readTimestamp()
{
  return this->readRegister(12);
}

//==================================================================================================
void ROCCoreInterface::writeDelay(unsigned delay)
{
 this->writeRegister(21,delay);

 return;
}

//==================================================================================================
int ROCCoreInterface::readDelay()
{
 return this->readRegister(7);
}

//==================================================================================================
void ROCCoreInterface::configure(void)
{
  // read 6 should read back 0x12fc
  //  __CFG_COUT__ << " 1 read register 6 = " << this->readRegister(6) << __E__;
  //  __CFG_COUT__ << " 2 read register 6 = " << this->readRegister(6) << __E__;
  //  __CFG_COUT__ << " 3 read register 6 = " << this->readRegister(6) << __E__;
  __CFG_COUT__ << " Confirm read register 6 = " << this->readRegister(6) << __E__;

  __MCOUT_INFO__ ("........." << " Set delay = " << delay_ << __E__);

  this->writeDelay(delay_);

  __CFG_COUT__ << " Read delay = " << this->readDelay() << __E__;

  __CFG_COUT__ << __E__;   __CFG_COUT__ << __E__;   __CFG_COUT__ << __E__;
  __CFG_COUT__ << " Read register 6 = " << this->readRegister(6) << __E__;
  __CFG_COUT__ << " Read register 6 = " << this->readRegister(6) << __E__;
  __CFG_COUT__ << " Read register 7 = " << this->readRegister(7) << __E__;
  __CFG_COUT__ << " Read register 7 = " << this->readRegister(7) << __E__;

  return;
}

//========================================================================================================================
void ROCCoreInterface::halt(void)
{

}

//========================================================================================================================
void ROCCoreInterface::pause(void)
{

}

//========================================================================================================================
void ROCCoreInterface::resume(void)
{

}

//========================================================================================================================
void ROCCoreInterface::start(std::string )//runNumber)
{

}

//========================================================================================================================
void ROCCoreInterface::stop(void)
{

}

//========================================================================================================================
bool ROCCoreInterface::running(void)
{  
  
  return false; 
}


DEFINE_OTS_INTERFACE(ROCCoreInterface)

