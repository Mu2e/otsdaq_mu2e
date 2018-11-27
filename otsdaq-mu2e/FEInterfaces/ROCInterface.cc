#include "otsdaq-mu2e/FEInterfaces/ROCInterface.h"
#include "otsdaq-core/Macros/CoutMacros.h"


using namespace ots;

#undef 	__MF_SUBJECT__
#define __MF_SUBJECT__ getIdString()

//=========================================================================================
ROCInterface::ROCInterface(const unsigned int linkID,
			   DTCLib::DTC* thisDTC, 
			   const unsigned int delay,
			   const ConfigurationTree& theXDAQContextConfigTree,
			   const std::string& theConfigurationPath)
: Configurable(theXDAQContextConfigTree,theConfigurationPath)
{
	INIT_MF("ROCInterface");

	linkID_  = DTCLib::DTC_Link_ID(linkID);  
	thisDTC_ = thisDTC;	
	delay_   = delay;
	__COUTV__(delay_);
	
//	try
//	{
//		delay_ = getSelfNode().getNode("DelayOffset").getValue<unsigned int>();
//	}
//	catch(...)
//	{
//		__COUT__ << "Field does not exist?" << __E__;
//	}

	__MCOUT_INFO__("ROCInterface instantiated with link: " << linkID_ << __E__);

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
  std::cout << getIdString() << " 1 read register 6 = " << this->readRegister(6) << std::endl;
  std::cout << getIdString() << " 2 read register 6 = " << this->readRegister(6) << std::endl;
  std::cout << getIdString() << " 3 read register 6 = " << this->readRegister(6) << std::endl;
  std::cout << getIdString() << " 4 read register 6 = " << this->readRegister(6) << std::endl;

  std::cout << getIdString() << " set delay = " << delay_ << std::endl;
  this->writeDelay(delay_);

  std::cout << getIdString() << " read delay = " << this->readDelay() << std::endl;
  
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



