#ifndef _ots_ROCInterface_h_
#define _ots_ROCInterface_h_

#include <string>
#include <sstream>
#include "dtcInterfaceLib/DTC.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
//#include "otsdaq-core/Configurable/Configurable.h"



class ROCInterface// : public Configurable
{
  
 public:
  ROCInterface (const unsigned int linkID, DTCLib::DTC* thisDTC, const unsigned int delay);
  
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
		
 private:
  DTCLib::DTC_Link_ID 		linkID_;
  DTCLib::DTC* 				thisDTC_;
  unsigned int 				delay_;
    
};

#endif
