#ifndef _ots_CFOandDTCCoreVInterface_h_
#define _ots_CFOandDTCCoreVInterface_h_

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <mutex>
#include "dtcInterfaceLib/mu2edev.h"
//#include "dtcInterfaceLib/DTC.h"
//#include "dtcInterfaceLib/DTCSoftwareCFO.h"
#include "mu2e_driver/mu2e_mmap_ioctl.h"  // m_ioc_cmd_t, m_ioc_reg_access_t, dtc_address_t, dtc_data_t

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

	static std::string					CONFIG_MODE_HARDWARE_DEV;
	static std::string					CONFIG_MODE_EVENT_BUILDING;
	static std::string					CONFIG_MODE_LOOPBACK;

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
	virtual mu2edev* 					getDevice					(void) = 0;

	// DTC specific items
	//----------------
	// void  								configureJitterAttenuator	(void);
	virtual std::string					readStatus					(void) = 0;
	// float 								readTemperature				(void);  // return temperature of FPGA in degC
	// std::string							printVoltages				(void);

	// void 								turnOnLED					(void);  // turn on LED on visible side of timing card
	// void 								turnOffLED					(void);  // turn off LED on visible side of timing card

  protected:
	
	void 								registerCFOandDTCFEMacros	(void);

	int         						deviceIndex_		         	= -1; //PCIe index
	bool        						configure_clock_    			= false;
	bool      							emulatorMode_					= false;
	bool 								skipInit_						= true;
	std::string							operatingMode_ 					= "";

	static const int					CONFIG_DTC_TIMING_CHAIN_START_INDEX = 1;
	static const int					CONFIG_DTC_TIMING_CHAIN_STEPS = 3;

	bool 								artdaqMode_ = false; // true to prevent run data file generation


  public: 
	// std::string							GetFirmwareVersion			(void);
		// void								ResetLinkRx					(__ARGS__);
	// void								ShutdownLinkTx				(__ARGS__);
	// void								StartupLinkTx				(__ARGS__);
	// void								ShutdownFireflyTx			(__ARGS__);
	// void								StartupFireflyTx			(__ARGS__);


	// clang-format on
};
}  // namespace ots
#endif
