#ifndef _ots_CFOFrontEndInterface_h_
#define _ots_CFOFrontEndInterface_h_

#include <map>
#include <string>
#include "otsdaq-mu2e/CFOandDTCCore/CFOandDTCCoreVInterface.h"

#include "cfoInterfaceLib/CFO_Registers.h"
#include "cfoInterfaceLib/CFO_Compiler.hh"

namespace ots
{
class CFOFrontEndInterface : public CFOandDTCCoreVInterface
{
	// clang-format off

  public:
	CFOFrontEndInterface(const std::string&       interfaceUID,
	                     const ConfigurationTree& theXDAQContextConfigTree,
	                     const std::string&       interfaceConfigurationPath);

	virtual ~CFOFrontEndInterface(void);

	// state machine
	//----------------
	void 								configure					(void) override;
	void 								halt						(void) override;
	void 								pause						(void) override;
	void 								resume						(void) override;
	void 								start						(std::string runNumber) override;
	void 								stop						(void) override;
	bool 								running						(void) override;

	// CFO specific items
	//----------------
	float 								MeasureLoopback				(int linkToLoopback); //pre-covid loopback calculation stretegy
	uint32_t 							measureDelay				(CFOLib::CFO_Link_ID link); //post-covid loopback function

	// int  								getLinkStatus				(void);
	void 								configureEventBuildingMode	(int step = -1);
	void 								configureLoopbackMode		(int step = -1);
	void 								configureForTimingChain		(int step = -1);
	void								loopbackTest				(std::string runNumber, int step = -1);


	// hardware access
	//----------------
	virtual mu2edev* 					getDevice					(void) {return thisCFO_->GetDevice();};

	float 								delay[8][6][8];
	float 								delay_rms[8][6][8];
	float 								delay_failed[8][6][8];

  protected: 
  	

  private:
	void 								registerFEMacros			(void);
	
	CFOLib::CFO_Registers* 				thisCFO_;
	int									timing_chain_first_substep_	   = -1;
	//int                    configure_clock_ = 0;

	int          						loopback_distribution_[10000];
	unsigned int 						min_distribution_;
	unsigned int 						max_distribution_;
	float        						average_loopback_;
	float        						rms_loopback_;
	float        						failed_loopback_;

  public:
	// void 								FlashLEDs						(__ARGS__);	
	void 								GetFirmwareVersion				(__ARGS__);
	void 								GetStatus						(__ARGS__);
	void 								GetCounters						(__ARGS__);
	void 								GetFPGATemperature				(__ARGS__);
	void								SelectJitterAttenuatorSource	(__ARGS__);


	void 								CFOReset						(__ARGS__);
	void 								CFOHalt							(__ARGS__);
	
	void 								WriteCFO						(__ARGS__);
	void 								ReadCFO							(__ARGS__);
	void 								ResetRunplan					(__ARGS__);
	void 								CompileRunplan					(__ARGS__);
	void 								SetRunplan						(__ARGS__);
	void 								LaunchRunplan					(__ARGS__);
	void 								ConfigureForTimingChain			(__ARGS__);
	void								LoopbackTest					(__ARGS__);
	void 								TestMarker						(__ARGS__);
};

// clang-format on
}  // namespace ots

#endif
