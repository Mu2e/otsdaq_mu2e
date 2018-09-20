#include "otsdaq-mu2e/FEInterfaces/CFOFrontEndInterface.h"
#include "otsdaq-core/Macros/InterfacePluginMacros.h"
//#include "otsdaq/DAQHardware/FrontEndHardwareTemplate.h"
//#include "otsdaq/DAQHardware/FrontEndFirmwareTemplate.h"


#include "mu2e_driver/mu2e_mmap_ioctl.h"	// m_ioc_cmd_t
#include "dtcInterfaceLib/DTC.h"

using namespace ots;

#undef 	__MF_SUBJECT__
#define __MF_SUBJECT__ "CFOFrontEndInterface"

//========================================================================================================================
CFOFrontEndInterface::CFOFrontEndInterface(const std::string& interfaceUID, const ConfigurationTree& theXDAQContextConfigTree, const std::string& interfaceConfigurationPath) :
						FEVInterface (interfaceUID, theXDAQContextConfigTree, interfaceConfigurationPath)
{
  //theFrontEndHardware_ = new FrontEndHardwareTemplate();
  //theFrontEndFirmware_ = new FrontEndFirmwareTemplate();
  universalAddressSize_ = 4;
  universalDataSize_    = 4;

  __COUT__ << "CFOFrontEndInterface instantiated with name: " << interfaceUID << std::endl;
}

