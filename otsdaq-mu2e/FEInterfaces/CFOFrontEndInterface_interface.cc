#include "otsdaq-mu2e/FEInterfaces/CFOFrontEndInterface.h"
#include "otsdaq-core/Macros/InterfacePluginMacros.h"
//#include "otsdaq/DAQHardware/FrontEndHardwareTemplate.h"
//#include "otsdaq/DAQHardware/FrontEndFirmwareTemplate.h"

//#include "mu2e_driver/mu2e_mmap_ioctl.h"	// m_ioc_cmd_t

using namespace ots;

#undef 	__MF_SUBJECT__
#define __MF_SUBJECT__ "CFOFrontEndInterface"

//===========================================================================================
CFOFrontEndInterface::CFOFrontEndInterface(const std::string& interfaceUID, 
					   const ConfigurationTree& theXDAQContextConfigTree, 
					   const std::string& interfaceConfigurationPath) :
  FEVInterface (interfaceUID, theXDAQContextConfigTree, interfaceConfigurationPath)
{
  //theFrontEndHardware_ = new FrontEndHardwareTemplate();
  //theFrontEndFirmware_ = new FrontEndFirmwareTemplate();
  universalAddressSize_ = 4;
  universalDataSize_ 	= 4;
  
  dtc_ = getSelfNode().getNode("DeviceIndex").getValue<unsigned int>();
  snprintf(devfile_, 11, "/dev/" MU2E_DEV_FILE, dtc_);
  fd_ = open(devfile_, O_RDONLY);
  
  unsigned roc_mask = 0x1;
  std::string expectedDesignVersion = "";
  auto mode = DTCLib::DTC_SimMode_NoCFO;
  
  thisCFO_ = new DTCLib::DTC(mode,dtc_,roc_mask,expectedDesignVersion);
  
  __COUT__ << "CFOFrontEndInterface instantiated with name: " << interfaceUID 
	   << " talking to /dev/mu2e" << dtc_
	   << __E__;
}

//===========================================================================================
CFOFrontEndInterface::~CFOFrontEndInterface(void)
{
  delete thisCFO_;
  //delete theFrontEndHardware_;
  //delete theFrontEndFirmware_;
}


//===========================================================================================
//universalRead
//	Must implement this function for Macro Maker to work with this interface.
//	When Macro Maker calls:
//		- address will be a [universalAddressSize_] byte long char array
//		- returnValue will be a [universalDataSize_] byte long char array
//		- expects return value of 0 on success and negative numbers on failure
int CFOFrontEndInterface::universalRead(char *address, char *returnValue)
{
  // __COUT__ << "CFO READ" << __E__;
  
  reg_access_.access_type = 0; 										// 0 = read, 1 = write
  
  reg_access_.reg_offset = *((int*) address); 
  // __COUTV__(reg_access.reg_offset);
  
  if ( ioctl(fd_,M_IOC_REG_ACCESS,&reg_access_) ) 
    {
      __COUT_ERR__ << "ERROR: CFO universalRead - Does file exist? -> /dev/mu2e" << dtc_ << __E__; 
      return -1; //failed
    }
  
  std::memcpy(returnValue,&reg_access_.val,universalDataSize_);
  // __COUTV__(reg_access.val);
  
  return 0; //success
}

//===========================================================================================
// registerRead: return = value read from register at address "address"
//
int CFOFrontEndInterface::registerRead(int address) {
  
  uint8_t *addrs = new uint8_t[universalAddressSize_];	//create address buffer of interface size
  uint8_t *data = new uint8_t[universalDataSize_];	//create data buffer of interface size
  
  uint8_t macroAddrs[20] = {};	//total hack, assuming we'll never have 200 bytes in an address
  
  //fill byte-by-byte
  for (unsigned int i=0; i<universalAddressSize_; i++) 
    macroAddrs[i] 	= 0xff & (address >> i*8);
  
  //0-pad
  for(unsigned int i=0;i<universalAddressSize_;++i) 
    addrs[i] = (i < 2)?macroAddrs[i]:0;
  
  universalRead((char *)addrs,(char *)data);
  
  
  unsigned int readvalue = 0x00000000;
  
  //unpack byte-by-byte
  for (uint8_t i = universalDataSize_; i > 0 ; i-- ) 
    readvalue = (readvalue << 8 & 0xffffff00) | data[i-1];
  
  // __COUT__ << "DTC: readvalue register 0x" << std::hex << address 
  //		<< " is..." << std::hex << readvalue << __E__;
  
  delete[] addrs; //free the memory
  delete[] data; //free the memory
  
  return readvalue;
}


