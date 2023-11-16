#ifndef _ots_ROCCoreVInterface_h_
#define _ots_ROCCoreVInterface_h_

#include <sstream>
#include <string>
#include "dtcInterfaceLib/DTC.h" /* for DTC data types*/
#include "otsdaq/FECore/FEVInterface.h"

namespace ots
{
class ROCCoreVInterface : public FEVInterface
{
	// clang-format off
  public:
	ROCCoreVInterface(
		const std::string&       					rocUID,
        const ConfigurationTree& 					theXDAQContextConfigTree,
        const std::string&       					interfaceConfigurationPath);

	~ROCCoreVInterface(void);

	virtual std::string 					getInterfaceType			(void) const override
	{
		return theXDAQContextConfigTree_.getBackNode(theConfigurationPath_)
			.getNode(FEVInterface::interfaceUID_)
		    .getNode("ROCInterfacePluginName")
		    .getValue<std::string>();
	}

	// state machine
	//----------------
	void 									configure					(void) override;
	void 									halt						(void) override;
	void 									pause						(void) override;
	void 									resume						(void) override;
	void 									start						(std::string runNumber) override;
	void 									stop						(void) override;
	bool 									running						(void) override;

	//----------------
	// just to keep FEVInterface, defining universal read..
	void 									universalRead				(
		char* 										address, 
		char* 										readValue) override
	{
		__SS__ << "Not defined. Should never be called. Parent should be "
		          "DTCFrontEndInterface, not "
		          "FESupervisor."
		       << __E__;
		__SS_THROW__;
	}
	void 									universalWrite				(
		char* 										address, 
		char* 										writeValue) override
	{
		__SS__ << "Not defined. Should never be called. Parent should be "
		          "DTCFrontEndInterface, not "
		          "FESupervisor."
		       << __E__;
		__SS_THROW__;
	}
	//----------------

	// write and read to registers
	//	Philosophy: call writeRegister/readRegister/readBlock and it will choose the ROC or software emulator implementation
	//      For each there is a "ROC" and "Emulator" version:   readROCRegister/readEmulatorRegister/readROCBlock/readEmulatorBlock/writeROCRegister/writeEmulatorRegister 
	void         							writeRegister				(DTCLib::roc_address_t address, DTCLib::roc_data_t writeData);  // chooses ROC or Emulator version
	DTCLib::roc_data_t 						readRegister				(DTCLib::roc_address_t address);     // chooses ROC or Emulator version
	void 									readBlock					(std::vector<DTCLib::roc_data_t>& data, DTCLib::roc_address_t address, uint16_t wordCount, bool incrementAddress);     // chooses ROC or Emulator version
	void 									writeBlock					(const std::vector<DTCLib::roc_data_t>& writeData, DTCLib::roc_address_t address, bool incrementAddress, bool requestAck = true);     // chooses ROC or Emulator version

	virtual void 							writeROCRegister			(DTCLib::roc_address_t address, DTCLib::roc_data_t writeData) = 0;  // pure virtual, must define in inheriting children
	virtual DTCLib::roc_data_t				readROCRegister				(DTCLib::roc_address_t address) = 0;  // pure virtual, must define in inheriting children
	virtual void 							readROCBlock				(std::vector<DTCLib::roc_data_t>& data, DTCLib::roc_address_t address, uint16_t wordCount, bool incrementAddress) {throw std::runtime_error("UNDEFINED BLOCK ROC READ");}; // pure virtual, must define in inheriting children
	virtual void 							writeROCBlock				(const std::vector<DTCLib::roc_data_t>& writeData, DTCLib::roc_address_t address, bool incrementAddress, bool requestAck = true) {throw std::runtime_error("UNDEFINED BLOCK ROC WRITE");};     // pure virtual, must define in inheriting children

