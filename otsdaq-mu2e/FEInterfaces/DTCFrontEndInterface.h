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

class DTCFrontEndInterface : public CFOandDTCCoreVInterface
{
	// clang-format off
  public:
	DTCFrontEndInterface(const std::string&       interfaceUID,
	                     const ConfigurationTree& theXDAQContextConfigTree,
	                     const std::string&       interfaceConfigurationPath);
	virtual ~DTCFrontEndInterface(void);

	void 								DTCInstantiate();
	
	// specialized ROC handling slow controls
	//----------------
	virtual void 						configureSlowControls		(void) override;
	virtual void						resetSlowControlsChannelIterator (void) override;
	virtual FESlowControlsChannel*		getNextSlowControlsChannel	(void) override;
	virtual unsigned int				getSlowControlsChannelCount	(void) override;

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
	virtual mu2edev* 					getDevice					(void) override {return thisDTC_->GetDevice();};
	virtual CFOandDTC_Registers* 		getCFOandDTCRegisters		(void) override {return thisDTC_;};

	// DTC specific items
	//----------------
	void 								configureHardwareDevMode	(void);
	void 								configureEventBuildingMode	(int step = -1);
	void 								configureLoopbackMode		(int step = -1);
	void 								configureForTimingChain		(int step);

	void								loopbackTest				(int step = -1);

	DTCLib::DTC* 									thisDTC_;

	struct DetachedBufferTestThreadStruct
	{
		std::mutex 				lock_;
		std::atomic<bool>		running_ = false;
		std::atomic<bool>		exitThread_ = false;
		std::atomic<bool>		resetStartEventTag_ = false;

		DTCLib::DTC* 			thisDTC_;

		bool					inSubeventMode_ = false;
		bool					activeMatch_ = false;
		std::atomic<uint64_t>	expectedEventTag_ = -1, nextEventWindowTag_ = -1;		
		bool					saveBinaryData_ = false;
		bool					saveSubeventsToBinaryData_ = false;		
		bool					doNotResetCounters_ = false;
		bool                                    stopOnMismatchedEventTag_ = false; 

		std::atomic<uint64_t>	eventsCount_;
		std::atomic<uint64_t>	subeventsCount_;
		std::atomic<uint64_t>	mismatchedEventTagsCount_;
		std::vector<std::pair<uint64_t, uint64_t>>	mismatchedEventTagJumps_;

		std::vector<uint64_t> 	rocFragmentsCount_, rocFragmentTimeoutsCount_, rocFragmentErrorsCount_, 
			rocPayloadEmptyCount_, rocHeaderTimeoutsCount_, rocPayloadByteCount_;		
		uint64_t				totalSubeventBytesTransferred_;
		std::chrono::time_point<std::chrono::steady_clock>
							transferStartTime_, transferEndTime_;

		FILE*					fp_ = nullptr;

	};  // end DetachedBufferTestThreadStruct declaration

	static std::string 					getDetachedBufferTestStatus			(std::shared_ptr<DTCFrontEndInterface::DetachedBufferTestThreadStruct> threadStruct);
	static uint64_t 					getDetachedBufferTestReceivedCount	(std::shared_ptr<DTCFrontEndInterface::DetachedBufferTestThreadStruct> threadStruct);
	static void 						handleDetachedSubevent				(const DTCLib::DTC_SubEvent& subevent,
																				std::shared_ptr<DTCFrontEndInterface::DetachedBufferTestThreadStruct> threadStruct);

	std::shared_ptr<DTCFrontEndInterface::DetachedBufferTestThreadStruct>	bufferTestThreadStruct_;

  private:
	void 								createROCs							(void);
	void 								registerFEMacros					(void);

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


	static void detechedBufferTestThread(
		std::shared_ptr<DTCFrontEndInterface::DetachedBufferTestThreadStruct> threadStruct);

  public:

	void								SetupROCs							(__ARGS__);
	std::string							SetupROCs							(DTCLib::DTC_Link_ID rocLinkIndex,
																			bool rocRxTxEnable, bool rocTimingEnable, bool rocEmulationEnable,
																			DTCLib::DTC_ROC_Emulation_Type rocEmulationType, uint32_t size);
	void 								ReadROC								(__ARGS__);
	void 								WriteROC							(__ARGS__);
	void 								BockReadROC							(__ARGS__);
	void 								BockWriteROC						(__ARGS__);
    void 								WriteExternalROCRegister			(__ARGS__);
	void                             	ReadExternalROCRegister        		(__ARGS__);
	void 								DTCHighRateBlockCheck				(__ARGS__);
	
	void 								DTCHighRateDCSCheck					(__ARGS__);
	void 								RunROCFEMacro						(__ARGS__);
	void 								DTCSendHeartbeatAndDataRequest		(__ARGS__);
	void								ResetLossOfLockCounter				(__ARGS__);
	void								ReadLossOfLockCounter				(__ARGS__);
	void								GetLinkLockStatus					(__ARGS__);	
	void								SelectJitterAttenuatorSource		(__ARGS__);
	void								WriteDTC							(__ARGS__);
	void								ReadDTC								(__ARGS__);

	void								configureHardwareDevMode			(__ARGS__);
	void								ConfigureForTimingChain				(__ARGS__);

	void 								DTCCounters							(__ARGS__);
	void 								readRxDiagFIFO						(__ARGS__);
	void 								readTxDiagFIFO						(__ARGS__);
	void 								GetLinkErrors						(__ARGS__);
	void 								ROCResetLink						(__ARGS__);
	void								HeaderFormatTest					(__ARGS__);

	void 								DTCInstantiate						(__ARGS__);
	void 								ResetDTCLinks						(__ARGS__);

	void 								ResetPCIe							(__ARGS__);
	void 								ResetCFOLinkRx						(__ARGS__);
	void 								ResetCFOLinkTx						(__ARGS__);
	void 								ResetCFOLinkRxPLL					(__ARGS__);
	void 								ResetCFOLinkTxPLL					(__ARGS__);
	
	void 								SetupCFOInterface					(__ARGS__);
	void 								SetCFOEmulatorOnOffSpillEmulation	(__ARGS__);
	std::string							SetCFOEmulatorOnOffSpillEmulation	(bool enable,
																			bool useDetachedBufferTest, uint32_t numberOfSuperCycles, uint64_t initialEventWindowTag,
																			bool enableClockMarkers, bool enableAutogenDRP, bool saveBinaryDataToFile, bool saveSubeventHeadersToDataFile,
																			bool doNotResetCounters);
	void 								SetCFOEmulatorFixedWidthEmulation	(__ARGS__);
	std::string							SetCFOEmulatorFixedWidthEmulation	(bool enable, bool useDetachedBufferTest,
																			std::string eventDuration, uint32_t numberOfEventWindowMarkers, uint64_t initialEventWindowTag,
																			uint64_t eventWindowMode, bool enableClockMarkers, bool enableAutogenDRP, bool saveBinaryDataToFile,
																			bool saveSubeventHeadersToDataFile,	bool doNotResetCounters);

	void 								BufferTest							(__ARGS__);
	void 								PatternTest							(__ARGS__);
	void 								BufferTest_detached					(__ARGS__);


    void                                SoftwareDataRequest                 (__ARGS__);
    void                                PunchedClock                        (__ARGS__);

	void 								CFOEmulatorLoopbackTest				(__ARGS__);
	void 								ManualLoopbackSetup					(__ARGS__);

	
	// clang-format on
};
}  // namespace ots
#endif