//======================================================================================
//universalWrite
//	Must implement this function for Macro Maker to work with this interface.
//	When Macro Maker calls:
//		- address will be a [universalAddressSize_] byte long char array
//		- writeValue will be a [universalDataSize_] byte long char array
void CFOFrontEndInterface::universalWrite(char* address, char* writeValue)
{
  // __COUT__ << "CFO WRITE" << __E__;
  
  reg_access_.access_type = 1; 										// 0 = read, 1 = write
  
  reg_access_.reg_offset = *((int*) address); 
  // __COUTV__(reg_access.reg_offset);
  
  reg_access_.val = *((int*) writeValue); 
  // __COUTV__(reg_access.val);
  
  if ( ioctl(fd_,M_IOC_REG_ACCESS,&reg_access_) ) 
    __COUT_ERR__ << "ERROR: CFO universal write - Does file exist? /dev/mu2e" << dtc_ << __E__; 
  
  return;
}

//===============================================================================================
// registerWrite: return = value readback from register at address "address"
//
int CFOFrontEndInterface::registerWrite(int address, int dataToWrite) {
  
  uint8_t *addrs = new uint8_t[universalAddressSize_];	//create address buffer of interface size
  uint8_t *data = new uint8_t[universalDataSize_];	//create data buffer of interface size
  
  uint8_t macroAddrs[20] = {};	//assume we'll never have 20 bytes in an address
  uint8_t macroData[20] = {};	//assume we'll never have 20 bytes read out from a register
  
  // fill byte-by-byte
  for (unsigned int i=0; i<universalAddressSize_; i++) 
    macroAddrs[i] 	= 0xff & (address >> i*8);
  
  // 0-pad
  for(unsigned int i=0;i<universalAddressSize_;++i) 
    addrs[i] = (i < 2)?macroAddrs[i]:0; 				
  
  // fill byte-by-byte
  for (unsigned int i=0; i<universalDataSize_; i++) 
    macroData[i] 	= 0xff & (dataToWrite >> i*8);
  
  // 0-pad
  for(unsigned int i=0;i<universalDataSize_;++i) 
    data[i] = (i < 4)?macroData[i]:0;
  
  universalWrite((char *)addrs,(char *)data);
  
  // usleep(100);
  
  int readbackValue = registerRead(address);
  
  int i=0;
  
  ////this is a I2C register, it clears bit0 when transaction finishes
  if ( (address == 0x916c) && ((dataToWrite & 0x1) == 1) ) {
    
    // wait for I2C to clear...
    while ( (readbackValue & 0x1) != 0 ) { 
      i++;
      readbackValue = registerRead(address);
      usleep(100);
      if ( (i%10) == 0 ) __COUT__ << "CFO I2C waited " << i << " times..." << __E__;
    }
    // 	if (i > 0) __COUT__ << "CFO I2C waited " << i << " times..." << __E__;
    
  }
  
  // lowest 8-bits are the I2C read value. But we aren't reading anything back for the moment...
  if ( address == 0x9168 ) {
    if ( (readbackValue & 0xffffff00) != (dataToWrite & 0xffffff00) ) {
      
      __COUT_ERR__ << "CFO: write value 0x" << std::hex << dataToWrite
		   << " to register 0x" << std::hex << address 
		   << "... read back 0x" << std::hex << readbackValue 
		   << __E__;
    }
  }
  
  // lowest order bit clears itself
  if ( (address == 0x9380) && ((dataToWrite & 0x1) == 1) ) {
    if ( (readbackValue & 0xfffffffe) != (dataToWrite & 0xfffffffe) ) {
      
      __COUT_ERR__ << "CFO: write value 0x" << std::hex << dataToWrite
		   << " to register 0x" << std::hex << address 
		   << "... read back 0x" << std::hex << readbackValue 
		   << __E__;
    }
  }
  
  if (readbackValue != dataToWrite &&
      address != 0x9168 &&
      address != 0x916c &&
      address != 0x9380 ) {
    __COUT_ERR__ 	<< "CFO: write value 0x" << std::hex << dataToWrite
			<< " to register 0x" << std::hex << address 
			<< "... read back 0x" << std::hex << readbackValue 
			<< __E__;
  }
  
  delete[] addrs; //free the memory
  delete[] data; //free the memory
  
  return readbackValue;
}


