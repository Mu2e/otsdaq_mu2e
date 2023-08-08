#ifndef _ots_DTCFrontEndInterface_h_
#define _ots_DTCFrontEndInterface_h_

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include "dtcInterfaceLib/DTC.h"
#include "dtcInterfaceLib/DTCSoftwareCFO.h"
#include "mu2e_driver/mu2e_mmap_ioctl.h"  // m_ioc_cmd_t, m_ioc_reg_access_t, dtc_address_t, dtc_data_t
#include "otsdaq-mu2e/CFOandDTCCore/CFOandDTCCoreVInterface.h"
#include "otsdaq-mu2e/ROCCore/ROCCoreVInterface.h"

namespace ots
{
// class FrontEndHardwareTemplate;
// class FrontEndFirmwareTemplate;

class DTCFrontEndInterface : public CFOandDTCCoreVInterface
{
	// clang-format off
  public:
	DTCFrontEndInterface(const std::string&       interfaceUID,
	                     const ConfigurationTree& theXDAQContextConfigTree,
	                     const std::string&       interfaceConfigurationPath);

	virtual ~DTCFrontEndInterface(void);

	// specialized ROC handling slow controls
	//----------------
	virtual void 						configureSlowControls		(void) override;
	virtual void						resetSlowControlsChannelIterator (void) override;
	virtual FESlowControlsChannel*		getNextSlowControlsChannel	(void) override;
	virtual unsigned int				getSlowControlsChannelCount	(void) override;
	virtual void						getSlowControlsValue		(FESlowControlsChannel& channel, std::string& readValue) override;
  private:
	bool											currentChannelIsInROC_;
	std::string										currentChannelROCUID_;

  public:
	// state machine
	//----------------
	void 								configure					(void) override;
	void 								halt						(void) override;
	void 								pause						(void) override;
	void 								resume						(void) override;
	void 								start						(std::string runNumber) override;
	void 								stop						(void) override;
	bool 								running						(void) override;

	// emulator handlers
	//----------------
	void 								emulatorConfigure			(void);

	// hardware access
	//----------------
	virtual mu2edev* 					getDevice					(void) {return thisDTC_->GetDevice();};
//	void 								universalRead				(char* address, char* readValue) override; //defined in 
//	void 								universalWrite				(char* address, char* writeValue) override;
	// dtc_data_t							registerRead				(dtc_address_t address);
	// virtual	dtc_data_t					registerWrite				(dtc_address_t address, dtc_data_t dataToWrite) override;  // return read value after having written dataToWrite

	// DTC specific items
	//----------------
	virtual std::string					readStatus					(void) override;
	void 								configureEventBuildingMode	(void);
	void 								configureLoopbackMode		(void);
	void 								configureForTimingChain		(int step);

	// bool 								ROCActive					(unsigned int ROC_link);
	// int  								getROCLinkStatus			(int ROC_link);
	// int  								getCFOLinkStatus			(void);
	// int  								checkLinkStatus				(void);

	DTCLib::DTC* 									thisDTC_;

  private:
	void 								createROCs					(void);
	void 								registerFEMacros			(void);

	int         									dtc_location_in_chain_ = -1;
	unsigned int   									roc_mask_              = 0;
	int         									emulate_cfo_           = 0;
	DTCLib::DTCSoftwareCFO* 						EmulatedCFO_;

	std::ofstream datafile_[8];

	std::map<std::string /*ROC UID*/,
		std::unique_ptr<ROCCoreVInterface>> 		rocs_;

	std::map<std::string /*DTC's FEMacro name*/,
		std::pair<std::string /*ROC UID*/,
			std::string /*ROC's FEMacro name*/>> 	rocFEMacroMap_;

	std::map<std::string /* ROC UID*/, 
		FESlowControlsChannel> 						mapOfROCSlowControlsChannels_;

	// m_ioc_reg_access_t 								reg_access_;

	// dtc_data_t 										initial_9100_ = 0;
	// dtc_data_t 										initial_9114_ = 0;

	std::ofstream 									outputStream;


  public:
	// void 								FlashLEDs						(__ARGS__);	
	void 								GetFirmwareVersion				(__ARGS__);
	void 								GetStatus						(__ARGS__);
	void 								GetSimpleStatus					(__ARGS__);

	// FIXME -- copy from CFOandDTC and implement using DTC.h
	// void 								GetLinkLossOfLight				(__ARGS__);
	// void 								GetFireflyTemperature			(__ARGS__);
	// void								ResetLinkRx						(__ARGS__);
	// void								ShutdownLinkTx					(__ARGS__);
	// void								StartupLinkTx					(__ARGS__);
	// void								ShutdownFireflyTx				(__ARGS__);
	// void								StartupFireflyTx				(__ARGS__);

	void 								ReadROC							(__ARGS__);
	void 								WriteROC						(__ARGS__);
	// void 								WriteROCBlock					(__ARGS__);
	// void 								ReadROCBlock					(__ARGS__);
    // void                              	BlockReadROC                	(__ARGS__);
	// void 								DTCHighRateBlockCheck			(__ARGS__);
	void 								DTCReset						(__ARGS__);
	void 								DTCReset						(void);
	void 								DTCHighRateDCSCheck				(__ARGS__);
	void 								RunROCFEMacro					(__ARGS__);
	void 								DTCSendHeartbeatAndDataRequest	(__ARGS__);
	void								ResetLossOfLockCounter			(__ARGS__);
	void								ReadLossOfLockCounter			(__ARGS__);
	void								GetUpstreamControlLinkStatus	(__ARGS__);
	void								SelectJitterAttenuatorSource	(__ARGS__);
	void								WriteDTC						(__ARGS__);
	void								ReadDTC							(__ARGS__);
	void								SetEmulatedROCEventFragmentSize	(__ARGS__);
	void								configureHardwareDevMode		(__ARGS__);
	void 								configureHardwareDevMode		(void);
	void								ConfigureForTimingChain			(__ARGS__);
	void 								BufferTestROC					(__ARGS__);
	void 								DTCCounters						(__ARGS__);
	void 								readRxDiagFIFO					(__ARGS__);
	void 								readTxDiagFIFO					(__ARGS__);
	void 								GetLinkErrors					(__ARGS__);
	void 								ROCResetLink					(__ARGS__);
	void								HeaderFormatTest				(__ARGS__);


	// void 								ROCDestroy						(__ARGS__);
	// void 								ROCInstantiate					(__ARGS__);
	void 								DMABufferRelease				(__ARGS__);
	void 								DTCInstantiate					(__ARGS__);
	void 								ResetDTCLinks					(__ARGS__);
	
	// clang-format on
};
}  // namespace ots
#endif
