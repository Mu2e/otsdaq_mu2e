#ifndef _ots_CAPTANSignalGenerator_h_
#define _ots_CAPTANSignalGenerator_h_

#include "otsdaq-components/DAQHardware/OtsUDPFirmwareDataGen.h"
#include "otsdaq-components/DAQHardware/OtsUDPHardware.h"
#include "otsdaq/FECore/FEVInterface.h"

#include <string>

namespace ots
{
class CAPTANSignalGenerator : public FEVInterface,
                              public OtsUDPHardware,
                              public OtsUDPFirmwareDataGen
{
  public:
	CAPTANSignalGenerator(const std::string&       interfaceUID,
	                      const ConfigurationTree& theXDAQContextConfigTree,
	                      const std::string&       interfaceConfigurationPath);
	virtual ~CAPTANSignalGenerator(void);

	// state machine
	//----------------
	void configure(void) override;
	void halt(void) override;
	void pause(void) override;
	void resume(void) override;
	void start(std::string runNumber) override;
	void stop(void) override;
	bool running(void) override;

	virtual void universalRead(char* address, char* readValue) override;
	virtual void universalWrite(char* address, char* writeValue) override;

	void getFirmwareVersion(__ARGS__);
	void getPulsePeriod(__ARGS__);
	void setPulsePeriod(__ARGS__);
	void getManualMode(__ARGS__);
	void setManualMode(__ARGS__);
	void setManualMode(uint64_t manualMode);
	void setupBurstMode(__ARGS__);

  private:
	enum RTF_Register : uint64_t
	{
		// READ ONLY REGISTERS
		FirmwareVersion 			= 0x0,
		ManualModeStatus 			= 0x1,
		TriggerPeriod 				= 0x2,
		ClockSourcePLL 				= 0x3,
		PLLCounterLossLock200MHz	= 0x4,
		PLLCounterLossLockRF0		= 0x5,

		// WRITE ONLY REGISTERS
		ManualMode					= 0x9,
		BurstCount 	  				= 0xA,
		TriggerFrequency 			= 0xB,
	};
  public:  // FEMacro 'varTest' generated, Oct-11-2018 11:36:28, by 'admin' using
	       // MacroMaker.
	void varTest(__ARGS__);

  public:  // FEMacro 'varTest2' generated, Oct-11-2018 02:28:57, by 'admin' using
	       // MacroMaker.
	void varTest2(__ARGS__);
};

}  // namespace ots

#endif
