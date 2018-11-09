#ifndef _ots_FrontEndInterfaceTemplate_h_
#define _ots_FrontEndInterfaceTemplate_h_

#include "otsdaq-core/FECore/FEVInterface.h"
#include <string>
#include <map>
#include "dtcInterfaceLib/DTC.h"
#include "mu2e_driver/mu2e_mmap_ioctl.h"	// m_ioc_cmd_t

namespace ots
{
	
	//class FrontEndHardwareTemplate;
	//class FrontEndFirmwareTemplate;
	
	class DTCFrontEndInterface: public FEVInterface
	{
		
	public:
		DTCFrontEndInterface (const std::string& interfaceUID, const ConfigurationTree& theXDAQContextConfigTree, const std::string& interfaceConfigurationPath);
		
		virtual ~DTCFrontEndInterface(void);
		
		// state machine
		//----------------
		void configure (void);
		void halt (void);
		void pause (void);
		void resume (void);
		void start (std::string runNumber);
		void stop (void);
		bool running (void);
		
		
		// hardware access
		//----------------
		int universalRead (char* address, char* readValue ) override;
		void universalWrite (char* address, char* writeValue) override;
		int registerRead (int address );
		int registerWrite (int address, int dataToWrite); //return read value after having written dataToWrite
		
		
		// DTC specific items
		void configureJitterAttenuator(void);
		void readStatus(void);
		float readTemperature(); // return temperature of FPGA in degC
		void printVoltages();

		void turnOnLED(); // turn on LED on visible side of timing card
		void turnOffLED(); // turn off LED on visible side of timing card

		int getROCLinkStatus(int ROC_link);
		int getCFOLinkStatus();
		int checkLinkStatus();

	protected:
		//FrontEndHardwareTemplate* theFrontEndHardware_;
		//FrontEndFirmwareTemplate* theFrontEndFirmware_;
		
	private:
		char devfile_[11];
		int fd_;
		int dtc_ = -1;
		int dtc_location_in_chain_ = -1;
		std::string device_name_;
		DTCLib::DTC* thisDTC_ ;
		
		m_ioc_reg_access_t reg_access_; 
		
		
	public: 
		void ReadROC(frontEndMacroInArgs_t argsIn, frontEndMacroOutArgs_t argsOut);
		void WriteROC(frontEndMacroInArgs_t argsIn, frontEndMacroOutArgs_t argsOut);
		void WriteROCBlock(frontEndMacroInArgs_t argsIn, frontEndMacroOutArgs_t argsOut);
		void ReadROCBlock(frontEndMacroInArgs_t argsIn, frontEndMacroOutArgs_t argsOut);
};
}
#endif