//========================================================================================================================
CFOFrontEndInterface::~CFOFrontEndInterface(void)
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
int CFOFrontEndInterface::universalRead(char *address, char *returnValue)
{
  //  __COUT__ << "CFO READ" << __E__;

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
      __COUT_ERR__ << "ERROR:  CFO register read" << __E__; 
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
void CFOFrontEndInterface::universalWrite(char* address, char* writeValue)
{
  //  __COUT__ << "CFO WRITE" << __E__;

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
//
float CFOFrontEndInterface::MeasureLoopback(int linkToLoopback) {

  const int numberOfIterations = 10;

  uint8_t *addrs = new uint8_t[universalAddressSize_];	//create address buffer of interface size
  uint8_t *data = new uint8_t[universalDataSize_];		//create data buffer of interface size


  //=========== Setup for loopback =============

  __COUT__ << "LOOPBACK:  turn off Event and 40MHz markers" << __E__;

  //-------------- Turn off Event markers
  // universalWrite(0x91a0,0x00000000);
  {
    uint8_t macroAddrs[2] = {0xa0, 0x91};	
    for(unsigned int i=0;i<universalAddressSize_;++i) addrs[i] = (i < 2)?macroAddrs[i]:0;

    uint8_t macroData[4] = {0x00, 0x00, 0x00, 0x00};
    for(unsigned int i=0;i<universalDataSize_;++i) data[i] = (i < 4)?macroData[i]:0;

    universalWrite((char *)addrs,(char *)data);
  }

  //----------- Disable 40MHz markers (set 40MHz marker interval to 0)
  // universalWrite(0x9154,0x00000000);
  {
    uint8_t macroAddrs[2] = {0x54, 0x91};	
    for(unsigned int i=0;i<universalAddressSize_;++i) addrs[i] = (i < 2)?macroAddrs[i]:0;
    
    uint8_t macroData[4] = {0x00, 0x00, 0x00, 0x00};
    for(unsigned int i=0;i<universalDataSize_;++i) data[i] = (i < 4)?macroData[i]:0;
    
    universalWrite((char *)addrs,(char *)data);
  }

  float average = 0.0;

  for (int n = 0; n<numberOfIterations; n++) {

    //----------clear delay measurement mode
    // universalWrite(0x9380,0x00000000);
    {
      uint8_t macroAddrs[2] = {0x80, 0x93};	
      for(unsigned int i=0;i<universalAddressSize_;++i) addrs[i] = (i < 2)?macroAddrs[i]:0;
    
      uint8_t macroData[4] = {0x00, 0x00, 0x00, 0x00};	
      for(unsigned int i=0;i<universalDataSize_;++i) data[i] = (i < 4)?macroData[i]:0;

      universalWrite((char *)addrs,(char *)data);
    }

    //-------- Disable tx and rx data
    // universalWrite(0x9114,0x00000000);
    {
      uint8_t macroAddrs[2] = {0x14, 0x91};	
      for(unsigned int i=0;i<universalAddressSize_;++i) addrs[i] = (i < 2)?macroAddrs[i]:0;
    
      uint8_t macroData[4] = {0x00, 0x00, 0x00, 0x00};	
      for(unsigned int i=0;i<universalDataSize_;++i) data[i] = (i < 4)?macroData[i]:0;
      
      universalWrite((char *)addrs,(char *)data);
    }

    //--- enable tx and rx for link 
    // universalWrite(0x9114,0x00000101);
    {
      uint8_t macroAddrs[2] = {0x14, 0x91};	
      for(unsigned int i=0;i<universalAddressSize_;++i) addrs[i] = (i < 2)?macroAddrs[i]:0;
      
      uint8_t macroData[4] = {0x01, 0x01, 0x00, 0x00};
      macroData[0] <<= linkToLoopback;
      macroData[1] <<= linkToLoopback;

      for(unsigned int i=0;i<universalDataSize_;++i) data[i] = (i < 4)?macroData[i]:0;
      
      universalWrite((char *)addrs,(char *)data);
    }

    //----- Put link link[linkIndex] in delay measurement mode 
    // universalWrite(0x9380,0x00000100);
    {
      uint8_t macroAddrs[2] = {0x80, 0x93};	
      for(unsigned int i=0;i<universalAddressSize_;++i) addrs[i] = (i < 2)?macroAddrs[i]:0;
      
      uint8_t macroData[4] = {0x00, 0x01, 0x00, 0x00};
      macroData[1] <<= linkToLoopback;
      for(unsigned int i=0;i<universalDataSize_;++i) data[i] = (i < 4)?macroData[i]:0;

      universalWrite((char *)addrs,(char *)data);
    }

    // debug universalRead(0x9380,data);
    //    {
    //      uint8_t macroAddrs[2] = {0x80, 0x93};	
    //      for(unsigned int i=0;i<universalAddressSize_;++i) addrs[i] = (i < 2)?macroAddrs[i]:0;
    //	  
    //      universalRead((char *)addrs,(char *)data);
    //
    //      unsigned int read_value = 0x00000000;
    //
    //      for (uint8_t i = universalDataSize_; i > 0 ; i-- ) read_value = (read_value << 8 & 0xffffff00) | data[i-1];
    //    
    //      __COUT__ << "LOOPBACK:  read_value is..." << std::hex << read_value <<  __E__;	
    //    }


    //------ Link link[linkIndex] begin delay measurement
    // universalWrite(0x9380,0x00000101);
    {
      uint8_t macroAddrs[2] = {0x80, 0x93};	
      for(unsigned int i=0;i<universalAddressSize_;++i) addrs[i] = (i < 2)?macroAddrs[i]:0;
      
      uint8_t macroData[4] = {0x01, 0x01, 0x00, 0x00};	
      macroData[0] <<= linkToLoopback;
      macroData[1] <<= linkToLoopback;
      for(unsigned int i=0;i<universalDataSize_;++i) data[i] = (i < 4)?macroData[i]:0;

      universalWrite((char *)addrs,(char *)data);
    }

    // debug universalRead(0x9380,data);
    //    {
    //      uint8_t macroAddrs[2] = {0x80, 0x93};	
    //      for(unsigned int i=0;i<universalAddressSize_;++i) addrs[i] = (i < 2)?macroAddrs[i]:0;
    //	  
    //      universalRead((char *)addrs,(char *)data);
    //
    //      unsigned int read_value = 0x00000000;
    //
    //      for (uint8_t i = universalDataSize_; i > 0 ; i-- ) read_value = (read_value << 8 & 0xffffff00) | data[i-1];
    //    
    //      __COUT__ << "LOOPBACK:  read_value is..." << std::hex << read_value <<  __E__;	
    //    }

    //    __COUT__ << "LOOPBACK:  wait" << __E__;
    // wait for 100us 
    usleep(100);

    //--------read delay value
    // universalRead(0x9360,data);
    {
      uint8_t macroAddrs[2] = {0x60, 0x93};	
      for(unsigned int i=0;i<universalAddressSize_;++i) addrs[i] = (i < 2)?macroAddrs[i]:0;
	  
      universalRead((char *)addrs,(char *)data);
    }

    unsigned int delay = 0x00000000;

    for (uint8_t i = universalDataSize_; i > 0 ; i-- ) delay = (delay << 8 & 0xffffff00) | data[i-1];

    //    __COUT__ << "LOOPBACK:  measured delay value is..." << std::hex << delay <<  __E__;

    average += (float) delay;

  }

  average /= (float) numberOfIterations;
 

  //===== Don't leave the CFO in loopback mode

  //----------clear delay measurement mode
  // universalWrite(0x9380,0x00000000);
  {
    uint8_t macroAddrs[2] = {0x80, 0x93};	
    for(unsigned int i=0;i<universalAddressSize_;++i) addrs[i] = (i < 2)?macroAddrs[i]:0;
    
    uint8_t macroData[4] = {0x00, 0x00, 0x00, 0x00};	
    for(unsigned int i=0;i<universalDataSize_;++i) data[i] = (i < 4)?macroData[i]:0;
    
    universalWrite((char *)addrs,(char *)data);
  }

  //-------- Disable tx and rx data
  // universalWrite(0x9114,0x00000000);
  {
    uint8_t macroAddrs[2] = {0x14, 0x91};	
    for(unsigned int i=0;i<universalAddressSize_;++i) addrs[i] = (i < 2)?macroAddrs[i]:0;
    
    uint8_t macroData[4] = {0x00, 0x00, 0x00, 0x00};	
    for(unsigned int i=0;i<universalDataSize_;++i) data[i] = (i < 4)?macroData[i]:0;
    
    universalWrite((char *)addrs,(char *)data);
  }
  
  delete[] addrs; //free the memory
  delete[] data; //free the memory

  return average;
}

//========================================================================================================================
void CFOFrontEndInterface::configure(void)
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

  auto thisCFO = new DTCLib::DTC(mode,dtc,roc_mask,expectedDesignVersion);
  auto oscillator = DTCLib::DTC_OscillatorType_SERDES;
  int targetFrequency = 200000000;
  //  thisCFO->SetNewOscillatorFrequency(oscillator,targetFrequency);

  //========== Continue configuration

  //---------Set Event Window interval time (beam off)
  // universalWrite(0x91a8,0x0000ffff);
  {
    uint8_t macroAddrs[2] = {0xa8, 0x91};	
    for(unsigned int i=0;i<universalAddressSize_;++i) addrs[i] = (i < 2)?macroAddrs[i]:0;

    uint8_t macroData[4] = {0xff, 0xff, 0x00, 0x00};
    for(unsigned int i=0;i<universalDataSize_;++i) data[i] = (i < 4)?macroData[i]:0;

    universalWrite((char *)addrs,(char *)data);
  }

  // universalRead(0x91a8,data);
  //  {
  //    uint8_t macroAddrs[2] = {0xa8, 0x91};	
  //    for(unsigned int i=0;i<universalAddressSize_;++i) addrs[i] = (i < 2)?macroAddrs[i]:0;
  //    
  //    universalRead((char *)addrs,(char *)data);
  //  }


  //----------reset serdes TX
  // universalWrite(0x9118,0xff0000);
  {
    uint8_t macroAddrs[2] = {0x18, 0x91};	
    for(unsigned int i=0;i<universalAddressSize_;++i) addrs[i] = (i < 2)?macroAddrs[i]:0;
    
    uint8_t macroData[3] = {0x00, 0x00, 0xff};	
    for(unsigned int i=0;i<universalDataSize_;++i) data[i] = (i < 3)?macroData[i]:0;

    universalWrite((char *)addrs,(char *)data);
  }

  // universalWrite(0x9118,0x0);
  {
    uint8_t macroAddrs[2] = {0x18, 0x91};	
    for(unsigned int i=0;i<universalAddressSize_;++i) addrs[i] = (i < 2)?macroAddrs[i]:0;
    
    uint8_t macroData[1] = {0x00};	
    for(unsigned int i=0;i<universalDataSize_;++i) data[i] = (i < 1)?macroData[i]:0;

    universalWrite((char *)addrs,(char *)data);
  }


  //----------reset serdes RX
  // universalWrite(0x9118,0x000000ff);
  {
    uint8_t macroAddrs[2] = {0x18, 0x91};	
    for(unsigned int i=0;i<universalAddressSize_;++i) addrs[i] = (i < 2)?macroAddrs[i]:0;
    
    uint8_t macroData[3] = {0xff, 0x00, 0x00};	
    for(unsigned int i=0;i<universalDataSize_;++i) data[i] = (i < 3)?macroData[i]:0;

    universalWrite((char *)addrs,(char *)data);
  }

  // universalWrite(0x9118,0x0);
  {
    uint8_t macroAddrs[2] = {0x18, 0x91};	
    for(unsigned int i=0;i<universalAddressSize_;++i) addrs[i] = (i < 2)?macroAddrs[i]:0;
    
    uint8_t macroData[1] = {0x00};	
    for(unsigned int i=0;i<universalDataSize_;++i) data[i] = (i < 1)?macroData[i]:0;

    universalWrite((char *)addrs,(char *)data);
  }


  //-----------enable Event Start character output
  // universalWrite(0x9100,0x5);
  {
    uint8_t macroAddrs[2] = {0x00, 0x91};	
    for(unsigned int i=0;i<universalAddressSize_;++i) addrs[i] = (i < 2)?macroAddrs[i]:0;
    
    uint8_t macroData[1] = {0x05};	
    for(unsigned int i=0;i<universalDataSize_;++i) data[i] = (i < 1)?macroData[i]:0;

    universalWrite((char *)addrs,(char *)data);
  }


  //-----------enable SERDES transmit and receive
  // universalWrite(0x9114,0x0000ffff);
  {
    uint8_t macroAddrs[2] = {0x14, 0x91};	
    for(unsigned int i=0;i<universalAddressSize_;++i) addrs[i] = (i < 2)?macroAddrs[i]:0;
    
    uint8_t macroData[4] = {0xff, 0xff, 0x00, 0x00};
    for(unsigned int i=0;i<universalDataSize_;++i) data[i] = (i < 4)?macroData[i]:0;

    universalWrite((char *)addrs,(char *)data);
  }


  //-----------set 40MHz marker interval
  // universalWrite(0x9154,0x80);
  {
    uint8_t macroAddrs[2] = {0x54, 0x91};	
    for(unsigned int i=0;i<universalAddressSize_;++i) addrs[i] = (i < 2)?macroAddrs[i]:0;
    
    uint8_t macroData[1] = {0x80};	
    for(unsigned int i=0;i<universalDataSize_;++i) data[i] = (i < 1)?macroData[i]:0;

    universalWrite((char *)addrs,(char *)data);
  }

  delete thisCFO;

}

//========================================================================================================================
void CFOFrontEndInterface::halt(void)
{
}

//========================================================================================================================
void CFOFrontEndInterface::pause(void)
{
}

//========================================================================================================================
void CFOFrontEndInterface::resume(void)
{
}

//========================================================================================================================
void CFOFrontEndInterface::start(std::string )//runNumber)
{
}

//========================================================================================================================
void CFOFrontEndInterface::stop(void)
{
}

//========================================================================================================================
bool CFOFrontEndInterface::running(void)
{
  const int numberOfLinks = 8;
  int link[numberOfLinks] = {0, 1, 2, 3, 4, 5, 6, 7};

  float delay[numberOfLinks] = {};


  //=========== Perform loopback=============

  int linkIndex = 0;

  while(linkIndex < numberOfLinks && WorkLoop::continueWorkLoop_) {

    //    __COUT__ << "LOOPBACK: on link " << link[linkIndex] <<__E__;

    if (linkIndex == 0) {
      // Do setup for this link
    }

    delay[linkIndex] = MeasureLoopback(link[linkIndex]);

    __COUT__ << "LOOPBACK:  link " << link[linkIndex] << " -> delay = " << delay[linkIndex] <<  __E__;	
    
    linkIndex++;

  } // (linkIndex < numberOfLinks && WorkLoop::continueWorkLoop_)

  return false;
}



DEFINE_OTS_INTERFACE(CFOFrontEndInterface)
