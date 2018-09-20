#ifndef _ots_FrontEndInterfaceTemplate_h_
#define _ots_FrontEndInterfaceTemplate_h_

#include "otsdaq-core/FECore/FEVInterface.h"
#include <string>
#include <map>

namespace ots
{

  //class FrontEndHardwareTemplate;
  //class FrontEndFirmwareTemplate;

class CFOFrontEndInterface: public FEVInterface
{

public:
	CFOFrontEndInterface (const std::string& interfaceUID, const ConfigurationTree& theXDAQContextConfigTree, const std::string& interfaceConfigurationPath);

	virtual ~CFOFrontEndInterface(void);

	void configure        (void);
	void halt             (void);
	void pause            (void);
	void resume           (void);
	void start            (std::string runNumber);
	void stop             (void);
    bool running   		  (void);

    int  universalRead	  (char* address, char* readValue) 	override;
    void universalWrite	  (char* address, char* writeValue) override;

    float MeasureLoopback(int linkToLoopback);

protected:
    //FrontEndHardwareTemplate* theFrontEndHardware_;
    //FrontEndFirmwareTemplate* theFrontEndFirmware_;
};

}

#endif
