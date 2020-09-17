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
	void 								configure					(void);
	void 								halt						(void);
	void 								pause						(void);
	void 								resume						(void);
	void 								start						(std::string runNumber);
	void 								stop						(void);
	bool 								running						(void);

	// emulator handlers
	//----------------
	void 								emulatorConfigure			(void);

	// hardware access
	//----------------
//	void 								universalRead				(char* address, char* readValue) override;
//	void 								universalWrite				(char* address, char* writeValue) override;
//	dtc_data_t							registerRead				(const dtc_address_t address);
//	dtc_data_t  						registerWrite				(const dtc_address_t address, dtc_data_t dataToWrite);  // return read value after having written dataToWrite

	// DTC specific items
	//----------------
	//void  								configureJitterAttenuator	(void);
	virtual void  						readStatus					(void) override;
	//float 								readTemperature				(void);  // return temperature of FPGA in degC
	//void  								printVoltages				(void);

	//void 								turnOnLED					(void);   // turn on LED on visible side of timing card
	//void 								turnOffLED					(void);  // turn off LED on visible side of timing card

	bool 								ROCActive					(unsigned int ROC_link);
	int  								getROCLinkStatus			(int ROC_link);
	int  								getCFOLinkStatus			(void);
	int  								checkLinkStatus				(void);

	DTCLib::DTC* 									thisDTC_;

  private:
	void 								createROCs					(void);
	void 								registerFEMacros			(void);

//	char        									devfile_[11];
//	int         									fd_;
//	int         									dtc_                   = -1;
	int         									dtc_location_in_chain_ = -1;
	//bool        									configure_clock_       = 0;
	unsigned    									roc_mask_              = 0;
	//std::string 									device_name_;
	//bool      										emulatorMode_;
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

	m_ioc_reg_access_t 								reg_access_;

	unsigned 										initial_9100_ = 0;
	unsigned 										initial_9114_ = 0;

	std::ofstream 									outputStream;

  public:
	void 								ReadROC						(__ARGS__);
	void 								WriteROC					(__ARGS__);
	void 								WriteROCBlock				(__ARGS__);
	void 								ReadROCBlock				(__ARGS__);
    void                               BlockReadROC                 (__ARGS__);
	void 								DTCHighRateBlockCheck		(__ARGS__);
	void 								DTCReset					(__ARGS__);
	void 								DTCReset					(void);
	void 								DTCHighRateDCSCheck			(__ARGS__);
	void 								RunROCFEMacro				(__ARGS__);
	void 								DTCSendHeartbeatAndDataRequest(__ARGS__);
	void								ResetLossOfLockCounter		(__ARGS__);
	void								ReadLossOfLockCounter		(__ARGS__);
	void								GetUpstreamControlLinkStatus(__ARGS__);
	void								ShutdownLinkTx				(__ARGS__);
	void								StartupLinkTx				(__ARGS__);
	void								WriteDTC					(__ARGS__);
	void								ReadDTC						(__ARGS__);
	// clang-format on
};
}  // namespace ots
#endif
