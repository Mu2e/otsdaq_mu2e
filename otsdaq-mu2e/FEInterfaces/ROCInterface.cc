#include "otsdaq-mu2e/FEInterfaces/ROCInterface.h"
#include "otsdaq-core/Macros/CoutMacros.h"


using namespace ots;

#undef 	__MF_SUBJECT__
#define __MF_SUBJECT__ getIdString()

//=========================================================================================
ROCInterface::ROCInterface(const unsigned int linkID,
			   DTCLib::DTC* thisDTC, 
			   const unsigned int delay)//,
			   //const ConfigurationTree& theXDAQContextConfigTree,
			   //const std::string& theConfigurationPath)
//: Configurable(theXDAQContextConfigTree,theConfigurationPath)
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
void ROCInterface::configure(void)
{
  	//junk;
  	
  	std::cout << "ROC link " << linkID_ << " set delay = " << delay_ << std::endl;
  	//should be equivalent with ROC decoration specified in ROCInterface::getIdString()
 	__COUT__ << " set delay = " << delay_ << __E__;
  	
	//  	thisDTC_->WriteROCRegister(linkID_,6,delay_);

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



