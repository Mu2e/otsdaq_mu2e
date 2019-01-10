#ifndef _ots_CAPTANSignalGenerator_h_
#define _ots_CAPTANSignalGenerator_h_

#include "otsdaq-core/FECore/FEVInterface.h"
#include "otsdaq-components/DAQHardware/OtsUDPHardware.h"
#include "otsdaq-components/DAQHardware/OtsUDPFirmwareDataGen.h"

#include <string>

namespace ots
{

class CAPTANSignalGenerator	: public FEVInterface, public OtsUDPHardware, public OtsUDPFirmwareDataGen
{

public:
	CAPTANSignalGenerator 			(const std::string& interfaceUID, const ConfigurationTree& theXDAQContextConfigTree, const std::string& interfaceConfigurationPath);
	virtual ~CAPTANSignalGenerator	(void);

	void configure 		(void) override;
	void halt 		 	(void) override;
	void pause 		 	(void) override;
	void resume 	 	(void) override;
	void start 		 	(std::string runNumber) override;
	void stop 		 	(void) override;
	bool running 		(void) override;

	virtual void	universalRead			(char* address, char* readValue) override;
	virtual void 	universalWrite			(char* address, char* writeValue) override;

private:


public: // FEMacro 'varTest' generated, Oct-11-2018 11:36:28, by 'admin' using MacroMaker.
	void varTest	(__ARGS__);

public: // FEMacro 'varTest2' generated, Oct-11-2018 02:28:57, by 'admin' using MacroMaker.
	void varTest2	(__ARGS__);
};

}

#endif
