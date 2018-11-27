#ifndef _ots_ROCInterface_h_
#define _ots_ROCInterface_h_

#include <string>
#include <sstream>
#include "dtcInterfaceLib/DTC.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Configurable/Configurable.h"


namespace ots
{
	

class ROCInterface : public Configurable
{
  
 public:
  ROCInterface (const unsigned int linkID, DTCLib::DTC* thisDTC, const unsigned int delay, const ConfigurationTree& theXDAQContextConfigTree, const std::string& theConfigurationPath);
  
  ~ROCInterface(void);
	  
  // state machine
  //----------------
  void configure (void);
  void halt (void);
  void pause (void);
  void resume (void);
  void start (std::string runNumber);
  void stop (void);
  bool running (void);
  
  std::string getIdString(void) { std::stringstream ss; ss << "ROC" << linkID_; return ss.str();}

  // write and read to registers
  void writeRegister (unsigned address, unsigned data_to_write);
  int readRegister (unsigned address);

  //specific ROC functions
  int readTimestamp();
  void writeDelay(unsigned delay); //5ns steps
  int readDelay();                 //5ns steps

 private:
  DTCLib::DTC_Link_ID linkID_;
  DTCLib::DTC* 	      thisDTC_;
  unsigned int 	      delay_;
  
};

}

#endif
