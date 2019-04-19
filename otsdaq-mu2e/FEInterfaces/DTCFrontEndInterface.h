#ifndef _ots_DTCFrontEndInterface_h_
#define _ots_DTCFrontEndInterface_h_

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include "dtcInterfaceLib/DTC.h"
#include "mu2e_driver/mu2e_mmap_ioctl.h"  // m_ioc_cmd_t
#include "otsdaq-core/FECore/FEVInterface.h"
#include "otsdaq-mu2e/ROCCore/ROCCoreVInterface.h"

namespace ots
{
// class FrontEndHardwareTemplate;
// class FrontEndFirmwareTemplate;

class DTCFrontEndInterface : public FEVInterface
{
  public:
	DTCFrontEndInterface(const std::string&       interfaceUID,
	                     const ConfigurationTree& theXDAQContextConfigTree,
	                     const std::string&       interfaceConfigurationPath);

	virtual ~DTCFrontEndInterface(void);

	// state machine
	//----------------
	void configure(void);
	void halt(void);
	void pause(void);
	void resume(void);
	void start(std::string runNumber);
	void stop(void);
	bool running(void);

	// emulator handlers
	void emulatorConfigure(void);

	// hardware access
	//----------------
	void universalRead(char* address, char* readValue) override;
	void universalWrite(char* address, char* writeValue) override;
	int  registerRead(int address);
	int  registerWrite(
	     int address,
	     int dataToWrite);  // return read value after having written dataToWrite

	// DTC specific items
	void  configureJitterAttenuator(void);
	void  readStatus(void);
	float readTemperature();  // return temperature of FPGA in degC
	void  printVoltages();

	void turnOnLED();   // turn on LED on visible side of timing card
	void turnOffLED();  // turn off LED on visible side of timing card

	bool ROCActive(unsigned ROC_link);
	int  getROCLinkStatus(int ROC_link);
	int  getCFOLinkStatus();
	int  checkLinkStatus();

	DTCLib::DTC* thisDTC_;

  private:
	void createROCs(void);
	void registerFEMacros(void);

	char        devfile_[11];
	int         fd_;
	int         dtc_                   = -1;
	int         dtc_location_in_chain_ = -1;
	bool        configure_clock_       = 0;
	unsigned    roc_mask_              = 0;
	std::string device_name_;
	bool        emulatorMode_;
	int         emulate_cfo_           = 0;

	std::ofstream datafile_[8];

	std::map<std::string /*name*/, std::unique_ptr<ROCCoreVInterface>> rocs_;
	std::map<std::string /*DTC's FEMacro name*/,
	std::pair<std::string /*ROC UID*/, std::string /*ROC's FEMacro name*/>> rocFEMacroMap_;

	m_ioc_reg_access_t reg_access_;

	unsigned initial_9100_ = 0;
	unsigned initial_9114_ = 0;

  public:
	void ReadROC(__ARGS__);
	void WriteROC(__ARGS__);
	void WriteROCBlock(__ARGS__);
	void ReadROCBlock(__ARGS__);
	void DTCReset(__ARGS__);
	void DTCReset();
	void DTCHighRateDCSCheck(__ARGS__);
	void RunROCFEMacro(__ARGS__);
};
}  // namespace ots
#endif