//=====================================================================================
void CFOFrontEndInterface::readStatus(void) 
{
  __CFG_COUT__ << "firmware version    (0x9004) = 0x" << std::hex << registerRead(0x9004) << __E__;
  __CFG_COUT__ << "link enable         (0x9114) = 0x" << std::hex << registerRead(0x9114) << __E__;
  __CFG_COUT__ << "SERDES reset        (0x9118) = 0x" << std::hex << registerRead(0x9118) << __E__;
  __CFG_COUT__ << "SERDES unlock error (0x9124) = 0x" << std::hex << registerRead(0x9124) << __E__;
  __CFG_COUT__ << "PLL locked          (0x9128) = 0x" << std::hex << registerRead(0x9128) << __E__;
  __CFG_COUT__ << "SERDES Rx status....(0x9134) = 0x" << std::hex << registerRead(0x9134) << __E__;
  __CFG_COUT__ << "SERDES reset done...(0x9138) = 0x" << std::hex << registerRead(0x9138) << __E__;
  __CFG_COUT__ << "SERDES Rx CDR lock..(0x9140) = 0x" << std::hex << registerRead(0x9140) << __E__;
  __CFG_COUT__ << "SERDES ref clk freq.(0x9160) = 0x" << std::hex << registerRead(0x9160) << " = " 
	       << std::dec << registerRead(0x9160) << __E__;
  
  __COUT__ << __E__;
  
}

//=====================================================================================
//
int CFOFrontEndInterface::getLinkStatus() {
  
  int overall_link_status = registerRead(0x9140);
  
  int link_status = (overall_link_status >> 0) & 0x1;
  
  return link_status;
  
}

//=====================================================================================
//
float CFOFrontEndInterface::MeasureLoopback(int linkToLoopback) {
  
  const int numberOfIterations = 100;
  
  float numerator				= 0.0;
  float denominator			= 0.0;
  int number_of_failures 		= 0;
  
  int loopback_data[numberOfIterations] = {};
  
  max_distribution_ = 0;         // maximum value of the histogram
  min_distribution_ = 99999;     // minimum value of the histogram
  
  for (int n = 0; n < 10000; n++) 
    loopback_distribution_[n] = 0;   //zero out the histogram
  
  __COUT__ << "LOOPBACK: CFO status before loopback" << __E__;
  readStatus();
  
  //	__COUT__ << "LOOPBACK: BEFORE max_distribution_: " << max_distribution_ << __E__;	
  
  for (int n = 0; n<numberOfIterations; n++) {
    
    //----------take out of delay measurement mode
    registerWrite(0x9380,0x00000000);
    
    //-------- Disable tx and rx data
    registerWrite(0x9114,0x00000000);
    
    //--- enable tx and rx for link linkToLoopback
    int dataToWrite = (0x00000101 << linkToLoopback);
    registerWrite(0x9114,dataToWrite);
    
    //----- Put linkToLoopback in delay measurement mode 
    dataToWrite = (0x00000100 << linkToLoopback);
    registerWrite(0x9380,dataToWrite);
    
    
    //------ begin delay measurement
    dataToWrite = (0x00000101 << linkToLoopback);
    registerWrite(0x9380,dataToWrite);
    usleep(100);
    
    //--------read delay value
    unsigned int delay = registerRead(0x9360);
    //		__COUT__ << "LOOPBACK iteration " << std::dec << n << " gives " << delay << __E__;
    
    if (delay < 10000) {
      
      numerator += (float) delay;
      denominator += 1.0;
      // 		__COUT__ << "LOOPBACK iteration " << std::dec << n << " gives " << delay << __E__;
      
      loopback_data[n] = delay;
      
      loopback_distribution_[delay]++;
      
      if (delay > max_distribution_) {
	max_distribution_ = delay;
	//		    __COUT__ << "LOOPBACK: new max_distribution_: " << max_distribution_ << __E__;	
      }
      
      if (delay < min_distribution_) 
	min_distribution_ = delay;
      
    } else {
      
      loopback_data[n] = -999;
      number_of_failures++;
      
    }
    
    //----------clear delay measurement mode
    registerWrite(0x9380,0x00000000);
    
    //-------- Disable tx and rx data
    registerWrite(0x9114,0x00000000);
    
    usleep(100);
    
  }
  
  __COUT__ << "LOOPBACK: CFO status after loopback" << __E__;
  readStatus();
  
  // do a little bit of analysis
  
  average_loopback_ = -999.;
  if (denominator > 0.0) 
    average_loopback_ = numerator / denominator;
  
  rms_loopback_ = 0.;
  
  for (int n = 0; n<numberOfIterations; n++)
    rms_loopback_ = (loopback_data[n] - average_loopback_) * (loopback_data[n] - average_loopback_);
  
  if (denominator > 0.0) 
    rms_loopback_ = sqrt(rms_loopback_ / denominator);
  
  __COUT__ << "LOOPBACK: distribution: " << __E__;
  //	__COUT__ << "LOOPBACK: min_distribution_: " << min_distribution_ << __E__;
  //	__COUT__ << "LOOPBACK: max_distribution_: " << max_distribution_ << __E__;
  
  for (unsigned int n=(min_distribution_-5); n<(max_distribution_+5); n++) 
    __COUT__ << " delay [ " << n << " ] = " << loopback_distribution_[n] << __E__;
  
  __COUT__ << __E__;
  
  __COUT__ << "LOOPBACK: number of failed loopbacks = " << std::dec << number_of_failures << __E__;
  
  
  
  return average_loopback_;
}


