#include "otsdaq-mu2e/FEInterfaces/ROCInterface.h"
#include "otsdaq-core/Macros/CoutMacros.h"


using namespace ots;

#undef 	__MF_SUBJECT__
#define __MF_SUBJECT__ getIdString()

//=========================================================================================
ROCInterface::ROCInterface(DTCLib::DTC* thisDTC, 
			   const std::string& interfaceUID, 
			   const ConfigurationTree& theXDAQContextConfigTree,
			   const std::string& theConfigurationPath)
: Configurable(theXDAQContextConfigTree,theConfigurationPath)
{
  INIT_MF("ROCInterface");

  thisDTC_ = thisDTC;

  device_name_ = interfaceUID;

  linkID_  = DTCLib::DTC_Link_ID( getSelfNode().getNode("linkID").getValue<unsigned int>() );  
  //  __COUTV__(linkID_);

  delay_   = getSelfNode().getNode("EventWindowDelayOffset").getValue<unsigned int>();
  //  __COUTV__(delay_);
	
  __MCOUT_INFO__("ROCInterface instantiated with link: " << linkID_ 
		 << " and EventWindowDelayOffset = " << delay_
		 << __E__);
}

//==========================================================================================
ROCInterface::~ROCInterface(void)
{
}

//==================================================================================================
void ROCInterface::writeRegister(unsigned address, unsigned data_to_write) {

  thisDTC_->WriteROCRegister(linkID_,address,data_to_write);

  return;

}

//==================================================================================================
int ROCInterface::readRegister(unsigned address) {

  return thisDTC_->ReadROCRegister(linkID_,address);

}

//==================================================================================================
int ROCInterface::readTimestamp() {

  return this->readRegister(12);
}

void ROCInterface::writeDelay(unsigned delay) {

 this->writeRegister(21,delay);

 return;
}

int ROCInterface::readDelay() {

 return this->readRegister(7);
}

//==================================================================================================
void ROCInterface::configure(void)
{

  // read 6 should read back 0x12fc
  //  __COUT__ << getIdString() << " 1 read register 6 = " << this->readRegister(6) << __E__;
  //  __COUT__ << getIdString() << " 2 read register 6 = " << this->readRegister(6) << __E__;
  //  __COUT__ << getIdString() << " 3 read register 6 = " << this->readRegister(6) << __E__;
  __COUT__ << getIdString() << " Confirm read register 6 = " << this->readRegister(6) << __E__;

  __MCOUT_INFO__ ("........." << getIdString() << " Set delay = " << delay_ << __E__);

  this->writeDelay(delay_);

  __COUT__ << getIdString() << " Read delay = " << this->readDelay() << __E__;

  __COUT__ << __E__;   __COUT__ << __E__;   __COUT__ << __E__;
  __COUT__ << getIdString() << " Read register 6 = " << this->readRegister(6) << __E__;
  __COUT__ << getIdString() << " Read register 6 = " << this->readRegister(6) << __E__;
  __COUT__ << getIdString() << " Read register 7 = " << this->readRegister(7) << __E__;
  __COUT__ << getIdString() << " Read register 7 = " << this->readRegister(7) << __E__;

  return;
}

//========================================================================================================================
void ROCInterface::halt(void)
{

}

//========================================================================================================================
void ROCInterface::pause(void)
{

}

//========================================================================================================================
void ROCInterface::resume(void)
{

}

//========================================================================================================================
void ROCInterface::start(std::string )//runNumber)
{

}

//========================================================================================================================
void ROCInterface::stop(void)
{

}

//========================================================================================================================
bool ROCInterface::running(void)
{  
  
  return false; 
}



