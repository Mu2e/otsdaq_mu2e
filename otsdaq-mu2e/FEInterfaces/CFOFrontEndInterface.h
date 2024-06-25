#ifndef _ots_CFOFrontEndInterface_h_
#define _ots_CFOFrontEndInterface_h_

#include <map>
#include <string>
#include "otsdaq-mu2e/CFOandDTCCore/CFOandDTCCoreVInterface.h"

#include "cfoInterfaceLib/CFO.h"
#include "cfoInterfaceLib/CFO_Compiler.hh"

namespace ots
{
class CFOFrontEndInterface : public CFOandDTCCoreVInterface
{
	// clang-format off

  public:
	CFOFrontEndInterface(const std::string&       interfaceUID,
	                     const ConfigurationTree& theXDAQContextConfigTree,
	                     const std::string&       interfaceConfigurationPath);

	virtual ~CFOFrontEndInterface(void);

	// state machine
	//----------------
	void 								configure					(void) override;
	void 								halt						(void) override;
	void 								pause						(void) override;
	void 								resume						(void) override;
	void 								start						(std::string runNumber) override;
	void 								stop						(void) override;
	bool 								running						(void) override;

	// CFO specific items
	//----------------
	float 								MeasureLoopback				(int linkToLoopback); //pre-covid loopback calculation stretegy
	uint32_t 							measureDelay				(CFOLib::CFO_Link_ID link); //post-covid loopback function

	// int  								getLinkStatus				(void);
	void 								configureEventBuildingMode	(int step = -1);
	void 								configureLoopbackMode		(int step = -1);
	void 								configureForTimingChain		(int step = -1);
	void								loopbackTest				(std::string runNumber, int step = -1);


	// hardware access
	//----------------
	virtual mu2edev* 					getDevice					(void) override {return thisCFO_->GetDevice();};
	virtual CFOandDTC_Registers* 		getCFOandDTCRegisters		(void) override {return thisCFO_;};

	float 								delay[8][6][8];
	float 								delay_rms[8][6][8];
	float 								delay_failed[8][6][8];

	struct DetachedBufferTestThreadStruct
	{
		std::mutex 				lock_;
		std::atomic<bool>		running_ = false;
		std::atomic<bool>		exitThread_ = false;
		std::atomic<bool>		resetStartEventTag_ = false;

		CFOLib::CFO* 			thisCFO_;

		std::atomic<uint64_t>	expectedEventTag_ = -1, nextEventWindowTag_ = -1;		
		bool					saveBinaryData_ = false;		
		bool					doNotResetCounters_ = false;

		std::atomic<uint64_t>	subeventsCount_;
		std::atomic<uint64_t>	mismatchedEventTagsCount_;
		std::vector<std::pair<uint64_t, uint64_t>>	mismatchedEventTagJumps_;
	
		uint64_t				totalSubeventBytesTransferred_;
		std::chrono::time_point<std::chrono::steady_clock>
							transferStartTime_, transferEndTime_;

		FILE*					fp_ = nullptr;

		std::string				error_;
		
	};  // end DetachedBufferTestThreadStruct declaration

	static std::string 					getDetachedBufferTestStatus			(std::shared_ptr<CFOFrontEndInterface::DetachedBufferTestThreadStruct> threadStruct);
	static uint64_t 					getDetachedBufferTestReceivedCount	(std::shared_ptr<CFOFrontEndInterface::DetachedBufferTestThreadStruct> threadStruct);
	static void 						handleDetachedSubevent				(const CFOLib::CFO_Event& subevent,
																				std::shared_ptr<CFOFrontEndInterface::DetachedBufferTestThreadStruct> threadStruct);
	
	std::shared_ptr<CFOFrontEndInterface::DetachedBufferTestThreadStruct>	bufferTestThreadStruct_;

  	

  private:

	void initDetachedBufferTest(
		uint64_t initialEventWindowTag,
		bool saveBinaryDataToFile,
		bool saveSubeventHeadersToDataFile, bool doNotResetCounters);
	static void detechedBufferTestThread(
		std::shared_ptr<CFOFrontEndInterface::DetachedBufferTestThreadStruct> threadStruct);

	
	void 								registerFEMacros			(void);
	
	// CFOLib::CFO_Registers* 				thisCFO_;
	int									timing_chain_first_substep_	   = -1;
	//int                    configure_clock_ = 0;

	// float        						average_loopback_;

  public:

	CFOLib::CFO* 						thisCFO_;

	// void 								FlashLEDs						(__ARGS__);	
	// void 								GetFirmwareVersion				(__ARGS__);
	// void 								GetStatus						(__ARGS__);
	void 								GetCounters						(__ARGS__);
	// void 								GetFPGATemperature				(__ARGS__);
	// void								SelectJitterAttenuatorSource	(__ARGS__);


	void 								CFOReset						(__ARGS__);
	void 								CFOHalt							(__ARGS__);
	
	void 								WriteCFO						(__ARGS__);
	void 								ReadCFO							(__ARGS__);
	void 								ResetRunplan					(__ARGS__);
	void 								CompileRunplan					(__ARGS__);
	void 								SetRunplan						(__ARGS__);
	std::string							SetRunplan						(const std::string& binFilename);
	void 								LaunchRunplan					(__ARGS__);
	void 								CompileSetAndLaunchTemplateSuperCycleRunPlan	(__ARGS__);
	std::string							CompileSetAndLaunchTemplateSuperCycleRunPlan	(bool enable,
																						bool useDetachedBufferTest, uint32_t numberOfSuperCycles, uint64_t initialEventWindowTag,
																						bool enableClockMarkers, bool saveBinaryDataToFile, bool saveSubeventHeadersToDataFile,
																						bool doNotResetCounters);
	void 								CompileSetAndLaunchTemplateFixedWidthRunPlan	(__ARGS__);
	std::string							CompileSetAndLaunchTemplateFixedWidthRunPlan	(bool enable, bool useDetachedBufferTest,
																						std::string eventDuration, uint32_t numberOfEventWindowMarkers, uint64_t initialEventWindowTag,
																						uint64_t eventWindowMode, bool enableClockMarkers, bool saveBinaryDataToFile,
																						bool saveSubeventHeadersToDataFile,	bool doNotResetCounters);
	void 								ConfigureForTimingChain			(__ARGS__);
	void								LoopbackTest					(__ARGS__);
	void 								TestMarker						(__ARGS__);
};

// clang-format on
}  // namespace ots

#endif