//===============================================================================================
void CFOFrontEndInterface::configure(void)
{
  
  __CFG_COUTV__( getIterationIndex() );
  __CFG_COUTV__( getSubIterationIndex() );
  
  // NOTE: otsdaq/xdaq has a soap reply timeout for state transitions.
  // Therefore, break up configuration into several steps so as to reply before the time out
  // As well, there is a specific order in which to configure the links in the chain of CFO->DTC0->DTC1->...DTCN
  
  
  const int number_of_system_configs	= -1; // if < 0, keep trying until links are OK.  
                                              // If > 0, go through configuration steps this many times 
  const int config_clock		= 0;  // 1 = yes, 0 = no
  const int reset_tx			= 1;  // 1 = yes, 0 = no
  
  const int number_of_dtc_config_steps = 6; 
  
  int number_of_total_config_steps = number_of_system_configs * number_of_dtc_config_steps;
  
  int config_step = getIterationIndex();
  
  if (number_of_system_configs > 0) {
    if (config_step >= number_of_total_config_steps) // done - exit system config
      return;
  }
  
  if ( (config_step%number_of_dtc_config_steps) == 0 ) {
    
    __COUT__ << "CFO disable Event Start character output " << __E__;
    registerWrite(0x9100,0x0); 
    
    __COUT__ << "CFO disable serdes transmit and receive " << __E__;
    registerWrite(0x9114,0x00000000); 
    
    __COUT__ << "CFO turn off Event Windows" << __E__;
    registerWrite(0x91a0,0x00000000); 	
    
    __COUT__ << "CFO turn off 40MHz marker interval" << __E__;
    registerWrite(0x9154,0x00000000); 	
    
    if (config_clock == 1 && config_step < number_of_dtc_config_steps) {
      // only configure the clock/crystal the first loop through...
      __MCOUT_INFO__("CFO resetting clock..." << __E__);
      __COUT__ << "CFO set crystal frequency to 156.25 MHz" << __E__;
      
      registerWrite(0x9160,0x09502F90);
      
      //set RST_REG bit
      registerWrite(0x9168,0x55870100);
      registerWrite(0x916c,0x00000001);
      
      sleep(5);
      
      //-----begin code snippet pulled from: mu2eUtil program_clock -C 0 -F 200000000 ---
      // C=0 = main board SERDES clock
      // C=1 = DDR clock
      // C=2 = Timing board SERDES clock
      
      int targetFrequency = 200000000;
      
      auto oscillator = DTCLib::DTC_OscillatorType_SERDES; //-C 0 = CFO (main board SERDES clock)
      // auto oscillator = DTCLib::DTC_OscillatorType_DDR; //-C 1 (DDR clock)
      // auto oscillator = DTCLib::DTC_OscillatorType_Timing; //-C 2 = DTC (with timing card)
      
      __COUT__ << "CFO set oscillator frequency to " << std::dec << targetFrequency << " MHz" << __E__;
      
      thisCFO_->SetNewOscillatorFrequency(oscillator,targetFrequency);
      
      //-----end code snippet pulled from: mu2eUtil program_clock -C 0 -F 200000000
      
      sleep(5);
      
    } else { 

      __MCOUT_INFO__("CFO NOT resetting clock..." << __E__);      
      __COUT__ << "CFO DO NOT set oscillator frequency" << __E__;
      
    }
    
  } else if ( (config_step%number_of_dtc_config_steps) == 2 ) {
    
    //step 2: after DTC jitter attenuator OK, config CFO SERDES PLLs and TX
    if (reset_tx == 1) {

      __MCOUT_INFO__("CFO reset TX..." << __E__);

      __COUT__ << "CFO reset serdes PLLs " << __E__;
      registerWrite(0x9118,0x0000ff00);
      registerWrite(0x9118,0x0);
      sleep(3);
      
      __COUT__ << "CFO reset serdes TX " << __E__;
      registerWrite(0x9118,0x00ff0000);
      registerWrite(0x9118,0x0);
      sleep(3);
    } else {

      __MCOUT_INFO__("CFO do NOT reset TX..." << __E__);
      __COUT__ << "CFO do NOT reset PLL and TX" << __E__;

    } 
    
  } else if ( (config_step%number_of_dtc_config_steps) == 4 ) {
    
    __COUT__ << "CFO reset serdes RX " << __E__;
    registerWrite(0x9118,0x000000ff);
    registerWrite(0x9118,0x0);
    sleep(3);
    
    __COUT__ << "CFO enable Event Start character output " << __E__;
    registerWrite(0x9100,0x5); 
    
    __COUT__ << "CFO enable serdes transmit and receive " << __E__;
    registerWrite(0x9114,0x0000ffff); 
    
    __COUT__ << "CFO set Event Window interval time" << __E__;
    registerWrite(0x91a0,0x0000ffff); 
    // 	registerWrite(0x91a0,0x00000000); 	// for NO markers, write these values
    
    __COUT__ << "CFO set 40MHz marker interval" << __E__;
    registerWrite(0x9154,0x0800);
    // 	registerWrite(0x9154,0x00000000); 	// for NO markers, write these values
    
  } else if ( (config_step%number_of_dtc_config_steps) == 5 ) {
    
    __MCOUT_INFO__("--------------" << __E__);
    __MCOUT_INFO__("CFO configured" << __E__);
    
    if (getLinkStatus() == 1) {
      
      __MCOUT_INFO__("CFO links OK = 0x" << std::hex << registerRead(0x9140) << std::dec << __E__);
      
      if (number_of_system_configs < 0) {
	return;   // links OK, kick out
      }
      
    } else {
      
      __MCOUT_INFO__("CFO links not OK = 0x" << std::hex << registerRead(0x9140) << std::dec << __E__);
      
    }
    __COUT__ << __E__;
    
  }
  
  readStatus(); //spit out link status at every step
  indicateIterationWork(); // I still need to be touched
  return;
}

