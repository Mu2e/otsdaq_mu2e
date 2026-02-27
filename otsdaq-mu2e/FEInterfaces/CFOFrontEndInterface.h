#ifndef _ots_CFOFrontEndInterface_h_
#define _ots_CFOFrontEndInterface_h_

#include <map>
#include <string>
#include "otsdaq-mu2e/CFOandDTCCore/CFOandDTCCoreVInterface.h"
#include "otsdaq/CoreSupervisors/FESupervisor.h"

namespace ots
{
class CFOFrontEndInterface : public CFOandDTCCoreVInterface
{
  public:
	CFOFrontEndInterface(const std::string&       interfaceUID,
	                     const ConfigurationTree& theXDAQContextConfigTree,
	                     const std::string&       interfaceConfigurationPath);

	virtual ~CFOFrontEndInterface(void);

	// state machine
	//----------------
	void configure(void) override;
	void halt(void) override;
	void pause(void) override;
	void resume(void) override;
	void start(std::string runNumber) override;
	void stop(void) override;
	bool running(void) override;

	// CFO specific items
	//----------------
	float    MeasureLoopback(int linkToLoopback);     ///< pre-covid loopback calculation stretegy
	uint32_t measureDelay(CFOLib::CFO_Link_ID link);  ///< post-covid loopback function

	// int  								getLinkStatus				(void);
	void configureEventBuildingMode(int step = -1);
	void configureLoopbackMode(int step = -1);
	void configureForTimingChain(int step = -1);
	void loopbackTest(std::string runNumber, int step = -1);

	// hardware access
	//----------------
	virtual mu2edev* getDevice(void) override
	{
		if(!thisCFO_)
		{
			__SS__ << "thisCFO_ pointer has not been initialized! " << StringMacros::stackTrace();
			__SS_THROW__;
		}
		return thisCFO_->GetDevice();
	};
	virtual CFOandDTC_Registers* getCFOandDTCRegisters(void) override
	{
		if(!thisCFO_)
		{
			__SS__ << "thisCFO_ pointer has not been initialized! " << StringMacros::stackTrace();
			__SS_THROW__;
		}
		return thisCFO_;
	};

	float delay[8][6][8];
	float delay_rms[8][6][8];
	float delay_failed[8][6][8];

	struct DetachedBufferTestThreadStruct
	{
		std::mutex        lock_;
		std::atomic<bool> running_            = false;
		std::atomic<bool> exitThread_         = false;
		std::atomic<bool> resetStartEventTag_ = false;

		CFOLib::CFO* thisCFO_;

		std::atomic<uint64_t> expectedEventTag_ = -1, nextEventWindowTag_ = -1;
		bool                  saveBinaryData_     = false;
		bool                  doNotResetCounters_ = false;

		std::atomic<uint64_t>                      subeventsCount_;
		std::atomic<uint64_t>                      mismatchedEventTagsCount_;
		std::vector<std::pair<uint64_t, uint64_t>> mismatchedEventTagJumps_;

		uint64_t totalSubeventBytesTransferred_;
		std::chrono::time_point<std::chrono::steady_clock>
		    transferStartTime_, transferEndTime_;

		FILE* fp_ = nullptr;
		bool publish_ = false;
		FESupervisor* feSupervisor_ = nullptr;	

		std::string error_;

	};  // end DetachedBufferTestThreadStruct declaration

	static std::string getDetachedBufferTestStatus(std::shared_ptr<CFOFrontEndInterface::DetachedBufferTestThreadStruct> threadStruct);
	static uint64_t    getDetachedBufferTestReceivedCount(std::shared_ptr<CFOFrontEndInterface::DetachedBufferTestThreadStruct> threadStruct);
	static void        handleDetachedSubevent(const CFOLib::CFO_Event&                                              subevent,
	                                          std::shared_ptr<CFOFrontEndInterface::DetachedBufferTestThreadStruct> threadStruct);

	std::shared_ptr<CFOFrontEndInterface::DetachedBufferTestThreadStruct> bufferTestThreadStruct_;

  private:
	void        initDetachedBufferTest(uint64_t initialEventWindowTag,
	                                   bool     saveBinaryDataToFile,
	                                   bool     saveSubeventHeadersToDataFile,
	                                   bool     doNotResetCounters);
	static void detechedBufferTestThread(std::shared_ptr<CFOFrontEndInterface::DetachedBufferTestThreadStruct> threadStruct);

	void registerFEMacros(void);

