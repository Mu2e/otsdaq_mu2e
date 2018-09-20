#include "otsdaq-mu2e/FEInterfaces/DTCFrontEndInterface.h"
#include "otsdaq-core/Macros/InterfacePluginMacros.h"
//#include "otsdaq/DAQHardware/FrontEndHardwareTemplate.h"
//#include "otsdaq/DAQHardware/FrontEndFirmwareTemplate.h"


#include "mu2e_driver/mu2e_mmap_ioctl.h"	// m_ioc_cmd_t
#include "dtcInterfaceLib/DTC.h"

using namespace ots;

#undef 	__MF_SUBJECT__
#define __MF_SUBJECT__ "DTCFrontEndInterface"

//========================================================================================================================
DTCFrontEndInterface::DTCFrontEndInterface(const std::string& interfaceUID, const ConfigurationTree& theXDAQContextConfigTree, const std::string& interfaceConfigurationPath) :
						FEVInterface (interfaceUID, theXDAQContextConfigTree, interfaceConfigurationPath)
{
  //theFrontEndHardware_ = new FrontEndHardwareTemplate();
  //theFrontEndFirmware_ = new FrontEndFirmwareTemplate();
  universalAddressSize_ = 4;
  universalDataSize_    = 4;

  __COUT__ << "DTCFrontEndInterface instantiated with name: " << interfaceUID << std::endl;
}

//========================================================================================================================
DTCFrontEndInterface::~DTCFrontEndInterface(void)
{
  //delete theFrontEndHardware_;
  //delete theFrontEndFirmware_;
}


//========================================================================================================================
//universalRead
//	Must implement this function for Macro Maker to work with this interface.
//	When Macro Maker calls:
//		- address will be a [universalAddressSize_] byte long char array
//		- returnValue will be a [universalDataSize_] byte long char array
//		- expects return value of 0 on success and negative numbers on failure
int DTCFrontEndInterface::universalRead(char *address, char *returnValue)
{
  //  __COUT__ << "DTC READ" << __E__;

  char devfile[11];
  int fd;
  int dtc = -1;

  m_ioc_reg_access_t reg_access; 
  int                sts=0;


  dtc = getSelfNode().getNode("DeviceIndex").getValue<unsigned int>();
  snprintf(devfile, 11, "/dev/" MU2E_DEV_FILE, dtc);
  fd = open( devfile, O_RDONLY );

  //  DTCLib::DTC thisDTC();

 
  reg_access.reg_offset = *((int*) address); 
  //  __COUTV__(reg_access.reg_offset);

  reg_access.access_type = 0;                      // 0 = read, 1 = write
  sts = ioctl( fd, M_IOC_REG_ACCESS, &reg_access );

  if (sts) 
    {
      __COUT_ERR__ << "ERROR:  DTC register read" << __E__; 
      return -1; //failed
    }

  std::memcpy(returnValue,&reg_access.val,universalDataSize_);
  //  __COUTV__(reg_access.val);

  return 0; //success
}

//========================================================================================================================
//universalWrite
//	Must implement this function for Macro Maker to work with this interface.
//	When Macro Maker calls:
//		- address will be a [universalAddressSize_] byte long char array
//		- writeValue will be a [universalDataSize_] byte long char array
void DTCFrontEndInterface::universalWrite(char* address, char* writeValue)
{
  //  __COUT__ << "DTC WRITE" << __E__;

  char devfile[11];
  int fd;
  int dtc = -1;

  m_ioc_reg_access_t reg_access; 
  int                sts=0;

  dtc = getSelfNode().getNode("DeviceIndex").getValue<unsigned int>();
  snprintf(devfile, 11, "/dev/" MU2E_DEV_FILE, dtc);
  fd = open( devfile, O_RDONLY );

  reg_access.reg_offset = *((int*) address); 
  //  __COUTV__(reg_access.reg_offset);

  reg_access.val = *((int*) writeValue); 
  //  __COUTV__(reg_access.val);

  reg_access.access_type = 1;                      // 0 = read, 1 = write
  sts = ioctl( fd, M_IOC_REG_ACCESS, &reg_access );

  if (sts) 
    __COUT_ERR__ << "ERROR:  M_IOC_REG_ACCESS write"  << __E__; 

  return;

}


//========================================================================================================================
void DTCFrontEndInterface::configure(void)
{
  uint8_t *addrs = new uint8_t[universalAddressSize_];	//create address buffer of interface size
  uint8_t *data = new uint8_t[universalDataSize_];		//create data buffer of interface size
  
  //============= Set/check clock frequency
  //code pulled from:  mu2eUtil program_clock -C 0 -F 200000000
  // C=0 = main board SERDES clock
  // C=1 = DDR clock
  // C=2 = Timing board SERDES clock

  int dtc = -1;
  unsigned roc_mask = 0x1;
  std::string expectedDesignVersion = "";
  auto mode = DTCLib::DTC_SimMode_NoCFO;

  //  auto thisDTC = new DTCLib::DTC(mode,dtc,roc_mask,expectedDesignVersion);
  //  auto oscillator = DTCLib::DTC_OscillatorType_SERDES;
  //  int targetFrequency = 200000000;
  //  thisDTC->SetNewOscillatorFrequency(oscillator,targetFrequency);

  //  delete thisDTC;

}

//========================================================================================================================
void DTCFrontEndInterface::halt(void)
{
}

//========================================================================================================================
void DTCFrontEndInterface::pause(void)
{
}

//========================================================================================================================
void DTCFrontEndInterface::resume(void)
{
}

//========================================================================================================================
void DTCFrontEndInterface::start(std::string )//runNumber)
{
}

//========================================================================================================================
void DTCFrontEndInterface::stop(void)
{
}

//========================================================================================================================
bool DTCFrontEndInterface::running(void)
{

  while(WorkLoop::continueWorkLoop_) {

    break; 
  } 

  return false;  
}



DEFINE_OTS_INTERFACE(DTCFrontEndInterface)
