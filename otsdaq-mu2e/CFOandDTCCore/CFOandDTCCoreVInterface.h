#ifndef _ots_CFOandDTCCoreVInterface_h_
#define _ots_CFOandDTCCoreVInterface_h_

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <mutex>
//#include "dtcInterfaceLib/DTC.h"
//#include "dtcInterfaceLib/DTCSoftwareCFO.h"
#include "mu2e_driver/mu2e_mmap_ioctl.h"  // m_ioc_cmd_t, m_ioc_reg_access_t, dtc_address_t, dtc_data_t

#include "otsdaq-mu2e/ROCCore/ROCCoreVInterface.h"
#include "otsdaq/FECore/FEVInterface.h"

namespace ots
{
class CFOandDTCCoreVInterface : public FEVInterface
{
	// clang-format off
  public:
	CFOandDTCCoreVInterface(const std::string&       interfaceUID,
							const ConfigurationTree& theXDAQContextConfigTree,
							const std::string&       interfaceConfigurationPath);

	virtual ~CFOandDTCCoreVInterface(void);

	// specialized handling of slow controls
	//----------------		
	void 								outputEpicsPVFile			(ConfigurationManager* configManager);
	
  
  public:
	// state machine
	//----------------
//	void 								configure					(void);
//	void 								halt						(void);
//	void 								pause						(void);
//	void 								resume						(void);
//	void 								start						(std::string runNumber);
//	void 								stop						(void);
//	bool 								running						(void);

	// emulator handlers
	//----------------
//	void 								emulatorConfigure			(void);

	// hardware access
	//----------------
	void 								universalRead				(char* address, char* readValue) override;
	void 								universalWrite				(char* address, char* writeValue) override;
	dtc_data_t							registerRead				(dtc_address_t address);
	virtual dtc_data_t					registerWrite				(dtc_address_t address, dtc_data_t dataToWrite);  // return read value after having written dataToWrite
	void								readbackVerify				(dtc_address_t address, dtc_data_t dataToWrite, dtc_data_t readbackValue);

	// DTC specific items
	//----------------
	void  								configureJitterAttenuator	(void);
	virtual std::string					readStatus					(void) = 0;
	float 								readTemperature				(void);  // return temperature of FPGA in degC
	std::string							printVoltages				(void);

	void 								turnOnLED					(void);  // turn on LED on visible side of timing card
	void 								turnOffLED					(void);  // turn off LED on visible side of timing card

  protected:
	
	void 								registerCFOandDTCFEMacros	(void);

	char        						devfile_[11];
	int         						fd_;
	int         						device_                   	= -1;
	bool        						configure_clock_       = 0;
	std::string 						device_name_;
	bool      							emulatorMode_;
	std::mutex 							readWriteOperationMutex_;

	m_ioc_reg_access_t 					reg_access_;
	unsigned 							initial_9100_ 				= 0;
	unsigned 							initial_9114_ 				= 0;
	std::ofstream 						outputStream;

  public:
	void 								FlashLEDs					(__ARGS__);	
	void 								GetFirmwareVersion			(__ARGS__);
	std::string							GetFirmwareVersion			(void);
	void 								GetStatus					(__ARGS__);


	// clang-format on
};
}  // namespace ots
#endif