	/// -- helper functions for Shared Run Plan ---------
	void     parseEventDurationForRunPlan(const std::string& eventDuration, std::string& durationValue, std::string& durationUnits);
	void     getRatioOfOnPerEvents(uint32_t clocksPerOn, uint32_t clocksPerEvent, uint32_t& mPartRatio, uint32_t& nPartRatio);
	void     mnFixRatio(std::stringstream& logResult, uint32_t& mPartRatio, uint32_t& nPartRatio);
	uint64_t extractSharedRunPlanEventDuration(void);
	void     generateSharedRunPlanWithPeriodicModeOn(std::stringstream& logResult,
	                                                 std::string&       genFilename,
	                                                 const uint64_t     initEventTag,
	                                                 const uint16_t     onBits_startBit,
	                                                 const uint16_t     onBits_bitCount,
	                                                 const uint64_t     onBits_value,
	                                                 uint32_t           mPartRatio,
	                                                 uint32_t           nPartRatio,
	                                                 const std::string& eventDurationSplitNumber,
	                                                 const std::string& eventDurationSplitUnits);
	void     generateSharedRunPlanWithPeriodicModeOff(std::stringstream& logResult,
	                                                  std::string&       genFilename,
	                                                  const uint16_t     onBits_startBit,
	                                                  const uint16_t     onBits_bitCount,
	                                                  const std::string& eventDurationSplitNumber,
	                                                  const std::string& eventDurationSplitUnits);
	/// -- end helper functions for Shared Run Plan ---------

	int                         timing_chain_first_substep_     = -1;
	uint64_t                    next_starting_event_window_tag_ = 0;
	const std::vector<uint32_t> standardNValues_                = {100, 200, uint32_t(1e3)};  //, uint32_t(1e4), uint32_t(1e5), uint32_t(1e6), uint32_t(1e7), uint32_t(1e8), uint32_t(1e9)};
	const std::map<std::string,
	               uint16_t>
	    supportedSubsystems_ = {
	        {"CRV", static_cast<uint16_t>(SharedRunPlanSubsystemModeBit::CRV)},
	        {"Calo", static_cast<uint16_t>(SharedRunPlanSubsystemModeBit::Calo)},
	        {"Tracker", static_cast<uint16_t>(SharedRunPlanSubsystemModeBit::Tracker)},
	        {"STM", static_cast<uint16_t>(SharedRunPlanSubsystemModeBit::STM)},
	        {"ExtMon", static_cast<uint16_t>(SharedRunPlanSubsystemModeBit::ExtMon)},
	        {"Custom", 0}};

  public:
	//=======================
	struct SuperOrchestrationParams
	{
		uint64_t numberOfEventWindows = 10;
		bool     go                   = false;
	};  //end SuperOrchestrationParams struct

	CFOLib::CFO* thisCFO_;

	void GetCounters(__ARGS__);

	void CFOReset(__ARGS__);
	void CFOHalt(__ARGS__);
	void EnableOrDisableClockMarkers(__ARGS__);

	void WriteCFO(__ARGS__);
	void ReadCFO(__ARGS__);

	void                     SuperOrchestrationStart(__ARGS__);
	void                     SuperOrchestrationEnd(__ARGS__);
	void                     SuperOrchestration(__ARGS__);
	void                     SuperOrchestration(bool doCRVReset, bool doCaloReset, bool doCaloWrites);
	SuperOrchestrationParams theSuperParameters_;

	void        ResetRunplan(__ARGS__);
	void        CompileRunplan(__ARGS__);
	void        SetRunplan(__ARGS__);
	std::string SetRunplan(const std::string& binFilename);
	void        LaunchRunplan(__ARGS__);
	void        CompileSetAndLaunchTemplateSuperCycleRunPlan(__ARGS__);
	std::string CompileSetAndLaunchTemplateSuperCycleRunPlan(bool     enable,
	                                                         bool     useDetachedBufferTest,
	                                                         uint32_t numberOfSuperCycles,
	                                                         uint64_t initialEventWindowTag,
	                                                         bool     enableClockMarkers,
	                                                         bool     saveBinaryDataToFile,
	                                                         bool     saveSubeventHeadersToDataFile,
	                                                         bool     doNotResetCounters);
	void        CompileSetAndLaunchTemplateFixedWidthRunPlan(__ARGS__);
	std::string CompileSetAndLaunchTemplateFixedWidthRunPlan(bool enable, bool useDetachedBufferTest, std::string eventDuration, uint32_t numberOfEventWindowMarkers, uint64_t initialEventWindowTag, uint64_t eventWindowMode, bool enableClockMarkers, bool saveBinaryDataToFile, bool saveSubeventHeadersToDataFile, bool doNotResetCounters);

	/// Shared Run Plan related functions and declarations
	enum class SharedRunPlanSubsystemModeBit
	{
		CRV     = 31,
		Calo    = 23,
		Tracker = 15,
		STM     = 37,
		ExtMon  = 39,
		HWDev   = 7
	};
	void SharedRunPlanStatus(__ARGS__);  ///< Get Event Mode, Tag, Active Subsystems, and running status
	void SharedRunPlanStart(__ARGS__);
	void SharedRunPlanStop(__ARGS__);  ///< Halts Run Plan
	void SharedRunPlanSubsystemJoin(__ARGS__);
	void SharedRunPlanSubsystemLeave(__ARGS__);

	void ConfigureForTimingChain(__ARGS__);
	void LoopbackTest(__ARGS__);
	void TestMarker(__ARGS__);
};

}  // namespace ots

#endif
