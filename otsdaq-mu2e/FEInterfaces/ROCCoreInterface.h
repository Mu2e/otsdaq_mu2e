#ifndef _ots_ROCCoreInterface_h_
#define _ots_ROCCoreInterface_h_

#include <string>
#include <sstream>
#include "dtcInterfaceLib/DTC.h"
#include "otsdaq-core/FECore/FEVInterface.h"


namespace ots
{
	

class ROCCoreInterface : public FEVInterface
{
  
 public:
  ROCCoreInterface (const std::string& rocUID,
		  const ConfigurationTree& theXDAQContextConfigTree,
		  const std::string& interfaceConfigurationPath);
  
  ~ROCCoreInterface(void);

  std::string				getInterfaceType    			(void) const override {return theXDAQContextConfigTree_.getBackNode(theConfigurationPath_).getNode("ROCInterfacePluginName").getValue<std::string>();}//interfaceType_;}

  // state machine
  //----------------
  void configure (void);
  void halt (void);
  void pause (void);
  void resume (void);
  void start (std::string runNumber);
  void stop (void);
  bool running (void);
  

  //----------------
  //just to keep FEVInterface, defining universal read..
  int universalRead (char* address, char* readValue ) override { __SS__ << "Not defined. Parent should be DTCFrontEndInterface, not FESupervisor." << __E__; __SS_THROW__; }
  void universalWrite (char* address, char* writeValue) override { __SS__ << "Not defined. Parent should be DTCFrontEndInterface, not FESupervisor." << __E__; __SS_THROW__; }
  //----------------

  // write and read to registers
  virtual void writeRegister (unsigned address, unsigned data_to_write);
  virtual int readRegister (unsigned address);

  //specific ROC functions
  int readTimestamp();
  void writeDelay(unsigned delay); //5ns steps
  int readDelay();                 //5ns steps

  inline int getLinkID() { return linkID_; }

  bool					emulatorMode_;
  DTCLib::DTC* 	      	thisDTC_;

 private:
  DTCLib::DTC_Link_ID 	linkID_;
  unsigned int 	      	delay_;

  
};

}

#endif