	virtual void 							writeEmulatorRegister		(DTCLib::roc_address_t address, DTCLib::roc_data_t writeData) = 0;  // pure virtual, must define in inheriting children
	virtual DTCLib::roc_data_t				readEmulatorRegister		(DTCLib::roc_address_t address) = 0;  // pure virtual, must define in inheriting children
	virtual void 							readEmulatorBlock			(std::vector<DTCLib::roc_data_t>& data, DTCLib::roc_address_t address, uint16_t wordCount, bool incrementAddress) {throw std::runtime_error("UNDEFINED BLOCK EMULATOR READ");}; // pure virtual, must define in inheriting children
	virtual void 							writeEmulatorBlock			(const std::vector<DTCLib::roc_data_t>& writeData, DTCLib::roc_address_t address, bool incrementAddress, bool requestAck = true) {throw std::runtime_error("UNDEFINED BLOCK EMULATOR WRITE");};     // pure virtual, must define in inheriting children


	// pure virtual specific ROC functions
	virtual void       					    GetStatus  					(void) = 0;  // pure virtual, must define in inheriting children
	virtual void       					    GetFirmwareVersion			(void) = 0;  // pure virtual, must define in inheriting children
	virtual int  							readTimestamp				(void) = 0;  // pure virtual, must define in inheriting children
	virtual void 							writeDelay					(uint16_t delay) = 0;  // 5ns steps // pure virtual, must
	                						                			              // define in inheriting children
	virtual int								readDelay					(void) = 0;  // 5ns steps // pure virtual, must define in inheriting children

	virtual int								readDTCLinkLossCounter		(void) = 0;  // pure virtual, must define in inheriting children
	virtual void							resetDTCLinkLossCounter		(void) = 0;  // pure virtual, must define in inheriting children

	// ROC debugging functions
	void 									registerFEMacros			(void);
	void        							highRateCheck				(unsigned int loops, unsigned int baseAddress, unsigned int correctRegisterValue0, unsigned int correctRegisterValue1);
	static void 							highRateCheckThread			(ROCCoreVInterface* roc, unsigned int loops, unsigned int baseAddress, unsigned int correctRegisterValue0, unsigned int correctRegisterValue1);

	void        							highRateBlockCheck			(unsigned int loops, unsigned int baseAddress, unsigned int correctRegisterValue0, unsigned int correctRegisterValue1);
	static void 							highRateBlockCheckThread	(ROCCoreVInterface* roc, unsigned int loops, unsigned int baseAddress, unsigned int correctRegisterValue0, unsigned int correctRegisterValue1);

	inline unsigned int 					getLinkID					(void) { return linkID_; }

	bool         									emulatorMode_;
	DTCLib::DTC* 									thisDTC_;

  protected:
	DTCLib::DTC_Link_ID 							linkID_;
	const unsigned int  							delay_;

	//----------------- Emulator members
	// return false when done with workLoop
  public:
	virtual bool 							emulatorWorkLoop			(void)
	{
		__COUT__ << "This is an empty emulator work loop! this function should be overridden "
		          "by the derived class."
		       << __E__;
		//__SS_THROW__;

		return false;
	} // end emulatorWorkLoop()

	static void 							emulatorThread				(ROCCoreVInterface* roc)
	{
		roc->emulatorWorkLoopRunning_ = true;

		bool stillWorking = true;
		while(!roc->emulatorWorkLoopExit_ && stillWorking)
		{		  
		  //__COUT__ << "Calling emulator Work Loop..." << __E__;

			{
				// lockout member variables for the remainder of the scope
				// this guarantees the emulator thread can safely access the members
				//	Note: other functions (e.g. write and read) must also lock for
				// this to work!
				std::lock_guard<std::mutex> lock(roc->workLoopMutex_);
				stillWorking = roc->emulatorWorkLoop();
			}

			
			usleep(roc->emulatorWorkLoopPeriod_ /*microseconds*/);

			
		}
		__COUT__ << "Exited emulator Work Loop." << __E__;

		roc->emulatorWorkLoopRunning_ = false;
	}  // end emulatorThread()

  protected:
	const unsigned int  							emulatorWorkLoopPeriod_; // in microseconds
	volatile bool 									emulatorWorkLoopExit_;
  private:
	volatile bool 									emulatorWorkLoopRunning_;


	std::mutex 										workLoopMutex_;

	//----------------- end Emulator members

	// clang-format on
};  // end ROCCoreVInterface declaration

}  // namespace ots

#endif