//========================================================================================================================
void CFOFrontEndInterface::halt(void)
{
  __COUT__ << "HALT: CFO status" << __E__;
  readStatus();
}

//========================================================================================================================
void CFOFrontEndInterface::pause(void)
{
  __COUT__ << "PAUSE: CFO status" << __E__;
  readStatus();
}

//========================================================================================================================
void CFOFrontEndInterface::resume(void)
{
  __COUT__ << "RESUME: CFO status" << __E__;
  readStatus();
}

//========================================================================================================================
void CFOFrontEndInterface::start(std::string )//runNumber)
{
  const int numberOfChains = 1;
  int link[numberOfChains] = {0};
  
  const int numberOfDTCsPerChain = 2; // assume 0, then 1
  
  const int numberOfROCsPerDTC = 2; // assume 0, then 1
  
  // To do loopbacks on all CFOs, first have to setup all DTCs, then the CFO (this method)
  // work per iteration. Loop back done on all chains (in this method), assuming the following order: 
  // i DTC0 DTC1 ... DTCN
  // 0 ROC0 none ... none 
  // 1 ROC1 none ... none
  // 2 none ROC0 ... none
  // 3 none ROC1 ... none
  // ...
  // N-1 none none ... ROC0
  // N none none ... ROC1
  
  int totalNumberOfLoopbacks = numberOfChains * numberOfDTCsPerChain * numberOfROCsPerDTC;
  
  int loopbackIndex = getIterationIndex();
  
  if (loopbackIndex >= totalNumberOfLoopbacks) {
    __MCOUT_INFO__("-------------------------" << __E__); 	
    __MCOUT_INFO__("FULL SYSTEM loopback DONE" << __E__);
    
    for (int nChain = 0; nChain < numberOfChains; nChain++) {
      for (int nDTC = 0; nDTC < numberOfDTCsPerChain; nDTC++) {
	for (int nROC = 0; nROC < numberOfROCsPerDTC; nROC++) {
	  __MCOUT_INFO__("chain " << nChain << " - DTC " << nDTC << " - ROC " << nROC
			 << " = " << std::dec << delay[nChain][nDTC][nROC] << __E__);
	}
      }
    }
    __MCOUT_INFO__("-------------------------" << __E__); 
    
    __COUT__ << "LOOPBACK: CFO reset serdes RX " << __E__;
    registerWrite(0x9118,0x000000ff);
    registerWrite(0x9118,0x0);
    sleep(5);
    
    readStatus();
    return;
  }
  
  __COUT__ << "START: CFO status" << __E__;
  readStatus();
  
  if (loopbackIndex == 0) {
    for (int nChain = 0; nChain < numberOfChains; nChain++) {
      for (int nDTC = 0; nDTC < numberOfDTCsPerChain; nDTC++) {
	for (int nROC = 0; nROC < numberOfROCsPerDTC; nROC++) {
	  delay[nChain][nDTC][nROC] = -1;
	}
      }
    }
  }
  //=========== Perform loopback=============
  
  // where are we in the procedure?
  int activeROC = loopbackIndex % numberOfROCsPerDTC ;
  
  int activeDTC = -1;
  
  for (int nDTC = 0; nDTC < numberOfDTCsPerChain; nDTC++) { 
    //	__COUT__ << "loopback index = " << loopbackIndex
    //		<< " nDTC = " << nDTC
    //		<< " numberOfDTCsPerChain = " << numberOfDTCsPerChain
    //		<< __E__;
    if ( 	loopbackIndex >= (nDTC * numberOfROCsPerDTC) &&
		loopbackIndex < ((nDTC+1) * numberOfROCsPerDTC ) ) {
      //				__COUT__ << "ACTIVE DTC " << nDTC << __E__;
      activeDTC = nDTC;
    }
  }
  
  //	__MOUT__ 	<< "loopback index = " << loopbackIndex;
  __MCOUT_INFO__(" Looping back DTC" << activeDTC << " ROC" << activeROC << __E__);
  
  int chainIndex = 0;
  
  while( (chainIndex < numberOfChains) ) {
    
    // clean up after the DTC has done all of its resetting... 
    __COUT__ << "LOOPBACK: CFO reset serdes RX " << __E__;
    registerWrite(0x9118,0x000000ff);
    registerWrite(0x9118,0x0);
    sleep(5);
    
    // 	__COUT__ << "LOOPBACK: on DTC " << link[chainIndex] <<__E__;
    
    delay[chainIndex][activeDTC][activeROC] = MeasureLoopback(link[chainIndex]);
    
    __COUT__ << "LOOPBACK: link " << link[chainIndex] << " -> delay = " << delay[chainIndex][activeDTC][activeROC] 
	     << " (dec)" << __E__;	
    
    chainIndex++;
    
  } // (chainIndex < numberOfChains && WorkLoop::continueWorkLoop_)
  
  indicateIterationWork(); // I still need to be touched
  return;
}

//========================================================================================================================
void CFOFrontEndInterface::stop(void)
{
  __COUT__ << "STOP: CFO status" << __E__;
  readStatus();
}

//========================================================================================================================
bool CFOFrontEndInterface::running(void)
{
  while(WorkLoop::continueWorkLoop_) {
    
    break; 
  } 
  
  return false;
}

DEFINE_OTS_INTERFACE(CFOFrontEndInterface)
