#include "otsdaq-mu2e/FEInterfaces/DTCFrontEndInterface.h"
#include "otsdaq/FECore/MakeInterface.h"
#include "otsdaq/Macros/BinaryStringMacros.h"
// #include "otsdaq/Macros/InterfacePluginMacros.h"

// #include <fstream>

// ROOT includes
#include "TFile.h"
#include "TGraph.h"
#include "TH1.h"

#include <thread>

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "DTCFrontEndInterface"

#undef LOCAL_COUT_HDR
#define LOCAL_COUT_HDR                                                              \
	(threadStruct->thisDTC_                                                         \
	     ? ("FE:"                                                                   \
	        "DTCFrontEndInterface" +                                                \
	        std::string(":") + threadStruct->thisDTC_->getDeviceUID() + ":dev" +    \
	        std::to_string(threadStruct->thisDTC_->GetDevice()->getDeviceIndex()) + \
	        "\t<> ")                                                                \
	     : "")

// // some global variables, probably a bad idea. But temporary
// std::string RunDataFN = "";
// std::fstream runDataFile_;
// int FEWriteFile = 0;
// bool artdaqMode_ = true;

//=========================================================================================
DTCFrontEndInterface::DTCFrontEndInterface(
    const std::string&       interfaceUID,
    const ConfigurationTree& theXDAQContextConfigTree,
    const std::string&       interfaceConfigurationPath)
    : CFOandDTCCoreVInterface(
          interfaceUID, theXDAQContextConfigTree, interfaceConfigurationPath)
    , thisDTC_(0)
    , EmulatedCFO_(0)
{
	__FE_COUT__ << "instantiate DTC... " << getInterfaceUID() << " "
	            << theXDAQContextConfigTree << " " << interfaceConfigurationPath << __E__;

	if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_HARDWARE_DEV)
	{
		__FE_COUT_INFO__
		    << "Hardware Dev Mode identified, so forcing CFO emulation mode for DTC"
		    << __E__;
		emulate_cfo_ = true;
	}
	else
		emulate_cfo_ = getSelfNode().getNode("EmulateCFO").getValue<bool>();
	__FE_COUTV__(emulate_cfo_);

	DTCInstantiate();

}  // end constructor()

//==========================================================================================
DTCFrontEndInterface::~DTCFrontEndInterface(void)
{
	try
	{
		// Halt Detached Buffer Thread else if(command == "2" || command == "Halt")
		if(bufferTestThreadStruct_)
		{
			__FE_COUT__ << "Attempting to halt Buffer Test thread... " << __E__;

			// start mutex scope
			{
				std::lock_guard<std::mutex> lock(bufferTestThreadStruct_->lock_);
				bufferTestThreadStruct_->exitThread_ = true;
			}

			//check for thread to exit
			for(int i = 0; i < 10; ++i)
			{
				usleep(100 * 1000 /*100ms*/);  // sleep for exit time
				if(!bufferTestThreadStruct_->running_)
					break;
				__FE_COUT__ << "Waiting for thread to exit... #" << i << __E__;
			}

			if(bufferTestThreadStruct_->fp_)
			{
				__FE_COUT_WARN__ << "Buffer Test thread file was left open?! Closing..."
				                 << __E__;

				fclose(bufferTestThreadStruct_->fp_);
				bufferTestThreadStruct_->fp_ = nullptr;
			}

			__FE_COUT__ << "Detached Buffer Test thread exited. " << __E__;
			__FE_COUT__ << "Reading final status..." << __E__;
			__FE_COUT__ << DTCFrontEndInterface::getDetachedBufferTestStatus(
			    bufferTestThreadStruct_);
		}

		if(thisDTC_)
		{
			// uint32_t lossOfLockReadData = registerRead(0x93c8);	// read loss-of-lock counter
			__FE_COUTV__(thisDTC_->FormatRXCDRUnlockCountCFOLink());
		}

		// destroy ROCs before DTC destruction
		rocs_.clear();
	}
	catch(...)
	{
		__FE_COUT_WARN__ << "Exception caught on destruction." << __E__;
	}

	if(thisDTC_)
		delete thisDTC_;

	__FE_COUT__ << "Destructed." << __E__;
}  // end destructor()

//==============================================================================
void DTCFrontEndInterface::setParentPointers(CoreSupervisorBase*   supervisor,
                                             FEVInterfacesManager* manager)
{
	FEVInterface::setParentPointers(supervisor, manager);

	for(auto& roc : rocs_)
		roc.second->setParentPointers(supervisor, manager);
}  // end setParentPointers()

//==============================================================================
void DTCFrontEndInterface::registerFEMacros(void)
{
	__FE_COUT__ << "Registering DTC FE Macros..." << __E__;

	mapOfFEMacroFunctions_.clear();

	registerFEMacroFunction(
	    "ROC Setup",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::SetupROCs),
	    std::vector<std::string>{
	        "Target ROC or Mask (Default = -1 := all ROCs, or 0x111111 := all)",
	        "Set Link RX/TX Enable (Default := false)",
	        "Set Link Timing Enable (Default := false)",
	        "Set ROC Emulation Enable (Default := false)",
	        "ROC Emulation Type (Default = 0: Internal, 1: Fiber-Loopback, 2: External)",
	        "ROC generated Data Payload fragment packet count (11-bits, Default := 16)",
	        "Block Null Heartbeats to ALL ROCs (Default := false)",
	        "Resequence Non-null Events for ALL ROCs (Default := false)",
	    },
	    std::vector<std::string>{"Result"},
	    1,  // requiredUserPermissions
	    "*",
	    "Use this to control the parameters of the ROC Emulation of the DTC. "
	    "Internal ROC emulation does not require any fibers; the ROC is emulated without "
	    "using the link SERDES rx/tx. "
	    "Fiber-Loopback ROC emulation requires a single fiber connected between the link "
	    "SERDES rx <==> tx channels. "
	    "External ROC emulation uses the link SERDES rx/tx as though it is a ROC; this "
	    "means the rx/tx channels could be connected to another DTC, and the other DTC "
	    "would communicate to this emulated ROC following the ROC protocol of docdb-4914."
	    "\n\n"
	    "Note: Internal and External ROC emulation can co-exist on the same DTC link. "
	    "While Fiber-Loopback ROC emulation is exclusive to the other two types and will "
	    "take precedence.");

	registerFEMacroFunction(
	    "ROC Write",  // feMacroName
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::WriteROC),  // feMacroFunction
	    std::vector<std::string>{
	        "Target ROC or Mask (Default = -1 := all ROCs, or 0x111111 := all)",
	        "address",
	        "writeData"},
	    std::vector<std::string>{"Result"},  // namesOfOutput
	    1,                                   // requiredUserPermissions
	    "*",                                 // allowedCallingFEs
	    "This FE Macro writes data to a specific register on a specified link.");

	registerFEMacroFunction(
	    "ROC Read",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::ReadROC),  // feMacroFunction
	    std::vector<std::string>{
	        "Target ROC or Mask (Default = -1 := all ROCs, or 0x111111 := all)",
	        "address"},  // namesOfInputArgs
	    std::vector<std::string>{"readData"},
	    1,  // requiredUserPermissions
	    "*",
	    "This FE Macro reads data from a ROC given a link and address.");

	registerFEMacroFunction(
	    "ROC Block Read",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::BlockReadROC),
	    std::vector<std::string>{
	        "Target ROC or Mask (Default = -1 := all ROCs, or 0x111111 := all)",
	        "address",
	        "Number Of 16-bit words to Read (Default := 8)",
	        "incrementAddress (Default := false)"},
	    std::vector<std::string>{"readData"},
	    1,  // requiredUserPermissions
	    "*",
	    "This FE Macro is used to read multiple words from a ROC.");

	registerFEMacroFunction(
	    "ROC Block Write",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::BlockWriteROC),
	    std::vector<std::string>{
	        "Target ROC or Mask (Default = -1 := all ROCs, or 0x111111 := all)",
	        "address",
	        "writeData (CSV-literal or CSV-filename of 16-bit words, or keyword + "
	        "parameter 'AUTOGENERATE count')",
	        "incrementAddress (Default = false)",
	        "requestAck (Default = false)"},
	    std::vector<std::string>{"Status"},
	    1,  // requiredUserPermissions
	    "*",
	    "This FE Macro enables users to write multiple words in the format of a comma "
	    "separated values or "
	    "a CSV file.");

	registerFEMacroFunction("Headers Format test",
	                        static_cast<FEVInterface::frontEndMacroFunction_t>(
	                            &DTCFrontEndInterface::HeaderFormatTest),
	                        std::vector<std::string>{},
	                        std::vector<std::string>{"setRegister"},
	                        1,
	                        "*",
	                        "Use this FE Macro to test the header format using emulated "
	                        "CFO Heartbeat packets.");

	if(1)
	{
		registerFEMacroFunction(
		    "ROC_Write_ExtRegister",  // feMacroName
		    static_cast<FEVInterface::frontEndMacroFunction_t>(
		        &DTCFrontEndInterface::WriteExternalROCRegister),  // feMacroFunction
		    std::vector<std::string>{
		        "Target ROC or Mask (Default = -1 := all ROCs, or 0x111111 := all)",
		        "block",
		        "address",
		        "writeData"},
		    std::vector<std::string>{"Result"},  // namesOfOutputArgs
		    1);                                  // requiredUserPermissions

		registerFEMacroFunction(
		    "ROC_Read_ExtRegister",
		    static_cast<FEVInterface::frontEndMacroFunction_t>(
		        &DTCFrontEndInterface::ReadExternalROCRegister),
		    std::vector<std::string>{
		        "Target ROC or Mask (Default = -1 := all ROCs, or 0x111111 := all)",
		        "block",
		        "address"},
		    std::vector<std::string>{"readData"},
		    1);  // requiredUserPermissions
	}

	// Until further subsystem ROC development starts up, ignore the external block register access of core ROC firmware template established for the ROC dev cards.
	if(0)  // unregistering of "temporarily" unused macros
	{
		registerFEMacroFunction(
		    "Buffer Test",
		    static_cast<FEVInterface::frontEndMacroFunction_t>(
		        &DTCFrontEndInterface::BufferTest),  // feMacroFunction
		    std::vector<std::string>{
		        "Data are SubEvents (Default: true)",
		        "Number of [Sub]Events (Default: 1)",
		        "Starting Event Window Tag (Default: 0)",
		        "Match Event Tags (Default: false)",
		        "Display Payload at GUI (Default: true)",
		        // "eventDuration (Default := 400)",
		        // "doNotReadBack (bool)",
		        "Save Binary Data to File (Default: false)"
		        // "Software Generated Data Requests (bool)",
		        // "Do Not Send Heartbeats (bool)"
		    },
		    std::vector<std::string>{"Result"},
		    1,  // requiredUserPermissions
		    "*",
		    "Read a specified number of events from the Data DMA channel-0, and attempt "
		    "to validate data."
		    // "Send a request for a number of events and waits for the respective responses. "
		    // "Currently, the responses are simulated data (a counter)."
		);

		registerFEMacroFunction(
		    "Pattern Test",
		    static_cast<FEVInterface::frontEndMacroFunction_t>(
		        &DTCFrontEndInterface::PatternTest),  // feMacroFunction
		    std::vector<std::string>{
		        "Data are SubEvents (Default: true)",
		        "Number of [Sub]Events (Default: 1)",
		        "Starting Event Window Tag (Default: 0)",
		        "Match Event Tags (Default: false)",
		        "Display Payload at GUI (Default: true)",
		        // "eventDuration (Default := 400)",
		        // "doNotReadBack (bool)",
		        "Save Binary Data to File (Default: false)"
		        // "Software Generated Data Requests (bool)",
		        // "Do Not Send Heartbeats (bool)"
		    },
		    std::vector<std::string>{"Result"},
		    1,  // requiredUserPermissions
		    "*",
		    "Read a specified number of events from the Data DMA channel-0, and attempt "
		    "to validate data."
		    // "Send a request for a number of events and waits for the respective responses. "
		    // "Currently, the responses are simulated data (a counter)."
		);

		registerFEMacroFunction(
		    "DTC_HighRate_DCS_Check",
		    static_cast<FEVInterface::frontEndMacroFunction_t>(
		        &DTCFrontEndInterface::DTCHighRateDCSCheck),
		    std::vector<std::string>{
		        "Target ROC or Mask (Default = -1 := all ROCs, or 0x111111 := all)",
		        "loops",
		        "baseAddress",
		        "correctRegisterValue0",
		        "correctRegisterValue1"},
		    std::vector<std::string>{},
		    1);  // requiredUserPermissions

		registerFEMacroFunction(
		    "DTC_HighRate_DCS_Block_Check",
		    static_cast<FEVInterface::frontEndMacroFunction_t>(
		        &DTCFrontEndInterface::DTCHighRateBlockCheck),
		    std::vector<std::string>{
		        "Target ROC or Mask (Default = -1 := all ROCs, or 0x111111 := all)",
		        "loops",
		        "baseAddress",
		        "correctRegisterValue0",
		        "correctRegisterValue1"},
		    std::vector<std::string>{},
		    1);  // requiredUserPermissions

		registerFEMacroFunction(
		    "DTC_SendHeartbeatAndDataRequest",
		    static_cast<FEVInterface::frontEndMacroFunction_t>(
		        &DTCFrontEndInterface::DTCSendHeartbeatAndDataRequest),
		    std::vector<std::string>{
		        "numberOfRequests", "timestampStart", "useSWCFOEmulator", "rocMask"},
		    std::vector<std::string>{"readData"},
		    1);  // requiredUserPermissions

		registerFEMacroFunction("DTC Instantiate",
		                        static_cast<FEVInterface::frontEndMacroFunction_t>(
		                            &DTCFrontEndInterface::DTCInstantiate),
		                        std::vector<std::string>{},
		                        std::vector<std::string>{},
		                        1,  // requiredUserPermissions
		                        "*",
		                        "This FE Macro reinstantiates the DTC interface class.");

	}  // end unregistering of "temporarily" unused macros

	registerFEMacroFunction(
	    "Buffer Test Detached",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::BufferTest_detached),  // feMacroFunction
	    std::vector<std::string>{
	        "Command to 0/Status (to read counters, etc.), 1/Start, or 2/Halt (Default: "
	        "Status)",
	        "Data are SubEvents (Default: true)",
	        // "Number of [Sub]Events (Default: 1)",  // will be continuous!
	        "Starting Event Window Tag (Default: 0)",
	        "Match Event Tags (Default: false)",
	        // "Display Payload at GUI (Default: true)", // will be summary output
	        // "eventDuration (Default := 400)",
	        // "doNotReadBack (bool)",
	        "Save Binary Data to File (Default: false)",
	        "Save Binary Data Filename",
	        "Save Subevent Header to Binary File (Default: false)",
	        "Payload Packet Threshold for Saving Event (Default: 0)"
	        // "Software Generated Data Requests (bool)",
	        // "Do Not Send Heartbeats (bool)"
	    },
	    std::vector<std::string>{"Result"},
	    1,  // requiredUserPermissions
	    "*",
	    "Read a specified number of events from the Data DMA channel-0, and attempt to "
	    "validate data."
	    // "Send a request for a number of events and waits for the respective responses. "
	    // "Currently, the responses are simulated data (a counter)."
	);

	registerFEMacroFunction(
	    "DTC Write",  // feMacroName
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::WriteDTC),  // feMacroFunction
	    std::vector<std::string>{
	        "address", "writeData", "Do validation (Default := true)"},
	    std::vector<std::string>{"Status"},  // namesOfOutput
	    1,                                   // requiredUserPermissions
	    "*",
	    "This FE Macro writes to the DTC registers.");

	registerFEMacroFunction("DTC Read",
	                        static_cast<FEVInterface::frontEndMacroFunction_t>(
	                            &DTCFrontEndInterface::ReadDTC),  // feMacroFunction
	                        std::vector<std::string>{"address"},  // namesOfInputArgs
	                        std::vector<std::string>{"readData"},
	                        1,  // requiredUserPermissions
	                        "*",
	                        "Read from the DTC Memory Map.");

	registerFEMacroFunction(
	    "Event Mode Required Mask Set",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::SetCFOEventModeRequiredMask),
	    std::vector<std::string>{"Event Mode Required Mask (Default := 0)"},
	    std::vector<std::string>{"Result"},
	    1,
	    "*",
	    "Set the Event Mode Required Mask used for Event Mode filtering. "
	    "A mask bit of 1 requires the corresponding Event Mode bit to also be 1.");

	registerFEMacroFunction("Event Mode Required Mask Read",
	                        static_cast<FEVInterface::frontEndMacroFunction_t>(
	                            &DTCFrontEndInterface::ReadCFOEventModeRequiredMask),
	                        std::vector<std::string>{},
	                        std::vector<std::string>{"Event Mode Required Mask"},
	                        1,
	                        "*",
	                        "Readback the current DTC Event Mode Required Mask used for "
	                        "CFO Event Mode filtering.");

	registerFEMacroFunction("Loss-of-Lock Counter Read",
	                        static_cast<FEVInterface::frontEndMacroFunction_t>(
	                            &DTCFrontEndInterface::ReadLossOfLockCounter),
	                        std::vector<std::string>{},
	                        std::vector<std::string>{"Upstream Rx Lock Loss Count"},
	                        1,
	                        "*",
	                        "Displays the number of times the CFO Control Link lost CDR "
	                        "lock since the last reset. "
	                        "Use the FE Macro <b>Reset Loss-of-Lock Counter</b> to reset "
	                        "the register counter to zero.");

	registerFEMacroFunction(
	    "Loss-of-Lock Counter Reset",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::ResetLossOfLockCounter),
	    std::vector<std::string>{},
	    std::vector<std::string>{"Upstream Rx Lock Loss Count"},
	    1,  // requiredUserPermissions
	    "*",
	    "Use this FE Macro to reset the Loss-of-Lock register counter to zero. Resetting "
	    "the DTC will also reset this register.");

	registerFEMacroFunction(
	    "Spy Buffer",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::SpyBuffer),
	    std::vector<std::string>{},
	    std::vector<std::string>{"Result"},
	    1,  // requiredUserPermissions
	    "*",
	    "Dump the current DAQ DMA C2S buffers to the log (mu2edev::spy with "
	    "once|wide|stacktrace flags), then issue a DTC SoftReset. Useful for "
	    "diagnosing parser desync — captures whatever is sitting in the DMA "
	    "buffers right now and resets the DTC so subsequent reads start clean.");

	registerFEMacroFunction(
	    "Release All Data DMA Buffers",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::ReleaseAllDAQBuffers),
	    std::vector<std::string>{},
	    std::vector<std::string>{"Result"},
	    1,  // requiredUserPermissions
	    "*",
	    "Release all currently-held DAQ DMA buffers back to the driver "
	    "(getDTC()->ReleaseAllBuffers(DTC_DMA_Engine_DAQ)).");

	registerFEMacroFunction(
	    "Get DTC Counters",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::DTCCounters),
	    std::vector<std::string>{},
	    std::vector<std::string>{"Protocol Counters", "Performance Counters"},
	    1,  // requiredUserPermissions
	    "*",
	    "Fetches data from all the counter registers in a human-readable format. "
	    "Counters include the number of bytes and packets transmitted and received over "
	    "ROC/CFO links since the last reset. "
	    "Also includes Event Builder, Jitter Attenuator, Emulated ROC delay, Heartbeat "
	    "packet and Data Header packet counters since last reset. ");

	registerFEMacroFunction("Get Link Lock Status",
	                        static_cast<FEVInterface::frontEndMacroFunction_t>(
	                            &DTCFrontEndInterface::GetLinkLockStatus),
	                        std::vector<std::string>{},
	                        std::vector<std::string>{"Lock Status"},
	                        1,  // requiredUserPermissions
	                        "*",
	                        "Read the SERDES CDR Lock bit on all links.");

	registerFEMacroFunction(
	    "Configure DTC for HWDevmode",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::configureHardwareDevMode),
	    std::vector<std::string>{},
	    std::vector<std::string>{
	        "Setting the CFO emulated, DCS enabled and the retransmission off"},
	    1,  // requiredUserPermissions
	    "*",
	    "This FE Macro prepares the DTC for HW Dev Mode (emulated CFO).");

	registerFEMacroFunction(
	    "Read Rx Diag FIFO",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::readRxDiagFIFO),
	    std::vector<std::string>{"LinkIndex"},
	    std::vector<std::string>{"Diagnostic RX FIFO"},
	    1,  // requiredUserPermissions
	    "*",
	    "This FE Macro reads the ROC link RX diagnostic FIFO buffer from the SERDES. "
	    "When empty, the FIFO reports 0XDEADDEAD. <b>Note</b>: the FIFO must be read at "
	    "least once before valid data appears. "
	    "Reading the FIFO pulses the Read Enable input.");

	registerFEMacroFunction(
	    "Read Tx Diag FIFO",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::readTxDiagFIFO),
	    std::vector<std::string>{"LinkIndex"},
	    std::vector<std::string>{"Diagnostic TX FIFO"},
	    1,  // requiredUserPermissions
	    "*",
	    "This FE Macro reads the ROC link TX diagnostic FIFO buffer from the SERDES. "
	    "When empty, the FIFO reports 0XDEADDEAD. <b>Note</b>: the FIFO must be read at "
	    "least once before valid data appears. "
	    "Reading the FIFO pulses the Read Enable input.");

	registerFEMacroFunction(
	    "Get Link Errors",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::GetLinkErrors),
	    std::vector<std::string>{""},
	    std::vector<std::string>{"Link Errors"},
	    1,  // requiredUserPermissions
	    "*",
	    "This FE Macro returns the number of errors on all links since last reset. "
	    "Errors include the number of times illegal characters, a disparity, PRBS, and "
	    "CRC error the SERDES has received. "
	    "It also includes the number of EVB RX packet errors, and number of times the "
	    "Jitter Attenuator lost the RX Recovered clock and "
	    "lost the RX External clock since last reset.");

	std::stringstream feMacroTooltip;
	feMacroTooltip << "There are " << CONFIG_DTC_TIMING_CHAIN_STEPS
	               << " steps. So choose 1 step at a time, 0-"
	               << CONFIG_DTC_TIMING_CHAIN_STEPS - 1
	               << " or use -1 to run all steps sequentially." << __E__;

	registerFEMacroFunction("Configure for Timing Chain",
	                        static_cast<FEVInterface::frontEndMacroFunction_t>(
	                            &DTCFrontEndInterface::ConfigureForTimingChain),
	                        std::vector<std::string>{"StepIndex"},
	                        std::vector<std::string>{},
	                        1,
	                        "*" /*allowedCallingFEs*/,
	                        feMacroTooltip.str());  // requiredUserPermissions

	registerFEMacroFunction("Loopback CFO Emulator Test Run",
	                        static_cast<FEVInterface::frontEndMacroFunction_t>(
	                            &DTCFrontEndInterface::CFOEmulatorLoopbackTest),
	                        std::vector<std::string>{},
	                        std::vector<std::string>{"Result"},
	                        1,  // requiredUserPermissions
	                        "*",
	                        "Executes a loopback test at the DTC's CFO emulator, and "
	                        "broadcasts loopback markers to all ROCs.");

	registerFEMacroFunction(
	    "Loopback CFO Emulator Multi Test Run",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::CFOEmulatorLoopbackTests),
	    std::vector<std::string>{"numberOfTests",
	                             "Write ROOT file (Default := false)",
	                             "ROOT file name (Default := loopback.root)"},
	    std::vector<std::string>{"Average", "Maximum", "Minimum"},
	    1,  // requiredUserPermissions
	    "*",
	    "Executes many loopback tests at the DTC's CFO emulator, and broadcasts loopback "
	    "markers to all ROCs, and returns the average result.");

	registerFEMacroFunction(
	    "Loopback Manual Setup",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::ManualLoopbackSetup),
	    std::vector<std::string>{"setAsPassthrough", "ROC_Link"},
	    std::vector<std::string>{},
	    1,  // requiredUserPermissions
	    "*",
	    "Sets the DTC in loopback mode. This is accomplished by disabling all links "
	    "except for <b>ROC_Link</b>. "
	    "If <b>setAsPassThrough</b> is enabled, CFO markers will be transmitted to the "
	    "next DTC (Normal operation). "
	    "If <b>setAsPassThrough</b> is disabled, the CFO Link SERDES output is routed "
	    "back to the source. "
	    "The loopback functionality is managed through the DTC Control Register bit 28.");

	registerFEMacroFunction(
	    "Enable/Disable DTC Link",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::EnableDTCLink),
	    std::vector<std::string>{"Target Link (Default = -1 := all links)",
	                             "Set Link RX/TX Enable (Default := false)"},
	    std::vector<std::string>{"Result"},
	    1,  // requiredUserPermissions
	    "*",
	    "This FE Macro enables/disables a target DTC Link 0-7 (i.e., 0-5 ROCs, 6 CFO, 7 "
	    "EVB).");

	registerFEMacroFunction(
	    "Reset ALL (CFO/ROC/EVB) DTC Links",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::ResetDTCLinks),
	    std::vector<std::string>{},
	    std::vector<std::string>{},
	    1,  // requiredUserPermissions
	    "*",
	    "This FE Macro resets the SERDES TX/RX links and then the SERDES.");

	//------------------

	registerFEMacroFunction(
	    "Reset CFO Link Rx",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::ResetCFOLinkRx),  // feMacroFunction
	    std::vector<std::string>{},                  // namesOfInputArgs
	    std::vector<std::string>{},
	    1,  // requiredUserPermissions
	    "*",
	    "Reset the CFO SERDES RX interface.");
	registerFEMacroFunction(
	    "Reset CFO Link Tx",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::ResetCFOLinkTx),  // feMacroFunction
	    std::vector<std::string>{},                  // namesOfInputArgs
	    std::vector<std::string>{},
	    1,  // requiredUserPermissions
	    "*",
	    "Reset the CFO SERDES TX interface.");
	registerFEMacroFunction(
	    "Reset CFO Link Rx PLL",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::ResetCFOLinkRxPLL),  // feMacroFunction
	    std::vector<std::string>{},                     // namesOfInputArgs
	    std::vector<std::string>{},
	    1,  // requiredUserPermissions
	    "*",
	    "Reset the CFO SERDES RX PLL.");
	registerFEMacroFunction(
	    "Reset CFO Link Tx PLL",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::ResetCFOLinkTxPLL),  // feMacroFunction
	    std::vector<std::string>{},                     // namesOfInputArgs
	    std::vector<std::string>{},
	    1,  // requiredUserPermissions
	    "*",
	    "Reset the CFO SERDES TX PLL.");

	//------------------

	// registerFEMacroFunction(
	// 	"Reset EVB Link Rx",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&DTCFrontEndInterface::ResetEVBLinkRx),            // feMacroFunction
	// 				std::vector<std::string>{},  // namesOfInputArgs
	// 				std::vector<std::string>{},
	// 				1,  // requiredUserPermissions
	// 				"*",
	// 				"Reset the EVB SERDES RX interface."
	// );
	// registerFEMacroFunction(
	// 	"Reset EVB Link Tx",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&DTCFrontEndInterface::ResetEVBLinkTx),            // feMacroFunction
	// 				std::vector<std::string>{},  // namesOfInputArgs
	// 				std::vector<std::string>{},
	// 				1,  // requiredUserPermissions
	// 				"*",
	// 				"Reset the EVB SERDES RX interface."
	// );
	// registerFEMacroFunction(
	// 	"Reset EVB Link Rx/Tx PLL",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&DTCFrontEndInterface::ResetEVBLinkRxTxPLL),            // feMacroFunction
	// 				std::vector<std::string>{},  // namesOfInputArgs
	// 				std::vector<std::string>{},
	// 				1,  // requiredUserPermissions
	// 				"*",
	// 				"Reset the EVB SERDES RX/TX PLL."
	// );

	registerFEMacroFunction(
	    "Get DTC ID and Hardware EVB Info",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::GetDTCIdAndEVBInfo),  // feMacroFunction
	    std::vector<std::string>{},                      // namesOfInputArgs
	    std::vector<std::string>{"Result"},
	    1,  // requiredUserPermissions
	    "*",
	    "Read the fields of the DTC ID and EVB Info register.");

	registerFEMacroFunction(
	    "Set DTC ID and Hardware EVB Info",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::SetDTCIdAndEVBInfo),  // feMacroFunction
	    std::vector<std::string>{"DTC ID",
	                             "EVB Mode",
	                             "EVB Partition ID",
	                             "EVB Self MAC Address Last Byte",
	                             "EVB Dead Time in Cluster",
	                             "EVB Number of DTCs in Cluster",
	                             "EVB Cluster Base DTC MAC Address"},  // namesOfInputArgs
	    std::vector<std::string>{"Result"},
	    1,  // requiredUserPermissions
	    "*",
	    "Read the fields of the DTC ID and EVB Info register.");

	//------------------

	registerFEMacroFunction(
	    "CFO Interface (Emulation) Setup",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::SetupCFOInterface),  // feMacroFunction
	    std::vector<std::string>{
	        "Put DTC in CFO Emulation Mode (Default := false)",
	        "Also setup Jitter Attenuator (Default := false)",
	        "Set Link RX/TX Enable (Default := false)",
	        "Enable Auto-generation of Data Request Packets (Default := false)",
	        "Force External CFO Sample Clock Edge (0 for rising-edge, 1 for "
	        "falling-edge, 2 for auto-find, Default := 0)",
	        "Permanent Offset (-2 to 2, Default := 0)",
	    },  // namesOfInputArgs
	    std::vector<std::string>{"Result"},
	    1,  // requiredUserPermissions
	    "*",
	    "Select or Deselect the CFO Emulator to take priority over the Link-6 external "
	    "CFO. When selected, the CFO timing link, Link-6, will be ignored.");

	registerFEMacroFunction(
	    "CFO Emulator On/Off Spill Emulation Setup",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::SetCFOEmulatorOnOffSpillEmulation),  // feMacroFunction
	    std::vector<std::string>{
	        "Enable CFO Emulator (Default := true)",
	        "Number of 1.4s super cycle repetitions (0 := infinite)",
	        "Starting Event Window Tag (Default or -1 := start from 0 and continue)",
	        "Enable Auto-generation of Data Request Packets (Default := false)",
	        "Enable Clock Markers (Default := false)",
	        "Use Detached Buffer Test (Default := false)",
	        "For Detached Buffer Test, Save Binary Data to File (Default: false)",
	        "For Detached Buffer Test, Save Binary Data Filename",
	        "For Detached Buffer Test, Save Subevent Header to Binary File (Default: "
	        "false)",
	        "For Detached Buffer Test, Do NOT Reset Counters (Default: false)",
	        "For Detached Buffer Test, Skip-by-32 to Emulate Event Building (Default: "
	        "false)",
	        "For Detached Buffer Test, Payload Packet Threshold for Saving Event "
	        "(Default: 0)"},  // namesOfInputArgs
	    std::vector<std::string>{"Result"},
	    1,  // requiredUserPermissions
	    "*",
	    "Enable/Disable the CFO Emulator. Disabling turns off output of emulated Event "
	    "Window Markers, timing markers, and Heartbeat Packets. " /* feMacroTooltip */
	    "Enabling turns on emulated Event Window generation and timing markers based on "
	    "the CFO emulator parameters.");
	registerFEMacroFunction(
	    "CFO Emulator Fixed-width Event Window Emulation Setup",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::SetCFOEmulatorFixedWidthEmulation),  // feMacroFunction
	    std::vector<std::string>{
	        "Enable CFO Emulator (Default := true)",
	        "Fixed-width Event Window Duration (s, ms, us, ns, and clocks allowed) "
	        "[clocks := 25ns]",
	        "Number of Event Window Markers to generate (0 := infinite)",
	        "Starting Event Window Tag (Default or -1 := start from 0 and continue)",
	        "Event Window Mode (Default := 1)",
	        "Enable Auto-generation of Data Request Packets (Default := false)",
	        "Enable Clock Markers (Default := false)",
	        "Use Detached Buffer Test (Default := false)",
	        "For Detached Buffer Test, Save Binary Data to File (Default: false)",
	        "For Detached Buffer Test, Save Binary Data Filename",
	        "For Detached Buffer Test, Save Subevent Header to Binary File (Default: "
	        "false)",
	        "For Detached Buffer Test, Do NOT Reset Counters (Default: false)",
	        "For Detached Buffer Test, Skip-by-32 to Emulate Event Building (Default: "
	        "false)",
	        "For Detached Buffer Test, Payload Packet Threshold for Saving Event "
	        "(Default: 0)"},  // namesOfInputArgs
	    std::vector<std::string>{"Result"},
	    1,  // requiredUserPermissions
	    "*",
	    "Enable/Disable the CFO Emulator. Disabling turns off output of emulated Event "
	    "Window Markers, timing markers, and Heartbeat Packets. " /* feMacroTooltip */
	    "Enabling turns on emulated Event Window generation and timing markers based on "
	    "the CFO emulator parameters.");
	registerFEMacroFunction(
	    "DTC Software Data Request",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::SoftwareDataRequest),  // feMacroFunction
	    std::vector<std::string>{"Event Window Tag"},     // namesOfInputArgs
	    std::vector<std::string>{"Result"},
	    1,  // requiredUserPermissions
	    "*",
	    "Send a software DR from the DTC.");

	registerFEMacroFunction(
	    "DTC Punched Clock",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::PunchedClock),              // feMacroFunction
	    std::vector<std::string>{"Enable (Default := true)"},  // namesOfInputArgs
	    std::vector<std::string>{},
	    1,  // requiredUserPermissions
	    "*",
	    "Punched Clock Enable/Disable.");

	registerFEMacroFunction(
	    "Program ROCs",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &DTCFrontEndInterface::ProgramROCs),  // feMacroFunction
	    std::vector<std::string>{
	        //First, only write the bitfile, manually readback .. do not reprogram yet!
	        "Target ROC or Mask (Default = -1 := all ROCs, or 0x111111 := all)",
	        "Path to Directory map file (Default := do not use)",
	        "Write Directory map to SPI Flash (Default := false)",
	        "Verify Directory map (Default := false)",
	        "Image Index (Default := 0)",
	        "Path to Bitfile (Default := do not write bitfile, only program from Image "
	        "Index)",
	        "Write Bitfile to SPI Flash (Default := false)",
	        "For Debug, force Write size (Default := do not force)",
	        "Verify with Bitfile Readback (Default := false)",
	        "Do program from Image Index (Default := false)",
	    },  // namesOfInputArgs
	    std::vector<std::string>{"Result"},
	    1,  // requiredUserPermissions
	    "*",
	    "Program one or many ROCs with an indexed image in the SPI, or the bitfile at a "
	    "specified filepath. Use Link=7 with Mask to choose more than 1 ROC manually "
	    "with the mask.");

	registerFEMacroFunction("Validate DTC Control Registers",
	                        static_cast<FEVInterface::frontEndMacroFunction_t>(
	                            &DTCFrontEndInterface::ValidateDTCControlRegisters),
	                        std::vector<std::string>{},          // input arguments
	                        std::vector<std::string>{"Status"},  // outputs
	                        1,  // requiredUserPermissions
	                        "*",
	                        "Tests each DTC Control Register (address 0x9100)");

	{  //add ROC FE Macros
		__FE_COUT__ << "Getting children ROC FEMacros..." << __E__;
		rocFEMacroMap_.clear();

		//first check if all ROCs are the same plugin type
		//	if they are, use ROC_UID to target -1 or individual ROC and only make one FE Macro entry to reprent all ROCs
		bool        allROCsAreSameType = true;
		std::string pluginType         = "";
		for(auto& roc : rocs_)
		{
			if(pluginType == "")
			{
				pluginType = roc.second->getInterfaceType();
				__FE_COUT__ << "First ROC plugin type is '" << pluginType << "'" << __E__;
			}
			else if(pluginType != roc.second->getInterfaceType())
			{
				allROCsAreSameType = false;
				__FE_COUT__ << "Not all ROCs are the same plugin type, so individual FE "
				               "Macros will be created."
				            << __E__;
				break;
			}
		}  //end check for same ROC plugin type

		for(auto& roc : rocs_)
		{
			auto feMacros = roc.second->getMapOfFEMacroFunctions();
			for(auto& feMacro : feMacros)
			{
				__FE_COUTT__ << roc.first << "::" << feMacro.first << __E__;

				if(!allROCsAreSameType)
				{
					//make DTC FEMacro forwarding to ROC FEMacro
					std::string macroName = "Link" +
					                        std::to_string(roc.second->getLinkID()) +
					                        "_" + roc.first + "_" + feMacro.first;
					__FE_COUTTV__(macroName);
					std::vector<std::string> inputArgs, outputArgs;
					for(auto& inArg : feMacro.second.namesOfInputArguments_)
						inputArgs.push_back(inArg);
					for(auto& outArg : feMacro.second.namesOfOutputArguments_)
						outputArgs.push_back(outArg);

					__FE_COUTTV__(StringMacros::vectorToString(inputArgs));
					__FE_COUTTV__(StringMacros::vectorToString(outputArgs));

					rocFEMacroMap_.emplace(std::make_pair(
					    macroName, std::make_pair(roc.first, feMacro.first)));

					registerFEMacroFunction(
					    macroName,
					    static_cast<FEVInterface::frontEndMacroFunction_t>(
					        &DTCFrontEndInterface::RunROCFEMacro),
					    inputArgs,
					    outputArgs,
					    1);  // requiredUserPermissions
				}
				else  //allROCsAreSameType
				{
					//make DTC FEMacro forwarding to ROC FEMacro
					std::string macroName = "ROC FEMacro - " + feMacro.first;
					__FE_COUTTV__(macroName);
					std::vector<std::string> inputArgs, outputArgs;
					//take ROC target as parameter for ROC FE Macros (allow -1 as wildcard for all)
					inputArgs.push_back(
					    "Target ROC or Mask (Default = -1 := all ROCs, or 0x111111 := "
					    "all)");
					for(auto& inArg : feMacro.second.namesOfInputArguments_)
						inputArgs.push_back(inArg);
					outputArgs.push_back(
					    "Target ROC");  //for display (especially to see which ROCs were targeted with -1)
					for(auto& outArg : feMacro.second.namesOfOutputArguments_)
						outputArgs.push_back(outArg);

					__FE_COUTTV__(StringMacros::vectorToString(inputArgs));
					__FE_COUTTV__(StringMacros::vectorToString(outputArgs));

					rocFEMacroMap_.emplace(std::make_pair(
					    macroName,
					    std::make_pair("" /* no ROC id, because applies to all */,
					                   feMacro.first)));

					registerFEMacroFunction(
					    macroName,
					    static_cast<FEVInterface::frontEndMacroFunction_t>(
					        &DTCFrontEndInterface::RunROCFEMacro),
					    inputArgs,
					    outputArgs,
					    1);  // requiredUserPermissions
				}
			}

			if(allROCsAreSameType)
			{
				__FE_COUT__ << "All ROCs are same plugin type, so defined by first ROC."
				            << __E__;
				break;
			}
		}
	}  //end add ROC FE Macros

	CFOandDTCCoreVInterface::registerCFOandDTCFEMacros();

}  // end registerFEMacros()

//==============================================================================
void DTCFrontEndInterface::configureSlowControls(void)
{
	__FE_COUTV__(skipInit_);
	if(skipInit_)
		return;

	bool slowControlsEnable = true;
	try
	{
		slowControlsEnable = getSelfNode().getNode("SlowControlsEnable").getValue<bool>();
	}
	catch(...)
	{
		__FE_COUT__ << "Missing `slowControlsEnable` in configuration, "
		               "slowControlsEnable defaults to "
		            << slowControlsEnable << __E__;
	}

	if(!slowControlsEnable)
	{
		__FE_COUT__ << "Slow controls are disabled..." << __E__;
		return;
	}
	__FE_COUT__ << "Configuring slow controls..." << __E__;

	// parent configure adds DTC slow controls channels
	FEVInterface::configureSlowControls();  // also resets mapOfSlowControlsChannels_

	__FE_COUT__ << "DTC '" << getInterfaceUID()
	            << "' slow controls channel count (BEFORE considering ROCs): "
	            << getSlowControlsChannelCount() << __E__;

	for(auto& roc : rocs_)
	{
		__FE_COUT__ << "Configuring DTC '" << getInterfaceUID() << "' ROC '"
		            << roc.second->getInterfaceUID() << "' slow controls..." << __E__;
		roc.second->configureSlowControls();
	}

	__FE_COUT__ << "DTC '" << getInterfaceUID()
	            << "' slow controls channel count (AFTER considering ROCs): "
	            << getSlowControlsChannelCount() << __E__;

	__FE_COUT__ << "Done configuring DTC+ROC slow controls." << __E__;
}  // end configureSlowControls()

//==============================================================================
// virtual in case channels are handled in multiple maps, for example
void DTCFrontEndInterface::resetSlowControlsChannelIterator(void)
{
	// call parent
	FEVInterface::resetSlowControlsChannelIterator();
	for(auto& roc : rocs_)
		roc.second->resetSlowControlsChannelIterator();
}  // end resetSlowControlsChannelIterator()

//==============================================================================
// virtual in case channels are handled in multiple maps, for example
FESlowControlsChannel* DTCFrontEndInterface::getNextSlowControlsChannel(void)
{
	// if not finished with DTC slow controls channels, return next DTC channel
	FESlowControlsChannel* nextSlowControlsChannel =
	    FEVInterface::getNextSlowControlsChannel();
	if(nextSlowControlsChannel != nullptr)
		return nextSlowControlsChannel;

	// else if finished with DTC slow controls channels, move on to ROC list

	for(auto& roc : rocs_)
	{
		nextSlowControlsChannel = roc.second->getNextSlowControlsChannel();
		if(nextSlowControlsChannel != nullptr)
			return nextSlowControlsChannel;
	}

	// else no more channels
	return nullptr;
}  // end getNextSlowControlsChannel()

//==============================================================================
// virtual in case channels are handled in multiple maps, for example
unsigned int DTCFrontEndInterface::getSlowControlsChannelCount(void)
{
	unsigned int rocChannelCount = 0;
	for(auto& roc : rocs_)
		rocChannelCount += roc.second->getSlowControlsChannelCount();
	return mapOfSlowControlsChannels_.size() + rocChannelCount;
}  // end getSlowControlsChannelCount()

//==============================================================================
void DTCFrontEndInterface::createROCs(void)
{
	rocs_.clear();

	auto rocLink = Configurable::getSelfNode().getNode("LinkToROCGroupTable");
	if(rocLink.isDisconnected())
	{
		__FE_COUT__ << "No ROC link table found, so no ROCs will be created." << __E__;
		return;
	}

	std::vector<std::pair<std::string, ConfigurationTree>> rocChildren =
	    rocLink.getChildren();

	// instantiate vector of ROCs
	for(auto& roc : rocChildren)
		if(roc.second.getNode("Status").getValue<bool>())
		{
			__FE_COUT__
			    << "ROC Plugin Name: "
			    << roc.second.getNode("ROCInterfacePluginName").getValue<std::string>()
			    << std::endl;
			__FE_COUT__ << "ROC Name: " << roc.first << std::endl;

			try
			{
				__FE_COUTV__(theXDAQContextConfigTree_.getValueAsString());
				__FE_COUTV__(
				    roc.second.getNode("ROCInterfacePluginName").getValue<std::string>());

				// Note: FEVInterface makeInterface returns a unique_ptr
				//	and we want to verify that ROCCoreVInterface functionality
				//	is there, so we do an intermediate dynamic_cast to check
				//	before placing in a new unique_ptr of type ROCCoreVInterface.
				std::unique_ptr<FEVInterface> tmpVFE = makeInterface(
				    roc.second.getNode("ROCInterfacePluginName").getValue<std::string>(),
				    roc.first,
				    theXDAQContextConfigTree_,
				    (theConfigurationPath_ + "/LinkToROCGroupTable/" + roc.first));

				// setup parent supervisor of FEVinterface (for backwards compatibility, left out of constructor), moved to virtual setParentPointers()
				tmpVFE->setParentPointers(parentSupervisor_, parentInterfaceManager_);

				ROCCoreVInterface& tmpRoc = dynamic_cast<ROCCoreVInterface&>(
				    *tmpVFE);  // dynamic_cast<ROCCoreVInterface*>(tmpRoc.get());

				// setup other members of ROCCore (for interface plug-in compatibility, left out of constructor)

				uint8_t roc_link_i = static_cast<uint8_t>(tmpRoc.getLinkID());
				bool    enabled    = ((roc_mask_ >> roc_link_i) & 1);
				bool    emulated   = ((roc_emulated_mask_ >> roc_link_i) & 1);
				__FE_COUT__ << "roc[" << (int)roc_link_i << "] enabled " << enabled
				            << " emulated " << emulated << __E__;
				tmpRoc.emulatedInDTC_ = emulated;
				tmpRoc.thisDTC_       = thisDTC_;
				tmpRoc
				    .onDTCReady();  // can be overridden by inheriting class to know when thisDTC_ is ready to use (which is after the ROC constructor completes)

				rocs_.emplace(std::pair<std::string, std::unique_ptr<ROCCoreVInterface>>(
				    roc.first, &tmpRoc));
				tmpVFE.release();  // release the FEVInterface unique_ptr, so we are left
				                   // with just one
				__FE_COUTV__(rocs_.at(roc.first)->parentSupervisor_);
			}
			catch(const cet::exception& e)
			{
				__FE_SS__ << "Failed to instantiate plugin named '" << roc.first
				          << "' of type '"
				          << roc.second.getNode("ROCInterfacePluginName")
				                 .getValue<std::string>()
				          << "' due to the following error: \n"
				          << e.what() << __E__;
				__FE_SS_THROW__;
			}
			catch(const std::bad_cast& e)
			{
				__SS__ << "Cast to ROCCoreVInterface failed! Verify the plugin inherits "
				          "from ROCCoreVInterface."
				       << __E__;
				ss << "Failed to instantiate plugin named '" << roc.first << "' of type '"
				   << roc.second.getNode("ROCInterfacePluginName").getValue<std::string>()
				   << "' due to the following error: \n"
				   << e.what() << __E__;

				__FE_SS_THROW__;
			}
		}

	__FE_COUT__ << "Done creating " << rocs_.size() << " ROC(s)" << std::endl;
}  // end createROCs()

//==================================================================================================
void DTCFrontEndInterface::configure(void)
try
{
	__FE_COUTV__(getIterationIndex());
	__FE_COUTV__(getSubIterationIndex());

	__FE_COUTV__(skipInit_);
	if(skipInit_)
		return;

	__FE_COUTV__(operatingMode_);

	if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_HARDWARE_DEV)
	{
		__FE_COUT_INFO__ << "Configuring for hardware development mode!" << __E__;
		configureHardwareDevMode();
	}
	else if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_EVENT_BUILDING ||
	        operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_LOOPBACK)
	{
		__FE_COUT_INFO__ << "Configuring for Event Building mode!" << __E__;
		configureEventBuildingMode();
	}
	// else if(operatingMode_ == "LoopbackMode")
	// {
	//	__FE_COUT_INFO__ << "Configuring for Loopback mode!" << __E__;
	//	configureLoopbackMode();
	// }
	else
	{
		__FE_SS__ << "Unknown system operating mode: " << operatingMode_ << __E__
		          << " Please specify a valid operating mode in the 'Mu2eGlobalsTable.'"
		          << __E__;
		__FE_SS_THROW__;
	}

	return;

	// /////////////////////////////
	// /////////////////////////////
	// ///////////////////////////// old configure
	// ///////////////////////////// old configure
	// ///////////////////////////// old configure
	// ///////////////////////////// old configure
	// /////////////////////////////
	// /////////////////////////////

	// if(getConfigurationManager()
	//	  ->getNode("/Mu2eGlobalsTable/SyncDemoConfig/SkipCFOandDTCConfigureSteps")
	//	  .getValue<bool>())
	// {
	//	__FE_COUT_INFO__ << "Skipping configure steps!" << __E__;
	//	return;
	// }

	// if(emulatorMode_)
	// {
	//	__FE_COUT__ << "Emulator DTC configuring... # of ROCs = " << rocs_.size()
	//		    << __E__;
	//	for(auto& roc : rocs_)
	//		roc.second->configure();
	//	return;
	// }

	// uint32_t dtcEventBuilderReg_DTCID = 0;
	// uint32_t dtcEventBuilderReg_Mode = 0;
	// uint32_t dtcEventBuilderReg_PartitionID = 0;
	// uint32_t dtcEventBuilderReg_MACIndex = 0;
	// // uint32_t dtcEventBuilderReg_DTCInfo = 0;

	// uint32_t dtcEventBuilderReg_NumBuff	     = 0;
	// uint32_t dtcEventBuilderReg_StartNode     = 0;
	// uint32_t dtcEventBuilderReg_NumNodes	     = 0;
	// // uint32_t dtcEventBuilderReg_Configuration = 0;

	// try
	// {
	//	dtcEventBuilderReg_DTCID =
	//	    getSelfNode().getNode("EventBuilderDTCID").getValue<uint32_t>();
	//	dtcEventBuilderReg_Mode =
	//	    getSelfNode().getNode("EventBuilderMode").getValue<uint32_t>();
	//	dtcEventBuilderReg_PartitionID =
	//	    getSelfNode().getNode("EventBuilderPartitionID").getValue<uint32_t>();
	//	dtcEventBuilderReg_MACIndex =
	//	    getSelfNode().getNode("EventBuilderMACIndex").getValue<uint32_t>();

	//	dtcEventBuilderReg_NumBuff =
	//	    getSelfNode().getNode("EventBuilderNumBuff").getValue<uint32_t>();
	//	dtcEventBuilderReg_StartNode =
	//	    getSelfNode().getNode("EventBuilderStartNode").getValue<uint32_t>();
	//	dtcEventBuilderReg_NumNodes =
	//	    getSelfNode().getNode("EventBuilderNumNodes").getValue<uint32_t>();

	//	__FE_COUTV__(dtcEventBuilderReg_DTCID);
	//	__FE_COUTV__(dtcEventBuilderReg_Mode);
	//	__FE_COUTV__(dtcEventBuilderReg_PartitionID);
	//	__FE_COUTV__(dtcEventBuilderReg_MACIndex);
	//	__FE_COUTV__(dtcEventBuilderReg_NumBuff);
	//	__FE_COUTV__(dtcEventBuilderReg_StartNode);
	//	__FE_COUTV__(dtcEventBuilderReg_NumNodes);

	//	// Register x9154 is #DTC ID [31-24] / EVB Mode [23-16]/ EVB Partition ID [15-8]/
	//	// EVB Local MAC Index [7-0]
	//	getDTC()->SetEVBInfo(dtcEventBuilderReg_DTCID,
	//		dtcEventBuilderReg_Mode,
	//		dtcEventBuilderReg_PartitionID,
	//		dtcEventBuilderReg_MACIndex);
	//	// dtcEventBuilderReg_DTCInfo =
	//	//     dtcEventBuilderReg_DTCID << 24 | dtcEventBuilderReg_Mode << 16 |
	//	//     dtcEventBuilderReg_PartitionID << 8 | dtcEventBuilderReg_MACIndex;
	//	// __FE_COUTV__(dtcEventBuilderReg_DTCInfo);
	//	// registerWrite(0x9154, dtcEventBuilderReg_DTCInfo);

	//	// Register x9158 is #Num EVB Buffers[22-16], EVB Start Node [14-8], Num Nodes
	//	// [6-0]
	//	getDTC()->SetEVBBufferInfo(dtcEventBuilderReg_NumBuff,
	//		dtcEventBuilderReg_StartNode,
	//		dtcEventBuilderReg_NumNodes);
	//	// dtcEventBuilderReg_Configuration = dtcEventBuilderReg_NumBuff << 16 |
	//	//				      dtcEventBuilderReg_StartNode << 8 |
	//	//				      dtcEventBuilderReg_NumNodes;
	//	// __FE_COUTV__(dtcEventBuilderReg_Configuration);
	//	// registerWrite(0x9158, dtcEventBuilderReg_Configuration);
	// }
	// catch(...)
	// {
	//	__FE_COUT_INFO__ << "Ignoring missing event building configuration values."
	//			 << __E__;
	// }
	// // end of the new code

	// __FE_COUT__ << "DTC configuring... # of ROCs = " << rocs_.size() << __E__;

	// // NOTE: otsdaq/xdaq has a soap reply timeout for state transitions.
	// // Therefore, break up configuration into several steps so as to reply before
	// // the time out As well, there is a specific order in which to configure the
	// // links in the chain of CFO->DTC0->DTC1->...DTCN

	// const int number_of_system_configs =
	//     -1;		      // if < 0, keep trying until links are OK.
	//			      // If > 0, go through configuration steps this many times

	// const int reset_fpga = 1;  // 1 = yes, 0 = no
	// // const bool config_clock		  = configure_clock_;  // 1 = yes, 0 = no
	// const bool config_jitter_attenuator = configure_clock_;  // 1 = yes, 0 = no
	// // const int	 reset_rx		  = 0;		       // 1 = yes, 0 = no

	// const int number_of_dtc_config_steps = 7;

	// const int max_number_of_tries = 3;  // max number to wait for links OK

	// int number_of_total_config_steps =
	//     number_of_system_configs * number_of_dtc_config_steps;

	// int config_step    = getIterationIndex();
	// int config_substep = getSubIterationIndex();

	// bool isLastTimeThroughConfigure = false;

	// if(number_of_system_configs > 0)
	// {
	//	if(config_step >= number_of_total_config_steps)	 // done, exit system config
	//		return;
	// }

	// // waiting for link loop
	// if(config_substep > 0 && config_substep < max_number_of_tries)
	// {  // wait a maximum of 30 seconds

	//	const int number_of_link_checks = 10;

	//	// int link_ok = 0;

	//	for(int i = 0; i < number_of_link_checks; i++)
	//	{
	//		if(checkLinkStatus() == 1)
	//		{
	//			// links OK,  continue with the rest of the configuration
	//			__FE_COUT__ << device_name_ << " Link Status is OK = 0x" << std::hex
	//				    << registerRead(0x9140) << std::dec << __E__;

	//			indicateIterationWork();
	//			turnOffLED();
	//			return;
	//		}
	//		else if(getCFOLinkStatus() == 0)
	//		{
	//			// in this case, links will never get to be OK, stop waiting for them

	//			__FE_COUT__ << device_name_ << " CFO Link Status is bad = 0x" << std::hex
	//				    << registerRead(0x9140) << std::dec << __E__;

	//			// usleep(500000 /*500ms*/);
	//			sleep(1);

	//			indicateIterationWork();
	//			turnOffLED();
	//			return;
	//		}
	//		else
	//		{
	//			// links still not OK, keep checking...

	//			__FE_COUT__ << "Waiting for DTC Link Status = 0x" << std::hex
	//				    << registerRead(0x9140) << std::dec << __E__;
	//			// usleep(500000 /*500ms*/);
	//			sleep(1);
	//		}
	//	}

	//	indicateSubIterationWork();
	//	return;
	// }
	// else if(config_substep > max_number_of_tries)
	// {
	//	// wait a maximum of 30 seconds, then stop waiting for them

	//	__FE_COUT__ << "Links still bad = 0x" << std::hex << registerRead(0x9140)
	//		    << std::dec << "... continue" << __E__;
	//	indicateIterationWork();
	//	turnOffLED();
	//	return;
	// }

	// turnOnLED();

	// if((config_step % number_of_dtc_config_steps) == 0)
	// {
	//	__FE_COUTV__(GetFirmwareVersion());
	//	if(reset_fpga == 1 && config_step < number_of_dtc_config_steps)
	//	{
	//		// only reset the FPGA the first time through
	//		__FE_COUT_INFO__ << "Step " << config_step << ": " << device_name_
	//				 << " Reset DTC " << __E__;

	//		DTCSoftReset();
	//	}

	//	// From Rick new code
	//	uint32_t dtcEventBuilderReg_DTCID	= 0;
	//	uint32_t dtcEventBuilderReg_Mode	= 0;
	//	uint32_t dtcEventBuilderReg_PartitionID = 0;
	//	uint32_t dtcEventBuilderReg_MACIndex	= 0;
	//	uint32_t dtcEventBuilderReg_DTCInfo	= 0;

	//	uint32_t dtcEventBuilderReg_NumBuff	  = 0;
	//	uint32_t dtcEventBuilderReg_StartNode	  = 0;
	//	uint32_t dtcEventBuilderReg_NumNodes	  = 0;
	//	uint32_t dtcEventBuilderReg_Configuration = 0;

	//	try
	//	{
	//		__FE_COUT__ << "Configuring DTC registers for the EVB" << rocs_.size()
	//			    << __E__;

	//		dtcEventBuilderReg_DTCID =
	//		    getSelfNode().getNode("EventBuilderDTCID").getValue<uint32_t>();
	//		dtcEventBuilderReg_Mode =
	//		    getSelfNode().getNode("EventBuilderMode").getValue<uint32_t>();
	//		dtcEventBuilderReg_PartitionID =
	//		    getSelfNode().getNode("EventBuilderPartitionID").getValue<uint32_t>();
	//		dtcEventBuilderReg_MACIndex =
	//		    getSelfNode().getNode("EventBuilderMACIndex").getValue<uint32_t>();

	//		dtcEventBuilderReg_NumBuff =
	//		    getSelfNode().getNode("EventBuilderNumBuff").getValue<uint32_t>();
	//		dtcEventBuilderReg_StartNode =
	//		    getSelfNode().getNode("EventBuilderStartNode").getValue<uint32_t>();
	//		dtcEventBuilderReg_NumNodes =
	//		    getSelfNode().getNode("EventBuilderNumNodes").getValue<uint32_t>();

	//		__FE_COUTV__(dtcEventBuilderReg_DTCID);	 // Doesn't work if I use uint8_t
	//		__FE_COUTV__(dtcEventBuilderReg_Mode);
	//		__FE_COUTV__(dtcEventBuilderReg_PartitionID);
	//		__FE_COUTV__(dtcEventBuilderReg_MACIndex);
	//		__FE_COUTV__(dtcEventBuilderReg_NumBuff);
	//		__FE_COUTV__(dtcEventBuilderReg_StartNode);
	//		__FE_COUTV__(dtcEventBuilderReg_NumNodes);

	//		// Register x9154 is #DTC ID [31-24] / EVB Mode [23-16]/ EVB Partition ID
	//		// [15-8]/ EVB Local MAC Index [7-0]
	//		dtcEventBuilderReg_DTCInfo =
	//		    dtcEventBuilderReg_DTCID << 24 | dtcEventBuilderReg_Mode << 16 |
	//		    dtcEventBuilderReg_PartitionID << 8 | dtcEventBuilderReg_MACIndex;
	//		__FE_COUTV__(dtcEventBuilderReg_DTCInfo);
	//		registerWrite(0x9154, dtcEventBuilderReg_DTCInfo);

	//		// Register x9158 is #Num EVB Buffers[22-16], EVB Start Node [14-8], Num Nodes
	//		// [6-0]
	//		dtcEventBuilderReg_Configuration = dtcEventBuilderReg_NumBuff << 16 |
	//						   dtcEventBuilderReg_StartNode << 8 |
	//						   dtcEventBuilderReg_NumNodes;
	//		__FE_COUTV__(dtcEventBuilderReg_Configuration);
	//		registerWrite(0x9158, dtcEventBuilderReg_Configuration);
	//	}
	//	catch(...)
	//	{
	//		__FE_COUT_INFO__ << "Ignoring missing event building configuration values."
	//				 << __E__;
	//	}
	//	// end of the new code
	// }
	// else if((config_step % number_of_dtc_config_steps) == 1)
	// {
	//	__FE_COUT_INFO__ << "Step" << config_step << ": " << device_name_
	//			 << "select/setup clock..." << __E__;

	//	// choose jitter attenuator input select (reg 0x9308, bits 5:4)
	//	//  0 is Upstream Control Link Rx Recovered Clock
	//	//  1 is RJ45 Upstream Clock
	//	//  2 is Timing Card Selectable (SFP+ or FPGA) Input Clock
	//	{
	//		uint32_t readData = registerRead(0x9308);
	//		uint32_t val	  = 0;
	//		try
	//		{
	//			val = getSelfNode()
	//				  .getNode("JitterAttenuatorInputSource")
	//				  .getValue<uint32_t>();
	//		}
	//		catch(...)
	//		{
	//			__FE_COUT__ << "Defaulting Jitter Attenuator Input Source to val = "
	//				    << val << __E__;
	//		}
	//		readData &= ~(3 << 4);	     // clear the two bits
	//		readData &= ~(1);	     // ensure unreset of jitter attenuator
	//		readData |= (val & 3) << 4;  // set the two bits to selected value

	//		registerWrite(0x9308, readData);
	//		__FE_COUT__
	//		    << "Jitter Attenuator Input Select: " << val << " ==> "
	//		    << (val == 0
	//			    ? "Upstream Control Link Rx Recovered Clock"
	//			    : (val == 1
	//				   ? "RJ45 Upstream Clock"
	//				   : "Timing Card Selectable (SFP+ or FPGA) Input Clock"))
	//		    << __E__;
	//	}
	// }
	// else if((config_step % number_of_dtc_config_steps) == 2)
	// {
	//	// configure Jitter Attenuator to recover clock
	//	if((config_jitter_attenuator == 1 || emulate_cfo_ == 1) &&
	//	   config_step < number_of_dtc_config_steps)
	//	{
	//		__FE_COUT_INFO__ << "Step " << config_step << ": " << device_name_
	//				       << " configure Jitter Attenuator..." << __E__;

	//		// It's needed only after a powercycle
	//		configureJitterAttenuator();

	//		usleep(500000 /*500ms*/);
	//		sleep(1);
	//	}
	//	else
	//	{
	//		__FE_COUT_INFO__ << "Step " << config_step << ": " << device_name_
	//				       << " do NOT configure Jitter Attenuator..." << __E__;
	//	}
	// }
	// else if((config_step % number_of_dtc_config_steps) == 3)
	// {
	//	if(emulate_cfo_ == 1)
	//	{
	//		__FE_COUT_INFO__ << "Step " << config_step << ": " << device_name_
	//				       << " enable CFO emulation and internal clock" << __E__;

	//		int dataToWrite = 0x40808404;
	//		registerWrite(0x9100, dataToWrite);  // This disable retransmission + set the
	//						     // CFO in emulation mode

	//		// Micol thinks this two lines below are not needed
	//		//__FE_COUT__ << ".......CFO emulation: time between data requests" << __E__;
	//		// registerWrite(0x91a8, 0x4e20);
	//	}
	//	else
	//	{
	//		int dataInReg = registerRead(0x9100);
	//		int dataToWrite =
	//		    dataInReg &
	//		    0xbfff7fff;	 // bit 30 = CFO emulation enable; bit 15 CFO emulation mode
	//		registerWrite(0x9100, dataToWrite);
	//	}
	// }
	// else if((config_step % number_of_dtc_config_steps) == 4) {}
	// else if((config_step % number_of_dtc_config_steps) == 5)
	// {
	//	__FE_COUT_INFO__ << "Step " << config_step << ": " << device_name_
	//			       << " enable markers, Tx, Rx" << __E__;

	//	// enable markers, tx and rx

	//	int data_to_write = (roc_mask_ << 8) | roc_mask_;
	//	__FE_COUT__ << "CFO enable markers - enabled ROC links 0x" << std::hex
	//		    << data_to_write << std::dec << __E__;
	//	registerWrite(0x91f8, data_to_write);

	//	data_to_write = 0x4040 | (roc_mask_ << 8) | roc_mask_;
	//	__FE_COUT__ << "DTC enable tx and rx - CFO and enabled ROC links 0x" << std::hex
	//		    << data_to_write << std::dec << __E__;
	//	registerWrite(0x9114, data_to_write);

	//	data_to_write = 0x00014141;  // DMA timeout from chants.
	//	__FE_COUT__ << "set DMA timeout" << std::hex << data_to_write << std::dec
	//		    << __E__;
	//	registerWrite(0x9144, data_to_write);

	//	// put DTC CFO link output into loopback mode
	//	__FE_COUT__ << "DTC set CFO link output loopback mode ENABLE" << __E__;

	//	__FE_COUT_INFO__ << "Step " << config_step << ": " << device_name_ << " configure ROCs"
	//			       << __E__;

	//	bool doConfigureROCs = false;
	//	try
	//	{
	//		doConfigureROCs = Configurable::getSelfNode()
	//				      .getNode("EnableROCConfigureStep")
	//				      .getValue<bool>();
	//	}
	//	catch(...)
	//	{
	//	}  // ignore missing field
	//	if(doConfigureROCs)
	//		for(auto& roc : rocs_)
	//			roc.second->configure();

	//	// usleep(500000 /*500ms*/);
	//	sleep(1);
	// }
	// else if((config_step % number_of_dtc_config_steps) == 6)
	// {
	//	if(emulate_cfo_ == 1)
	//	{
	//		__FE_COUT_INFO__ << "Step " << config_step
	//				       << ": CFO emulation enable Event start characters "
	//					  "and event window interval"
	//				       << __E__;

	//		__FE_COUT__ << "CFO emulation:	set Event Window interval" << __E__;
	//		registerWrite(0x91f0, 0x00000000);  // for NO markers, write these

	//		__FE_COUT__ << "CFO emulation:	set 40MHz marker interval" << __E__;
	//		registerWrite(0x91f4, 0x00000000);  // for NO markers, write these

	//		__FE_COUT__ << "CFO emulation:	set heartbeat interval " << __E__;
	//	}
	//	__FE_COUT_INFO__ << "Step " << config_step << ": " << device_name_ << " configured"
	//			       << __E__;

	//	__FE_COUTV__(getIterationIndex());
	//	__FE_COUTV__(getSubIterationIndex());

	//	if(checkLinkStatus() == 1)
	//	{
	//		__FE_COUT_INFO__ << device_name_ << " links OK 0x" << std::hex
	//					    << registerRead(0x9140) << std::dec << __E__;

	//		// usleep(500000 ); //500ms/
	//		sleep(1);
	//		turnOffLED();

	//		if(number_of_system_configs < 0)
	//		{
	//			isLastTimeThroughConfigure = true;
	//			// do a final DTC Reset
	//			//__FE_COUT_INFO__ << "Last step in configuration; doing DTCSoftReset" << __E__;
	//			// DTCSoftReset();
	//		}
	//	}
	//	else if(config_step > max_number_of_tries)
	//	{
	//		isLastTimeThroughConfigure = true;
	//		__FE_COUT_INFO__ << device_name_ << " after " << max_number_of_tries
	//					    << " tries, links not OK 0x" << std::hex
	//					    << registerRead(0x9140) << std::dec << __E__;
	//	}
	//	else
	//	{
	//		__FE_COUT_INFO__ << device_name_ << " links not OK 0x" << std::hex
	//					    << registerRead(0x9140) << std::dec << __E__;
	//	}

	//	__FE_COUTV__(isLastTimeThroughConfigure);
	//	if(isLastTimeThroughConfigure)
	//	{
	//		sleep(2);
	//		// write anything to reset
	//		// 0x93c8 is RX CDR Unlock counter (32-bit)
	//		__FE_COUT_INFO__ << "LAST STEP!! Reset Loss-of-Lock Counter() on DTC" << __E__;

	//		registerWrite(0x93c8, 0);

	//		// Registers to set the EVB
	//		try
	//		{
	//			__FE_COUT__ << "Configuring DTC registers for the EVB" << rocs_.size()
	//				    << __E__;
	//			// These registers are needed for the EVB, but I need to check their
	//			// meaning
	//			registerWrite(0x9100, 0x800404);
	//			registerWrite(0x92c0, 0x0);
	//			registerWrite(0x9114, 0xc1c1);
	//			registerWrite(0x96C8, 0x555555D5);
	//			registerWrite(0x96CC, 0x78555555);
	//		}
	//		catch(...)
	//		{
	//			__FE_COUT_INFO__
	//			    << "Ignoring missing event building DTC registers values." << __E__;
	//		}
	//		// end of the new code

	//		return;	 // links OK, kick out of configure OR link tries complete
	//	}
	// }

	// readStatus();	     // spit out link status at every step

	// indicateIterationWork();  // otherwise, tell state machine to stay in configure
	//			     // state ("come back to me")

	// turnOffLED();

	// return;
}  // end configure()
catch(const std::runtime_error& e)
{
	__FE_SS__ << "Error caught: " << e.what() << __E__;
	__FE_SS_THROW__;
}
catch(const std::exception& e)
{
	__FE_SS__ << "Error caught: " << e.what() << __E__;
	__FE_SS_THROW__;
}
catch(...)
{
	__FE_SS__ << "Unknown error caught. Check the printouts!" << __E__;
	try
	{
		throw;
	}  //one more try to printout extra info
	catch(const std::exception& e)
	{
		ss << "Exception message: " << e.what();
	}
	catch(...)
	{
	}
	__FE_SS_THROW__;
}

void DTCFrontEndInterface::configureCommon(void)
{
	__FE_COUT_INFO__ << "configureCommon()" << __E__;

	getDTC()->SoftReset();
	getDTC()->ReleaseAllBuffers(DTC_DMA_Engine_DCS);

	// setup ROCs and check if any ROCs should be DTC-hardware emulated ROCs
	{
		//enable ROC links (do not forget CFO link is off in HW dev mode)
		__FE_COUT__ << "Enabling/Disabling DTC links with ROC mask = " << roc_mask_
		            << " and emulated mask = " << roc_emulated_mask_ << __E__;

		//disable all ROCs by default
		std::string rocSetupString;
		for(size_t i = 0; i < DTCLib::DTC_ROC_Links.size(); ++i)
		{
			bool enabled  = ((roc_mask_ >> i) & 1);
			bool emulated = ((roc_emulated_mask_ >> i) & 1);
			__FE_COUT__ << "roc[" << i << "] enabled " << enabled << " emulated "
			            << emulated << __E__;

			if(!enabled)
				rocSetupString = SetupROCs(
				    DTCLib::DTC_Link_ID(i),  //DTCLib::DTC_Link_ID rocLinkIndex,
				    0,
				    1,
				    0,  //bool rocRxTxEnable, bool rocTimingEnable, bool rocEmulationEnable,
				    DTCLib::DTC_ROC_Emulation_Type(
				        0 /* 0: Internal, 1: Fiber-Loopback, 2: External */),  // DTCLib::DTC_ROC_Emulation_Type rocEmulationType,
				    0  // uint32_t size
				);
			else if(enabled && !emulated)
			{
				bool clockMakersEnabled = false;  // TODO
				if(getCFOandDTCRegisters()->isCRVDTCDesignFlavour())
				{
					__FE_COUT__ << "enable punched clock on CRV DTC" << __E__;
					clockMakersEnabled =
					    false;  // clock markers are always off for the CRV
				}
				rocSetupString = SetupROCs(
				    DTCLib::DTC_Link_ID(i),  //DTCLib::DTC_Link_ID rocLinkIndex,
				    1,
				    clockMakersEnabled,
				    0,  //bool rocRxTxEnable, bool rocTimingEnable, bool rocEmulationEnable,
				    DTCLib::DTC_ROC_Emulation_Type(
				        0 /* 0: Internal, 1: Fiber-Loopback, 2: External */),  // DTCLib::DTC_ROC_Emulation_Type rocEmulationType,
				    0  // uint32_t size
				);
			}
			else  //enabled and emulated
				rocSetupString = SetupROCs(
				    DTCLib::DTC_Link_ID(i),  //DTCLib::DTC_Link_ID rocLinkIndex,
				    1,
				    1,
				    1,  //bool rocRxTxEnable, bool rocTimingEnable, bool rocEmulationEnable,
				    DTCLib::DTC_ROC_Emulation_Type(
				        0 /* 0: Internal, 1: Fiber-Loopback, 2: External */),  // DTCLib::DTC_ROC_Emulation_Type rocEmulationType,
				    16  // uint32_t size
				);
		}

		__FE_COUT__ << "ROC Setup:\n" << rocSetupString << __E__;
	}  // end check if any ROCs should be DTC-hardware emulated ROCs

	// getDTC()->EnableCFOEmulation(); // this will enable sending requests, so do it later after configuring event
	getDTC()->EnableDCSReception();

	// If this is a CRV ROC, enable the punched clock by default
	if(getCFOandDTCRegisters()->isCRVDTCDesignFlavour())
	{
		__FE_COUT__ << "enable punched clock on CRV DTC" << __E__;
		getDTC()->SetPunchEnable();
	}

	// set the DTC ID, "data in this field are passed to the DTC ID field
	// of the Event Header in built events." from docdb 4097
	try
	{
		getDTC()->DisableLink(DTCLib::DTC_Link_EVB);

		uint32_t dtcEventBuilderReg_DTCID =
		    getSelfNode().getNode("EventBuilderDTCID").getValue<uint32_t>();
		uint32_t dtcEventBuilderReg_Mode =
		    getSelfNode().getNode("EventBuilderMode").getValue<uint32_t>();
		uint32_t dtcEventBuilderReg_PartitionID =
		    getSelfNode().getNode("EventBuilderPartitionID").getValue<uint32_t>();
		uint32_t dtcEventBuilderReg_MACIndex =
		    getSelfNode().getNode("EventBuilderMACIndex").getValue<uint32_t>();
		__FE_COUTV__(dtcEventBuilderReg_DTCID);
		__FE_COUTV__(dtcEventBuilderReg_Mode);
		__FE_COUTV__(dtcEventBuilderReg_PartitionID);
		__FE_COUTV__(dtcEventBuilderReg_MACIndex);

		// Register x9154 is #DTC ID [31-24] / EVB Mode [23-16]/ EVB Partition ID [15-8]/
		// EVB Local MAC Index [7-0]
		getDTC()->SetEVBInfo(dtcEventBuilderReg_DTCID,
		                     dtcEventBuilderReg_Mode,
		                     dtcEventBuilderReg_PartitionID,
		                     dtcEventBuilderReg_MACIndex);
	}
	catch(...)
	{
		__FE_COUT_INFO__ << "Ignoring missing event building configuration values."
		                 << __E__;
	}

	//in 13-Oct-2023 tests with Rick, reseting the serdes PLL brought the ROC tx back up
	// for(size_t i=0;i<DTCLib::DTC_PLLs.size();++i)
	//	getDTC()->ResetSERDESPLL(DTCLib::DTC_PLLs[i]);

	// getDTC()->ResetSERDESRX(DTCLib::DTC_Link_ID::DTC_Link_ALL);
	// getDTC()->ResetSERDESTX(DTCLib::DTC_Link_ID::DTC_Link_ALL);

	getDTC()->SoftReset();  // soft reset to clear lock counters

	//at this point should have stable ROC links, and be ready for DCS-based configure of ROCs:
	//------------------------------ ROCs //------------------------------
	bool doConfigureROCs = false;
	try
	{
		doConfigureROCs = Configurable::getSelfNode()
		                      .getNode("EnableROCConfigureStep")
		                      .getValue<bool>();
	}
	catch(...)
	{
	}  // ignore missing field

	if(doConfigureROCs)
	{
		for(auto& roc : rocs_)
		{
			// make sure the roc link is ready before configuring
			if(!getDTC()->WaitForLinkReady(
			       roc.second->getLinkID(), 1000 /*us*/, 2.0 /*seconds*/))
			{
				__FE_SS__ << "ROC " << roc.first << " on link " << roc.second->getLinkID()
				          << " was not ready after 2s. Aborting ROC configuration.";
				__FE_SS_THROW__;
			}
			roc.second->configure();
		}  //end roc configure loop
	}

	bool EnableSoftwareDataRequestMode = false;  //default to auto-gen DRP
	try
	{
		EnableSoftwareDataRequestMode =
		    getSelfNode().getNode("EnableSoftwareDataRequestMode").getValue<bool>();
	}
	catch(...)
	{
		;
	}  //ignore exceptions;
	if(EnableSoftwareDataRequestMode)
	{
		__FE_COUT__ << "Enabling Software Data Request Mode..." << __E__;
		getDTC()->EnableSoftwareDRP();
	}
	else
	{
		__FE_COUT__ << "Enabling Auto-generation of Data Requests..." << __E__;
		getDTC()->DisableSoftwareDRP();
	}
}

//==============================================================================
void DTCFrontEndInterface::configureHardwareDevMode(void)
{
	__FE_COUT_INFO__ << "configureHardwareDevMode()" << __E__;

	//Steps:
	//	- disable CFO
	//	- setup JA
	//	- setup ROCs
	//	- Soft Reset
	//	- enable CFO emulation and DCS
	//	- configure ROCs

	getDTC()->DisableCFOEmulation();
	getDTC()->SetCFOEmulationMode();  //turn on DTC emulation (ignores any real CFO)
	getDTC()->DisableLink(DTCLib::DTC_Link_CFO);

	//During debug session on 14-Nov-2023, realized JA config breaks ROC link CDR lock
	//	So solution:
	//		- only configure JA one time ever after cold start
	//		- from then on, do not touch JA
	if(configure_clock_)
	{
		uint32_t select = 0;
		try
		{
			select =
			    getSelfNode().getNode("JitterAttenuatorInputSource").getValue<uint32_t>();
		}
		catch(...)
		{
			__FE_COUT__ << "Defaulting Jitter Attenuator Input Source to select = "
			            << select << __E__;
		}

		__FE_COUTV__(select);
		//For DTC - 0 ==> CFO Control Link
		//For DTC - 1 ==> RTF copper clock
		//For DTC - 2 ==> FPGA FMC
		getDTC()->SetJitterAttenuatorSelect(
		    select,
		    true /* alsoResetJA */);  // this call should first check if JA is already locked, JA only needs to be set after a cold start or if input clock changes
	}
	else
		__FE_COUT_INFO__ << "Skipping configure clock." << __E__;

	configureCommon();

	bool EnableSoftwareDataRequestMode = false;  //default to auto-gen DRP
	try
	{
		EnableSoftwareDataRequestMode =
		    getSelfNode().getNode("EnableSoftwareDataRequestMode").getValue<bool>();
	}
	catch(...)
	{
		;
	}  //ignore exceptions;

	//enable CFO emulator
	emulate_cfo_ = true;
	SetupCFOInterface(0,                                //int forceCFOedge,
	                  emulate_cfo_,                     //bool useCFOemulator,
	                  false,                            //bool alsoSetupJA,
	                  true,                             //bool cfoRxTxEnable,
	                  !EnableSoftwareDataRequestMode);  //bool enableAutogenDRP);
}  // end configureHardwareDevMode()

//==============================================================================
void DTCFrontEndInterface::configureEventBuildingMode(int step)
{
	if(step == -1)
		step = getIterationIndex();

	__FE_COUT_INFO__ << "configureEventBuildingMode() " << step << __E__;

	if(emulate_cfo_)  //when no CFO, what is event building mode?
	{
		__FE_SS__ << "There is no CFO! Event Building Mode is invalid." << __E__;
		__SS_THROW__;
	}

	if(step < CFOandDTCCoreVInterface::CONFIG_DTC_TIMING_CHAIN_START_INDEX)
	{
		__FE_COUT__ << "Do nothing while CFO configures for timing chain." << __E__;
		indicateIterationWork();
	}
	else if(step < CFOandDTCCoreVInterface::CONFIG_DTC_TIMING_CHAIN_START_INDEX +
	                   CFOandDTCCoreVInterface::CONFIG_DTC_TIMING_CHAIN_STEPS)
	{
		configureForTimingChain(
		    getIterationIndex() -
		    CFOandDTCCoreVInterface::
		        CONFIG_DTC_TIMING_CHAIN_START_INDEX /* start case index!! */);
		indicateIterationWork();
	}
	else if(step == CFOandDTCCoreVInterface::CONFIG_DTC_TIMING_CHAIN_START_INDEX +
	                    CFOandDTCCoreVInterface::CONFIG_DTC_TIMING_CHAIN_STEPS)
	{
		__FE_COUT__ << "Do nothing while CFO reset its Tx..." << __E__;
		indicateIterationWork();
	}
	else if(step == 1 + CFOandDTCCoreVInterface::CONFIG_DTC_TIMING_CHAIN_START_INDEX +
	                    CFOandDTCCoreVInterface::CONFIG_DTC_TIMING_CHAIN_STEPS)
	{
		configureCommon();

		// getDTC()->SetSequenceNumberDisable(); //bit 10

		// registerWrite(0x92c0, 0x0);
		// getDTC()->ClearEventModeTableEnable();
		// getDTC()->SetEventModeLookupByteSelect(0);

		// registerWrite(0x9114, 0xc1c1);
		getDTC()->EnableLink(DTCLib::DTC_Link_EVB);

		uint32_t EventModeRequiredMask = uint32_t(0);
		// sets the bits that are required. If a mask-bit is 0 its accepted anyways.
		// If a mask-bit is 1, then the eventMode-bit also needs to be 1.
		try
		{
			EventModeRequiredMask =
			    getSelfNode().getNode("EventModeRequiredMask").getValue<uint32_t>();
		}
		catch(...)
		{
			__FE_COUT_INFO__ << "No 'EventModeRequiredMask' field found. Default to 0x"
			                 << std::hex << EventModeRequiredMask << __E__;
		}
		getDTC()->SetCFOEventModeRequiredMask(EventModeRequiredMask);

		// registerWrite(0x96C8, 0x555555D5);	//10G configurable preamble world
		// registerWrite(0x96CC, 0x78555555);	//10G configurable idle world

		__FE_COUT__ << "Setup EVB parameters done." << __E__;
	}
	else
		__FE_COUT__ << "Do nothing while other configurable entities finish..." << __E__;

}  // end configureEventBuildingMode()

//==============================================================================
void DTCFrontEndInterface::configureLoopbackMode(int step)
{
	__FE_COUT_INFO__ << "configureLoopbackMode() " << step << __E__;

	if(emulate_cfo_)  //when no CFO, what is loopback mode?
	{
		__FE_SS__ << "There is no CFO! Loopback Mode is invalid." << __E__;
		__SS_THROW__;
	}

}  // end configureLoopbackMode()

//==============================================================================
void DTCFrontEndInterface::configureForTimingChain(int step)
{
	__FE_COUT_INFO__ << "configureForTimingChain() " << step << __E__;

	//Jun/18/2023 14:00 raw-data: 0x23061814
	switch(step)
	{
	case 0:
		//put DTC in known state with DTC reset and control clear
		getDTC()->SoftReset();
		getDTC()->ClearControlRegister();

		indicateIterationWork();
		break;
	case 1:
		//During debug session on 14-Nov-2023, realized JA config breaks ROC link CDR lock
		//	So solution:
		//		- only configure JA one time ever after cold start
		//		- from then on, do not touch JA
		if(configure_clock_)
		{
			uint32_t select = 0;
			try
			{
				select = getSelfNode()
				             .getNode("JitterAttenuatorInputSource")
				             .getValue<uint32_t>();
			}
			catch(...)
			{
				__FE_COUT__ << "Defaulting Jitter Attenuator Input Source to select = "
				            << select << __E__;
			}

			__FE_COUTV__(select);
			//For DTC - 0 ==> CFO Control Link
			//For DTC - 1 ==> RTF copper clock
			//For DTC - 2 ==> FPGA FMC
			getDTC()->SetJitterAttenuatorSelect(select, true /* alsoResetJA */);
		}
		else
			__FE_COUT_INFO__ << "Skipping configure clock." << __E__;

		indicateIterationWork();
		break;
	case 2:

		// check if any ROCs should be DTC-hardware emulated ROCs

		{
			auto rocLink = Configurable::getSelfNode().getNode("LinkToROCGroupTable");
			if(!rocLink.isDisconnected())
			{
				std::vector<std::pair<std::string, ConfigurationTree>> rocChildren =
				    rocLink.getChildren();

				int dtcHwEmulateROCmask = 0;
				for(auto& roc : rocChildren)
				{
					bool enabled =
					    roc.second.getNode("EmulateInDTCHardware").getValue<bool>();

					if(enabled)
					{
						int linkID = roc.second.getNode("linkID").getValue<int>();
						__FE_COUT__ << "roc uid '" << roc.first << "' at link=" << linkID
						            << " is DTC-hardware emulated!" << __E__;
						dtcHwEmulateROCmask |= (1 << linkID);
					}
				}

				__FE_COUT__ << "Writing DTC-hardware emulation mask: 0x" << std::hex
				            << dtcHwEmulateROCmask << std::dec << __E__;
				getDTC()->SetROCEmulatorMask(dtcHwEmulateROCmask);
				// registerWrite(0x9110, dtcHwEmulateROCmask);
				__FE_COUT__ << "End check for DTC-hardware emulated ROCs." << __E__;
			}  // end check if any ROCs should be DTC-hardware emulated ROCs
			else
			{
				__FE_COUT_INFO__
				    << "LinkToROCGroupTable is disconnected; writing ROC emulator mask 0 "
				       "to ensure deterministic hardware state."
				    << __E__;
				getDTC()->SetROCEmulatorMask(0);
			}
		}

		//enable ROC links w/CFO link
		__FE_COUT__ << "Enabling/Disabling DTC links with ROC mask = " << roc_mask_
		            << __E__;
		getDTC()->EnableLink(DTCLib::DTC_Link_CFO);
		getDTC()->DisableLink(DTCLib::DTC_Link_EVB);
		for(size_t i = 0; i < DTCLib::DTC_ROC_Links.size(); ++i)
		{
			if((roc_mask_ >> i) & 1)
				getDTC()->EnableLink(DTCLib::DTC_ROC_Links[i]);
			else
				getDTC()->DisableLink(DTCLib::DTC_ROC_Links[i]);
		}

		// getDTC()->SetROCDCSResponseTimer(1000); //Register removed as of Dec 2023 //set ROC DCS timeout (if 0, the DTC will hang forever when a ROC does not respond)
		getDTC()->EnableDCSReception();
		getDTC()->DisableCFOLoopback();  //allow passthrough of markers to next DTC

		__FE_COUT__ << "DTC reset links" << __E__;
		// getDTC()->ResetSERDESPLL(DTCLib::DTC_PLL_ID::DTC_PLL_CFO_RX);
		getDTC()->ResetSERDESRX(DTCLib::DTC_Link_ID::DTC_Link_ALL);
		getDTC()->ResetSERDESTX(DTCLib::DTC_Link_ID::DTC_Link_ALL);
		getDTC()->ResetSERDES(DTCLib::DTC_Link_ID::DTC_Link_ALL);

		usleep(100);
		getDTC()->SoftReset();  // soft reset to clear lock counters and errors
		break;
	default:
		__FE_COUT__ << "Do nothing while other configurable entities finish..." << __E__;
	}

}  // end configureForTimingChain()

//==============================================================================
void DTCFrontEndInterface::halt(void)
{
	const std::string transitionStr = "Halting";

	__FE_COUTV__(skipInit_);
	if(skipInit_)
		return;
	__FE_COUTV__(transitionStr);

	if(bufferTestThreadStruct_)
	{
		__FE_COUT__ << "Attempting to halt Buffer Test thread... " << __E__;

		// start mutex scope
		{
			std::lock_guard<std::mutex> lock(bufferTestThreadStruct_->lock_);
			bufferTestThreadStruct_->exitThread_ = true;
		}
	}

	if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_HARDWARE_DEV)
	{
		__FE_COUT_INFO__ << transitionStr << " for hardware development mode!" << __E__;

		getDTC()->DisableCFOEmulation();  //stop Event Window Marker generation
	}
	else if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_EVENT_BUILDING)
	{
		__FE_COUT_INFO__ << transitionStr << " for Event Building mode!" << __E__;
	}
	else if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_LOOPBACK)
	{
		__FE_COUT_INFO__ << transitionStr << " for Loopback mode!" << __E__;
		getDTC()->DisableCFOLoopback();
	}
	else
	{
		__FE_SS__ << "Unknown system operating mode: " << operatingMode_ << __E__
		          << " Please specify a valid operating mode in the 'Mu2eGlobalsTable.'"
		          << __E__;
		__FE_SS_THROW__;
	}

	for(auto& roc : rocs_)  // halt "as usual"
	{
		roc.second->halt();
	}

	__FE_COUT__ << "Halted." << __E__;

	// if(device_name_ == "DTC8")
	// {
	//	__FE_COUT__ << "Forcing abort" << __E__;
	//	abort();
	// }

	//	__FE_COUT__ << "HALT: DTC status" << __E__;
	//	readStatus();

	// if(runDataFile_.is_open())
	//	runDataFile_.close();
}  // end halt()

//==============================================================================
void DTCFrontEndInterface::pause(void)
{
	const std::string transitionStr = "Pausing";

	__FE_COUTV__(skipInit_);
	if(skipInit_)
		return;
	__FE_COUTV__(transitionStr);

	if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_HARDWARE_DEV)
	{
		__FE_COUT_INFO__ << transitionStr << " for hardware development mode!" << __E__;

		getDTC()->DisableCFOEmulation();  //stop Event Window Marker generation
	}
	else if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_EVENT_BUILDING)
	{
		__FE_COUT_INFO__ << transitionStr << " for Event Building mode!" << __E__;
	}
	else if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_LOOPBACK)
	{
		__FE_COUT_INFO__ << transitionStr << " for Loopback mode!" << __E__;
		getDTC()->DisableCFOLoopback();
	}
	else
	{
		__FE_SS__ << "Unknown system operating mode: " << operatingMode_ << __E__
		          << " Please specify a valid operating mode in the 'Mu2eGlobalsTable.'"
		          << __E__;
		__FE_SS_THROW__;
	}

	for(auto& roc : rocs_)  // pause "as usual"
	{
		roc.second->pause();
	}

	//	__FE_COUT__ << "PAUSE: DTC status" << __E__;
	//	readStatus();

	__FE_COUT__ << "Paused." << __E__;
}  //end pause()

//==============================================================================
void DTCFrontEndInterface::stop(void)
{
	const std::string transitionStr = "Stopping";

	__FE_COUTV__(skipInit_);
	if(skipInit_)
		return;
	__FE_COUTV__(transitionStr);

	if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_HARDWARE_DEV)
	{
		__FE_COUT_INFO__ << transitionStr << " for hardware development mode!" << __E__;

		getDTC()->DisableCFOEmulation();  //stop Event Window Marker generation
	}
	else if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_EVENT_BUILDING)
	{
		__FE_COUT_INFO__ << transitionStr << " for Event Building mode!" << __E__;
	}
	else if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_LOOPBACK)
	{
		__FE_COUT_INFO__ << transitionStr << " for Loopback mode!" << __E__;
		getDTC()->DisableCFOLoopback();
	}
	else
	{
		__FE_SS__ << "Unknown system operating mode: " << operatingMode_ << __E__
		          << " Please specify a valid operating mode in the 'Mu2eGlobalsTable.'"
		          << __E__;
		__FE_SS_THROW__;
	}

	if(emulatorMode_)
	{
		__FE_COUT__ << "Emulator DTC stopping... # of ROCs = " << rocs_.size() << __E__;
		for(auto& roc : rocs_)
			roc.second->stop();
	}
	else  //not emulating all ROCs
	{
		__FE_COUT__ << "DTC stopping... # of ROCs = " << rocs_.size() << __E__;
		for(auto& roc : rocs_)
			roc.second->stop();
	}

	return;

	// must close data file on each possible return with call 'if(runDataFile_.is_open())
	// runDataFile_.close();'

	// if(emulatorMode_)
	// {
	//	__FE_COUT__ << "Emulator DTC stopping... # of ROCs = " << rocs_.size() << __E__;
	//	for(auto& roc : rocs_)
	//		roc.second->stop();

	//	// if(runDataFile_.is_open())
	//	//	runDataFile_.close();
	//	return;
	// }

	// int numberOfCAPTANPulses =
	//     getConfigurationManager()
	//	   ->getNode("/Mu2eGlobalsTable/SyncDemoConfig/NumberOfCAPTANPulses")
	//	   .getValue<unsigned int>();

	// __FE_COUTV__(numberOfCAPTANPulses);

	// // int stopIndex = getIterationIndex();

	// if(numberOfCAPTANPulses == 0)
	// {
	//	for(auto& roc : rocs_)	// stop "as usual"
	//	{
	//		roc.second->stop();
	//	}
	//	// if(runDataFile_.is_open())
	//	//	runDataFile_.close();
	//	// return;
	// }

	// if(stopIndex == 0)
	// {
	//	//		int i = 0;
	//	for(auto& roc : rocs_)
	//	{
	//		// re-align link
	//		roc.second->writeRegister(22, 0);
	//		roc.second->writeRegister(22, 1);

	//		// std::stringstream filename;
	//		// filename << "/home/mu2edaq/sync_demo/ots/" << device_name_ << "_ROC"
	//		//	    << roc.second->getLinkID() << "data.txt";
	//		// std::string filenamestring = filename.str();
	//		// datafile_[i].open(filenamestring);
	//		//	i++;
	//	}
	// }

	// if(stopIndex > numberOfCAPTANPulses)
	// {
	//	int i = 0;
	//	for(auto& roc : rocs_)
	//	{
	//		__FE_COUT_INFO__ << ".... ROC" << roc.second->getLinkID() << "-DTC link lost "
	//					  << roc.second->readDTCLinkLossCounter()
	//					  << " times" << __E__;
	//		datafile_[i].close();
	//		i++;
	//	}
	//	if(runDataFile_.is_open())
	//		runDataFile_.close();
	//	return;
	// }

	// int i = 0;
	// __FE_COUT__ << "Entering read timestamp loop..." << __E__;
	// for(auto& roc : rocs_)
	// {
	//	int timestamp_data = roc.second->readInjectedPulseTimestamp();

	//	__FE_COUT__ << "Read " << stopIndex << " -> " << device_name_ << " timestamp "
	//		    << timestamp_data << __E__;

	//	datafile_[i] << stopIndex << " " << timestamp_data << std::endl;
	//	i++;
	// }

	// indicateIterationWork();
	// return;
}  // end stop()

//==============================================================================
void DTCFrontEndInterface::resume(void)
{
	const std::string transitionStr = "Resuming";

	__FE_COUTV__(skipInit_);
	if(skipInit_)
		return;
	__FE_COUTV__(transitionStr);

	__FE_COUTV__(operatingMode_);
	__FE_COUTV__(emulatorMode_);

	if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_HARDWARE_DEV)
	{
		__FE_COUT_INFO__ << transitionStr << " for hardware development mode!" << __E__;

		uint32_t numberOfEventWindowMarkers =
		    getConfigurationManager()
		        ->getNode("/Mu2eGlobalsTable/SyncDemoConfig/NumberOfCAPTANPulses")
		        .getValue<unsigned int>();
		__FE_COUT__
		    << "Using 'numberOfCAPTANPulses' for number of Event Windows to generate: "
		    << numberOfEventWindowMarkers << __E__;

		SetCFOEmulatorFixedWidthEmulation(
		    1,                           //bool enable,
		    false,                       //bool useDetachedBufferTest,
		    "0x44 clocks",               //std::string eventDuration,
		    numberOfEventWindowMarkers,  //uint32_t numberOfEventWindowMarkers,
		    0,                           //uint64_t initialEventWindowTag,
		    1,                           //uint64_t eventWindowMode,
		    0,                           //bool enableClockMarkers,
		    1,                           //bool enableAutogenDRP,
		    0,                           //bool saveBinaryDataToFile,
		    "Default",                   //filename
		    0,                           //bool saveSubeventHeadersToDataFile,
		    0,                           //bool doNotResetCounters
		    0,                           //bool skipBy32
		    0                            //unint32_t packetThresholdToSave )
		);
	}
	else if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_EVENT_BUILDING)
	{
		__FE_COUT_INFO__ << transitionStr << " for Event Building mode!" << __E__;
	}
	else if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_LOOPBACK)
	{
		__FE_COUT_INFO__ << transitionStr << " for Loopback mode!" << __E__;
		loopbackTest();
	}
	else
	{
		__FE_SS__ << "Unknown system operating mode: " << operatingMode_ << __E__
		          << " Please specify a valid operating mode in the 'Mu2eGlobalsTable.'"
		          << __E__;
		__FE_SS_THROW__;
	}

	for(auto& roc : rocs_)  // resume "as usual"
	{
		roc.second->resume();
	}

	//	__FE_COUT__ << "RESUME: DTC status" << __E__;
	//	readStatus();

	__FE_COUT__ << "Resumed." << __E__;
}  // end resume()

//==============================================================================
void DTCFrontEndInterface::start(std::string runNumber)
{
	const std::string transitionStr = "Starting";

	__FE_COUTV__(skipInit_);
	if(skipInit_)
		return;
	__FE_COUTV__(transitionStr);

	__FE_COUTV__(operatingMode_);
	__FE_COUTV__(emulatorMode_);

	if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_HARDWARE_DEV)
	{
		__FE_COUT_INFO__ << transitionStr << " for hardware development mode!" << __E__;

		uint32_t numberOfEventWindowMarkers =
		    getConfigurationManager()
		        ->getNode("/Mu2eGlobalsTable/SyncDemoConfig/NumberOfCAPTANPulses")
		        .getValue<unsigned int>();
		__FE_COUT__
		    << "Using 'numberOfCAPTANPulses' for number of Event Windows to generate: "
		    << numberOfEventWindowMarkers << __E__;

		if(numberOfEventWindowMarkers != uint32_t(0))
		{
			__FE_COUT__ << "Using 'numberOfCAPTANPulses' for number of Event Windows to "
			               "generate: "
			            << numberOfEventWindowMarkers << __E__;

			SetCFOEmulatorFixedWidthEmulation(
			    1,                           //bool enable,
			    false,                       //bool useDetachedBufferTest,
			    "0x44 clocks",               //std::string eventDuration,
			    numberOfEventWindowMarkers,  //uint32_t numberOfEventWindowMarkers,
			    0,                           //uint64_t initialEventWindowTag,
			    1,                           //uint64_t eventWindowMode,
			    0,                           //bool enableClockMarkers,
			    1,                           //bool enableAutogenDRP,
			    0,                           //bool saveBinaryDataToFile,
			    "Default",                   //filename
			    0,                           //bool saveSubeventHeadersToDataFile,
			    0,                           //bool doNotResetCounters
			    0,                           //bool skipBy32
			    0                            //unint32_t packetThresholdToSave)
			);
		}
		else
		{
			__FE_COUT__ << "'numberOfCAPTANPulses' set to 0, skipping "
			               "SetCFOEmulatorFixedWidthEmulation"
			            << __E__;
		}

		getDTC()->SoftReset();  //reset counters
	}
	else if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_EVENT_BUILDING)
	{
		__FE_COUT_INFO__ << transitionStr << " for Event Building mode!" << __E__;
		getDTC()->SoftReset();  //reset counters
		for(auto& roc : rocs_)
		{
			__FE_COUT__ << "Starting ROC " << __E__;
			roc.second->start(runNumber);
			__FE_COUT__ << "Done starting ROC" << __E__;
		}
	}
	else if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_LOOPBACK)
	{
		__FE_COUT_INFO__ << transitionStr << " for Loopback mode!" << __E__;
		// loopbackTest();
		return;
	}
	else
	{
		__FE_SS__ << "Unknown system operating mode: " << operatingMode_ << __E__
		          << " Please specify a valid operating mode in the 'Mu2eGlobalsTable.'"
		          << __E__;
		__FE_SS_THROW__;
	}

	return;

	// /////////////////////////////
	// /////////////////////////////
	// ///////////////////////////// old start
	// ///////////////////////////// old start
	// ///////////////////////////// old start
	// ///////////////////////////// old start
	// /////////////////////////////
	// /////////////////////////////

	// // open a file for this run number to write data to, if it hasn't been opened yet
	// // define a data file
	// if(!artdaqMode_)
	// {
	//	std::string runDataFilename = std::string(__ENV__("OTSDAQ_DATA")) + "/RunData_" +
	//				      runNumber + "_" + device_name_ + ".dat";

	//	__FE_COUTV__(runDataFilename);
	//	if(runDataFile_.is_open())
	//	{
	//		__SS__
	//		    << "File was left open! How was this possible -  open data file RunData: "
	//		    << runDataFilename << __E__;
	//		__SS_THROW__;
	//	}

	//	runDataFile_.open(runDataFilename, std::ios::out | std::ios::app);

	//	if(runDataFile_.fail())
	//	{
	//		__SS__ << "FAILED to open data file RunData: " << runDataFilename << __E__;
	//		__SS_THROW__;
	//	}

	// }  // end local run file creation

	// if(emulatorMode_)
	// {
	//	__FE_COUT__ << "Emulator DTC starting... # of ROCs = " << rocs_.size() << __E__;
	//	for(auto& roc : rocs_)
	//	{
	//		__FE_COUT__ << "Starting ROC ";
	//		roc.second->start(runNumber);
	//		__FE_COUT__ << "Done starting ROC";
	//	}
	//	return;
	// }

	// int numberOfLoopbacks =
	//     getConfigurationManager()
	//	   ->getNode("/Mu2eGlobalsTable/SyncDemoConfig/NumberOfLoopbacks")
	//	   .getValue<unsigned int>();

	// __FE_COUTV__(numberOfLoopbacks);

	// // int stopIndex = getIterationIndex();

	// if(numberOfLoopbacks == 0)
	// {
	//	for(auto& roc : rocs_)	// start "as usual"
	//	{
	//		roc.second->start(runNumber);
	//	}
	//	return;
	// }

	// __FE_COUT_INFO__ << device_name_ << " Ignoring loopback for now..." << __E__;
	// return;  // for now ignore loopback mode

	// const int numberOfChains = 1;
	// // int	link[numberOfChains] = {0};

	// const int numberOfDTCsPerChain = 1;

	// const int numberOfROCsPerDTC = 1;  // assume these are ROC0 and ROC1

	// // To do loopbacks on all CFOs, first have to setup all DTCs, then the CFO
	// // (this method) work per iteration.	 Loop back done on all chains (in this
	// // method), assuming the following order: i	DTC0  DTC1  ...	 DTCN 0	 ROC0
	// // none  ...	 none 1	 ROC1  none  ...  none 2  none	ROC0  ...  none 3  none
	// // ROC1  ...	 none
	// // ...
	// // N-1  none	 none  ...  ROC0
	// // N	 none  none  ...  ROC1

	// int totalNumberOfMeasurements =
	//     numberOfChains * numberOfDTCsPerChain * numberOfROCsPerDTC;

	// int loopbackIndex = getIterationIndex();

	// if(loopbackIndex == 0)  // start
	// {
	//	initial_9100_ = registerRead(0x9100);
	//	initial_9114_ = registerRead(0x9114);
	//	indicateIterationWork();
	//	return;
	// }

	// if(loopbackIndex > totalNumberOfMeasurements)  // finish
	// {
	//	__FE_COUT_INFO__ << device_name_ << " loopback DONE" << __E__;

	//	if(checkLinkStatus() == 1)
	//	{
	//		//	__FE_COUT_INFO__ << device_name_ << " links OK 0x" << std::hex <<
	//		//	registerRead(0x9140) << std::dec << __E__;
	//	}
	//	else
	//	{
	//		//	__FE_COUT_INFO__ << device_name_ << " links not OK 0x" << std::hex <<
	//		//	registerRead(0x9140) << std::dec << __E__;
	//	}

	//	if(0)
	//		for(auto& roc : rocs_)
	//		{
	//			__FE_COUT_INFO__ << ".... ROC" << roc.second->getLinkID() << "-DTC link lost "
	//						  << roc.second->readDTCLinkLossCounter()
	//						  << " times";
	//		}

	//	registerWrite(0x9100, initial_9100_);
	//	registerWrite(0x9114, initial_9114_);

	//	return;
	// }

	// //=========== Perform loopback=============

	// // where are we in the procedure?
	// unsigned int activeROC = (loopbackIndex - 1) % numberOfROCsPerDTC;

	// int activeDTC = -1;

	// for(int nDTC = 0; nDTC < numberOfDTCsPerChain; nDTC++)
	// {
	//	if((loopbackIndex - 1) >= (nDTC * numberOfROCsPerDTC) &&
	//	   (loopbackIndex - 1) < ((nDTC + 1) * numberOfROCsPerDTC))
	//	{
	//		activeDTC = nDTC;
	//	}
	// }

	// // __FE_COUT__ << "loopback index = " << loopbackIndex
	// //	<< " activeDTC = " << activeDTC
	// //		<< " activeROC = " << activeROC
	// //		<< __E__;

	// if(activeDTC == dtc_location_in_chain_)
	// {
	//	__FE_COUT__ << "DTC" << activeDTC << "loopback mode ENABLE" << __E__;
	//	int dataInReg	= registerRead(0x9100);
	//	int dataToWrite = dataInReg & 0xefffffff;  // bit 28 = 0
	//	registerWrite(0x9100, dataToWrite);
	// }
	// else
	// {
	//	// this DTC is lower in chain than the one being looped.  Pass the loopback
	//	// signal through
	//	__FE_COUT__ << "active DTC = " << activeDTC
	//		    << " is NOT this DTC = " << dtc_location_in_chain_
	//		    << "... pass signal through" << __E__;

	//	int dataInReg	= registerRead(0x9100);
	//	int dataToWrite = dataInReg | 0x10000000;  // bit 28 = 1
	//	registerWrite(0x9100, dataToWrite);
	// }

	// int ROCToEnable =
	//     0x00004040 |
	//     (0x101 << activeROC);  // enables TX and Rx to CFO (bit 6) and appropriate ROC
	// __FE_COUT__ << "enable ROC " << activeROC << " --> 0x" << std::hex << ROCToEnable
	//	       << std::dec << __E__;

	// registerWrite(0x9114, ROCToEnable);

	// indicateIterationWork();  // FIXME -- go back to including the ROC (could not 'read'
	//			     // for some reason)
	// return;
	// // Re-align the links for the activeROC
	// for(auto& roc : rocs_)
	// {
	//	if(roc.second->getLinkID() == activeROC)
	//	{
	//		__FE_COUT__ << "... ROC realign link... " << __E__;
	//		roc.second->writeRegister(22, 0);
	//		roc.second->writeRegister(22, 1);
	//	}
	// }

	// indicateIterationWork();
	// return;
}  // end start()

//==============================================================================
// return true to keep running
bool DTCFrontEndInterface::running(void)
{
	__FE_COUTV__(skipInit_);
	if(skipInit_)
		return false;

	__FE_COUTV__(operatingMode_);
	__FE_COUTV__(emulatorMode_);

	if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_HARDWARE_DEV)
	{
		__FE_COUT_INFO__ << "Running for hardware development mode!" << __E__;
	}
	else if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_EVENT_BUILDING)
	{
		__FE_COUT_INFO__ << "Running for Event Building mode!" << __E__;
	}
	else if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_LOOPBACK)
	{
		__FE_COUT_INFO__ << "Running for Loopback mode!" << __E__;
	}
	else
	{
		__FE_SS__ << "Unknown system operating mode: " << operatingMode_ << __E__
		          << " Please specify a valid operating mode in the 'Mu2eGlobalsTable.'"
		          << __E__;
		__FE_SS_THROW__;
	}

	bool stillRunning = false;
	for(auto& roc : rocs_)
		stillRunning = stillRunning || roc.second->running();

	__FE_COUTV__(stillRunning);

	return stillRunning;

	// /////////////////////////////
	// /////////////////////////////
	// ///////////////////////////// old running
	// ///////////////////////////// old running
	// ///////////////////////////// old running
	// ///////////////////////////// old running
	// /////////////////////////////
	// /////////////////////////////

	// //	if(artdaqMode_) {
	// //	  __FE_COUT__ << "Running in artdaqmode" << __E__;
	// return true;
	// //	}
	// if(emulatorMode_)
	// {
	//	__FE_COUT__ << "Emulator DTC running... # of ROCs = " << rocs_.size() << __E__;
	//	bool stillRunning = false;
	//	for(auto& roc : rocs_)
	//		stillRunning = stillRunning || roc.second->running();

	//	return stillRunning;
	// }

	// // first setup DTC and CFO.	This is stolen from "getheartbeatanddatarequest"

	// //	auto start = DTCLib::DTC_EventWindowTag(static_cast<uint64_t>(timestampStart));

	// //	 std::time_t current_time;

	// bool incrementTimestamp = true;

	// uint32_t cfodelay = 10000;  // have no idea what this is, but 1000 didn't work (don't
	//			       // know if 10000 works, either)
	// int		requestsAhead  = 0;
	// unsigned int number	       = -1;  // largest number of events?
	// unsigned int timestampStart = 0;

	// auto device = getDTC()->GetDevice();
	// // auto initTime = device->GetDeviceTime();
	// device->ResetDeviceTime();
	// // auto afterInit = std::chrono::steady_clock::now();

	// if(emulate_cfo_ == 1)
	// {
	//	registerWrite(
	//	    0x9100, 0x40008404);  // bit 30 = CFO emulation enable, bit 15 = CFO
	//				  // emulation mode, bit 2 = DCS enable
	//				  // bit 10 turns off retry which isn't working right now
	//	sleep(1);

	//	// set number of null heartbeats
	//	// registerWrite(0x91BC, 0x0);
	//	registerWrite(0x91BC, 0x10);  // new incantaton from Rick K. 12/18/2019
	//	//	  sleep(1);

	//	// # Send data
	//	// #disable 40mhz marker
	//	registerWrite(0x91f4, 0x0);
	//	//	  sleep(1);

	//	// #set num dtcs
	//	registerWrite(0x9158, 0x1);
	//	//	  sleep(1);

	//	bool	 useSWCFOEmulator = true;
	//	uint16_t debugPacketCount = 0;
	//	auto	 debugType	  = DTCLib::DTC_DebugType_SpecialSequence;
	//	bool	 stickyDebugType  = true;
	//	bool	 quiet		  = false;
	//	bool	 asyncRR	  = false;
	//	bool	 forceNoDebugMode = true;

	//	DTCLib::DTCSoftwareCFO* EmulatedCFO_ =
	//	    new DTCLib::DTCSoftwareCFO(getDTC(),
	//				       useSWCFOEmulator,
	//				       debugPacketCount,
	//				       debugType,
	//				       stickyDebugType,
	//				       quiet,
	//				       asyncRR,
	//				       forceNoDebugMode);

	//	EmulatedCFO_->SendRequestsForRange(
	//	    number,
	//	    DTCLib::DTC_EventWindowTag(static_cast<uint64_t>(timestampStart)),
	//	    incrementTimestamp,
	//	    cfodelay,
	//	    requestsAhead);

	//	// auto readoutRequestTime = device->GetDeviceTime();
	//	device->ResetDeviceTime();
	//	// auto afterRequests = std::chrono::steady_clock::now();
	// }

	// while(WorkLoop::continueWorkLoop_)
	// {
	//	registerWrite(
	//	    0x9100, 0x40008404);  // bit 30 = CFO emulation enable, bit 15 = CFO
	//				  // emulation mode, bit 2 = DCS enable
	//				  // bit 10 turns off retry which isn't working right now
	//	sleep(1);

	//	// set number of null heartbeats
	//	// registerWrite(0x91BC, 0x0);
	//	registerWrite(0x91BC, 0x10);  // new incantaton from Rick K. 12/18/2019
	//	//	  sleep(1);

	//	// # Send data
	//	// #disable 40mhz marker
	//	registerWrite(0x91f4, 0x0);
	//	//	  sleep(1);

	//	// #set num dtcs
	//	registerWrite(0x9158, 0x1);
	//	//	  sleep(1);

	//	bool	 useSWCFOEmulator = true;
	//	uint16_t debugPacketCount = 0;
	//	auto	 debugType	  = DTCLib::DTC_DebugType_SpecialSequence;
	//	bool	 stickyDebugType  = true;
	//	bool	 quiet		  = false;
	//	bool	 asyncRR	  = false;
	//	bool	 forceNoDebugMode = true;

	//	DTCLib::DTCSoftwareCFO* EmulatedCFO_ =
	//	    new DTCLib::DTCSoftwareCFO(getDTC(),
	//				       useSWCFOEmulator,
	//				       debugPacketCount,
	//				       debugType,
	//				       stickyDebugType,
	//				       quiet,
	//				       asyncRR,
	//				       forceNoDebugMode);

	//	EmulatedCFO_->SendRequestsForRange(
	//	    number,
	//	    DTCLib::DTC_EventWindowTag(static_cast<uint64_t>(timestampStart)),
	//	    incrementTimestamp,
	//	    cfodelay,
	//	    requestsAhead);

	//	// auto readoutRequestTime = device->GetDeviceTime();
	//	device->ResetDeviceTime();
	//	// auto afterRequests = std::chrono::steady_clock::now();
	// }

	// while(WorkLoop::continueWorkLoop_)
	// {
	//	for(auto& roc : rocs_)
	//	{
	//		roc.second->running();
	//	}

	//	// print out stuff
	//	unsigned quietCount = 20;
	//	bool	 quiet	    = false;

	//	std::stringstream ostr;
	//	ostr << std::endl;

	//	//		std::cout << "Buffer Read " << std::dec << ii << std::endl;
	//	mu2e_databuff_t* buffer;
	//	auto		 tmo_ms = 1500;
	//	__FE_COUT__ << "util - before read for DAQ in running";
	//	auto sts = device->read_data(
	//	    DTC_DMA_Engine_DAQ, reinterpret_cast<void**>(&buffer), tmo_ms);
	//	__FE_COUT__ << "util - after read for DAQ in running "
	//		    << " sts=" << sts << ", buffer=" << (void*)buffer;

	//	if(sts > 0)
	//	{
	//		void* readPtr = &buffer[0];
	//		auto  bufSize = static_cast<uint16_t>(*static_cast<uint64_t*>(readPtr));
	//		readPtr	      = static_cast<uint8_t*>(readPtr) + 8;

	//		__FE_COUT__ << "Buffer reports DMA size of " << std::dec << bufSize
	//			    << " bytes. Device driver reports read of " << sts << " bytes,"
	//			    << std::endl;

	//		__FE_COUT__ << "util - bufSize is " << bufSize;
	//		outputStream.write(static_cast<char*>(readPtr), sts - 8);
	//		auto maxLine = static_cast<unsigned>(ceil((sts - 8) / 16.0));
	//		__FE_COUT__ << "maxLine " << maxLine;
	//		for(unsigned line = 0; line < maxLine; ++line)
	//		{
	//			ostr << "0x" << std::hex << std::setw(5) << std::setfill('0') << line
	//			     << "0: ";
	//			for(unsigned byte = 0; byte < 8; ++byte)
	//			{
	//				if(line * 16 + 2 * byte < sts - 8u)
	//				{
	//					auto thisWord =
	//					    reinterpret_cast<uint16_t*>(buffer)[4 + line * 8 + byte];
	//					ostr << std::setw(4) << static_cast<int>(thisWord) << " ";
	//				}
	//			}

	//			ostr << std::endl;
	//			//	std::cout << ostr.str();

	//			//     __SET_ARG_OUT__("readData", ostr.str());	 // write to data file

	//			// don't write data to the log file, only the data file
	//			// __FE_COUT__ << ostr.str();

	//			if(maxLine > quietCount * 2 && quiet && line == (quietCount - 1))
	//			{
	//				line =
	//				    static_cast<unsigned>(ceil((sts - 8) / 16.0)) - (1 + quietCount);
	//			}
	//		}
	//	}
	//	device->read_release(DTC_DMA_Engine_DAQ, 1);

	//	ostr << std::endl;

	//	if(runDataFile_.is_open())
	//	{
	//		runDataFile_ << ostr.str();
	//		runDataFile_.flush();  // flush to disk
	//	}
	//	//__FE_COUT__ << ostr.str();

	//	delete EmulatedCFO_;

	//	break;
	// }
	// return true;
}  // end running()

//==============================================================================
// rocRead
void DTCFrontEndInterface::ReadROC(__ARGS__)
{
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	uint32_t rocLinkIndexVal = __GET_ARG_IN__(
	    "Target ROC or Mask (Default = -1 := all ROCs, or 0x111111 := all)",
	    uint32_t,
	    -1 /* ALL */);
	bool usingRocMask = false;
	if(rocLinkIndexVal != uint32_t(-1) && rocLinkIndexVal > 5)
	{
		usingRocMask = true;
		__FE_COUT__ << "Using ROC Link Mask value: 0x" << std::hex
		            << (unsigned int)rocLinkIndexVal << std::dec << __E__;
	}

	DTCLib::DTC_Link_ID rocLinkIndex =
	    DTCLib::DTC_Link_ID(usingRocMask ? -1 : rocLinkIndexVal);
	__FE_COUT__ << "rocLinkIndexVal = 0x" << std::hex << rocLinkIndexVal << __E__;
	__FE_COUTV__(usingRocMask);
	__FE_COUTV__(rocLinkIndex);

	DTCLib::roc_address_t address = __GET_ARG_IN__("address", DTCLib::roc_address_t);
	__FE_COUTV__((unsigned int)address);

	DTCLib::roc_data_t readData = -999;

	bool        found = false;
	std::string result;
	for(auto& roc : rocs_)
	{
		if(usingRocMask)
			__FE_COUTT__ << "0x" << std::hex << (1 << (int(roc.second->getLinkID()) * 4))
			             << " vs rocLinkIndexVal = 0x" << std::hex << rocLinkIndexVal
			             << __E__;
		else
			__FE_COUTT__ << "Found link ID " << roc.second->getLinkID() << " looking for "
			             << rocLinkIndex << __E__;

		if((!usingRocMask &&  //use ROC index
		    (rocLinkIndex == DTCLib::DTC_Link_ID::DTC_Link_ALL ||
		     roc.second->getLinkID() == rocLinkIndex)) ||
		   (usingRocMask &&  //use ROC mask
		    ((1 << (int(roc.second->getLinkID()) * 4)) & rocLinkIndexVal)))
		{
			found = true;
			__FE_COUTT__ << "Doing " << roc.second->getLinkID() << __E__;
			try  //give user feedback on ROC status if exception caught
			{
				if(emulatorMode_)
				{
					readData = roc.second->readRegister(address);
				}
				else
				{
					readData =
					    getDTC()->ReadROCRegister(roc.second->getLinkID(), address, 300);
				}
			}
			catch(...)
			{
				__SS__ << "Error during ROC read of link " << roc.second->getLinkID()
				       << " - check that the ROC is enabled and ready; here is the DTC "
				          "ROC setup: "
				       << getDTC()->FormattedRegDump(
				              0, getDTC()->formattedROCEmulationFunctions_)
				       << __E__;
				try
				{
					throw;
				}
				catch(const std::runtime_error& e)
				{
					ss << "\nHere was the error: " << e.what() << __E__;
				}
				catch(const std::exception& e)
				{
					ss << "\nHere was the error: " << e.what() << __E__;
				}
				__SS_THROW__;
			}

			char readDataStr[100];
			sprintf(readDataStr, "0x%x", readData);
			if(result.size())
				result += ", ";
			else  //init
			{
				std::stringstream ss;
				ss << "Reading ROC Address " << address << "(0x" << std::hex
				   << (unsigned int)address << ") for ROC(s):\n";
				result = ss.str();
			}
			if(rocLinkIndex == DTC_Link_ALL || usingRocMask)
				result += "(" +
				          std::to_string(static_cast<uint8_t>(roc.second->getLinkID())) +
				          ") ";
			result += readDataStr;

			__FE_COUT__ << "readData"
			            << ": 0x" << std::hex << readData << std::dec << __E__;
		}
	}  //end roc exec loop

	if(found)
	{
		__SET_ARG_OUT__("readData", result);
		return;
	}

	__FE_SS__ << "Target ROC or Mask 0x" << std::hex << rocLinkIndexVal << " not found!"
	          << __E__;
	__FE_SS_THROW__;
}  // end ReadROC()

//==============================================================================
// DTCStatus
//	FEMacro 'DTCStatus' generated, Oct-22-2018 03:16:46, by 'admin' using
// MacroMaker.	Macro Notes:
void DTCFrontEndInterface::WriteROC(__ARGS__)
{
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	uint32_t rocLinkIndexVal = __GET_ARG_IN__(
	    "Target ROC or Mask (Default = -1 := all ROCs, or 0x111111 := all)",
	    uint32_t,
	    -1 /* ALL */);
	bool usingRocMask = false;
	if(rocLinkIndexVal != uint32_t(-1) && rocLinkIndexVal > 5)
	{
		usingRocMask = true;
		__FE_COUT__ << "Using ROC Link Mask value: 0x" << std::hex
		            << (unsigned int)rocLinkIndexVal << std::dec << __E__;
	}

	DTCLib::DTC_Link_ID rocLinkIndex =
	    DTCLib::DTC_Link_ID(usingRocMask ? -1 : rocLinkIndexVal);
	__FE_COUT__ << "rocLinkIndexVal = 0x" << std::hex << rocLinkIndexVal << __E__;
	__FE_COUTV__(usingRocMask);
	__FE_COUTV__(rocLinkIndex);

	DTCLib::roc_address_t address   = __GET_ARG_IN__("address", DTCLib::roc_address_t);
	DTCLib::roc_data_t    writeData = __GET_ARG_IN__("writeData", DTCLib::roc_data_t);

	__FE_COUTV__((unsigned int)address);
	__FE_COUTV__(writeData);

	__FE_COUT__ << "ROCs size = " << rocs_.size() << __E__;

	std::string result;
	bool        found = false;
	for(auto& roc : rocs_)
	{
		if(usingRocMask)
			__FE_COUTT__ << "0x" << std::hex << (1 << (int(roc.second->getLinkID()) * 4))
			             << " vs rocLinkIndexVal = 0x" << std::hex << rocLinkIndexVal
			             << __E__;
		else
			__FE_COUTT__ << "Found link ID " << roc.second->getLinkID() << " looking for "
			             << rocLinkIndex << __E__;

		if((!usingRocMask &&  //use ROC index
		    (rocLinkIndex == DTCLib::DTC_Link_ID::DTC_Link_ALL ||
		     roc.second->getLinkID() == rocLinkIndex)) ||
		   (usingRocMask &&  //use ROC mask
		    ((1 << (int(roc.second->getLinkID()) * 4)) & rocLinkIndexVal)))
		{
			found = true;
			__FE_COUTT__ << "Doing " << roc.second->getLinkID() << __E__;
			roc.second->writeRegister(address, writeData);

			if(result.size())
				result += ", ";
			else  //init
			{
				std::stringstream ss;
				ss << "Wrote Data " << writeData << "(0x " << std::hex
				   << (unsigned int)writeData << std::dec << ") to ROC Address "
				   << address << "(0x" << std::hex << (unsigned int)address
				   << ") for ROC(s): ";
				result = ss.str();
			}

			if(rocLinkIndex == DTC_Link_ALL || usingRocMask)
				result += "(" +
				          std::to_string(static_cast<uint8_t>(roc.second->getLinkID())) +
				          ")";
		}
	}  //end roc exec loop

	if(found)
	{
		__SET_ARG_OUT__("Result", result);
		return;
	}

	__FE_SS__ << "Target ROC or Mask 0x" << std::hex << rocLinkIndexVal << " not found!"
	          << __E__;
	__FE_SS_THROW__;
}  // end WriteROC()

//==============================================================================
void DTCFrontEndInterface::WriteExternalROCRegister(__ARGS__)
{
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	// macro commands section

	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;

	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	uint32_t rocLinkIndexVal = __GET_ARG_IN__(
	    "Target ROC or Mask (Default = -1 := all ROCs, or 0x111111 := all)",
	    uint32_t,
	    -1 /* ALL */);
	bool usingRocMask = false;
	if(rocLinkIndexVal != uint32_t(-1) && rocLinkIndexVal > 5)
	{
		usingRocMask = true;
		__FE_COUT__ << "Using ROC Link Mask value: 0x" << std::hex
		            << (unsigned int)rocLinkIndexVal << std::dec << __E__;
	}

	DTCLib::DTC_Link_ID rocLinkIndex =
	    DTCLib::DTC_Link_ID(usingRocMask ? -1 : rocLinkIndexVal);
	__FE_COUT__ << "rocLinkIndexVal = 0x" << std::hex << rocLinkIndexVal << __E__;
	__FE_COUTV__(usingRocMask);
	__FE_COUTV__(rocLinkIndex);

	DTCLib::roc_address_t address   = __GET_ARG_IN__("address", DTCLib::roc_address_t);
	DTCLib::roc_data_t    writeData = __GET_ARG_IN__("writeData", DTCLib::roc_data_t);
	DTCLib::roc_address_t block     = __GET_ARG_IN__("block", DTCLib::roc_address_t);
	__FE_COUT__ << "block = " << std::dec << (unsigned int)block << __E__;
	__FE_COUT__ << "address = 0x" << std::hex << (unsigned int)address << std::dec
	            << __E__;
	__FE_COUT__ << "writeData = 0x" << std::hex << writeData << std::dec << __E__;

	bool acknowledge_request = false;

	std::string result;
	bool        found = false;
	for(auto& roc : rocs_)
	{
		if(usingRocMask)
			__FE_COUTT__ << "0x" << std::hex << (1 << (int(roc.second->getLinkID()) * 4))
			             << " vs rocLinkIndexVal = 0x" << std::hex << rocLinkIndexVal
			             << __E__;
		else
			__FE_COUTT__ << "Found link ID " << roc.second->getLinkID() << " looking for "
			             << rocLinkIndex << __E__;

		if((!usingRocMask &&  //use ROC index
		    (rocLinkIndex == DTCLib::DTC_Link_ID::DTC_Link_ALL ||
		     roc.second->getLinkID() == rocLinkIndex)) ||
		   (usingRocMask &&  //use ROC mask
		    ((1 << (int(roc.second->getLinkID()) * 4)) & rocLinkIndexVal)))
		{
			found = true;
			__FE_COUTT__ << "Doing " << roc.second->getLinkID() << __E__;
			getDTC()->WriteExtROCRegister(roc.second->getLinkID(),
			                              block,
			                              address,
			                              writeData,
			                              acknowledge_request,
			                              0);

			if(result.size())
				result += ", ";
			else  //init
			{
				std::stringstream ss;
				ss << "Wrote Data " << writeData << "(0x " << std::hex
				   << (unsigned int)writeData << std::dec << ") to ROC external Block "
				   << block << "(0x" << std::hex << (unsigned int)block << std::dec
				   << ") and Address " << address << "(0x" << std::hex
				   << (unsigned int)address << ") for ROC(s): ";
				result = ss.str();
			}

			if(rocLinkIndex == DTC_Link_ALL || usingRocMask)
				result += "(" +
				          std::to_string(static_cast<uint8_t>(roc.second->getLinkID())) +
				          ")";
		}
	}  //end roc exec loop

	if(found)
	{
		__SET_ARG_OUT__("Result", result);
		return;
	}

	__FE_SS__ << "Target ROC or Mask 0x" << std::hex << rocLinkIndexVal << " not found!"
	          << __E__;
	__FE_SS_THROW__;
}  // end WriteExternalROCRegister()

//==============================================================================
void DTCFrontEndInterface::ReadExternalROCRegister(__ARGS__)
{
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	uint32_t rocLinkIndexVal = __GET_ARG_IN__(
	    "Target ROC or Mask (Default = -1 := all ROCs, or 0x111111 := all)",
	    uint32_t,
	    -1 /* ALL */);
	bool usingRocMask = false;
	if(rocLinkIndexVal != uint32_t(-1) && rocLinkIndexVal > 5)
	{
		usingRocMask = true;
		__FE_COUT__ << "Using ROC Link Mask value: 0x" << std::hex
		            << (unsigned int)rocLinkIndexVal << std::dec << __E__;
	}

	DTCLib::DTC_Link_ID rocLinkIndex =
	    DTCLib::DTC_Link_ID(usingRocMask ? -1 : rocLinkIndexVal);
	__FE_COUT__ << "rocLinkIndexVal = 0x" << std::hex << rocLinkIndexVal << __E__;
	__FE_COUTV__(usingRocMask);
	__FE_COUTV__(rocLinkIndex);

	DTCLib::roc_address_t address = __GET_ARG_IN__("address", DTCLib::roc_address_t);
	DTCLib::roc_address_t block   = __GET_ARG_IN__("block", DTCLib::roc_address_t);
	__FE_COUT__ << "block = " << std::dec << (unsigned int)block << __E__;
	__FE_COUT__ << "address = 0x" << std::hex << (unsigned int)address << std::dec
	            << __E__;

	// bool acknowledge_request = false;

	bool        found  = false;
	std::string result = "";
	for(auto& roc : rocs_)
	{
		if(usingRocMask)
			__FE_COUTT__ << "0x" << std::hex << (1 << (int(roc.second->getLinkID()) * 4))
			             << " vs rocLinkIndexVal = 0x" << std::hex << rocLinkIndexVal
			             << __E__;
		else
			__FE_COUTT__ << "Found link ID " << roc.second->getLinkID() << " looking for "
			             << rocLinkIndex << __E__;

		if((!usingRocMask &&  //use ROC index
		    (rocLinkIndex == DTCLib::DTC_Link_ID::DTC_Link_ALL ||
		     roc.second->getLinkID() == rocLinkIndex)) ||
		   (usingRocMask &&  //use ROC mask
		    ((1 << (int(roc.second->getLinkID()) * 4)) & rocLinkIndexVal)))
		{
			found = true;
			__FE_COUTT__ << "Doing " << roc.second->getLinkID() << __E__;
			DTCLib::roc_data_t readData;

			readData = getDTC()->ReadExtROCRegister(rocLinkIndex, block, address);

			std::string readDataString = "";
			readDataString = BinaryStringMacros::binaryNumberToHexString(readData);

			if(result.size())
				result += ", ";
			else  //init
			{
				std::stringstream ss;
				ss << "Reading from ROC external Block " << block << "(0x" << std::hex
				   << (unsigned int)block << std::dec << ") and Address " << address
				   << "(0x" << std::hex << (unsigned int)address << ") for ROC(s):\n";
				result = ss.str();
			}
			if(rocLinkIndex == DTC_Link_ALL || usingRocMask)
				result += "(" +
				          std::to_string(static_cast<uint8_t>(roc.second->getLinkID())) +
				          ") ";
			result += readDataString;

			__FE_COUT__ << "readData"
			            << ": " << readDataString << __E__;
		}
	}  //end roc exec loop

	if(found)
	{
		__SET_ARG_OUT__("readData", result);
		return;
	}

	__FE_SS__ << "Target ROC or Mask 0x" << std::hex << rocLinkIndexVal << " not found!"
	          << __E__;
	__FE_SS_THROW__;
}  // end ReadExternalROCRegister()

//========================================================================
void DTCFrontEndInterface::BlockReadROC(__ARGS__)
{
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	// macro commands section
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;

	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	uint32_t rocLinkIndexVal = __GET_ARG_IN__(
	    "Target ROC or Mask (Default = -1 := all ROCs, or 0x111111 := all)",
	    uint32_t,
	    -1 /* ALL */);
	bool usingRocMask = false;
	if(rocLinkIndexVal != uint32_t(-1) && rocLinkIndexVal > 5)
	{
		usingRocMask = true;
		__FE_COUT__ << "Using ROC Link Mask value: 0x" << std::hex
		            << (unsigned int)rocLinkIndexVal << std::dec << __E__;
	}

	DTCLib::DTC_Link_ID rocLinkIndex =
	    DTCLib::DTC_Link_ID(usingRocMask ? -1 : rocLinkIndexVal);
	__FE_COUT__ << "rocLinkIndexVal = 0x" << std::hex << rocLinkIndexVal << __E__;
	__FE_COUTV__(usingRocMask);
	__FE_COUTV__(rocLinkIndex);

	DTCLib::roc_address_t address = __GET_ARG_IN__("address", DTCLib::roc_address_t);
	uint16_t              wordCount =
	    __GET_ARG_IN__("Number Of 16-bit words to Read (Default := 8)", uint16_t, 8);
	bool incrementAddress = __GET_ARG_IN__("incrementAddress (Default := false)", bool);

	__FE_COUT__ << "address = 0x" << std::hex << (unsigned int)address << std::dec
	            << __E__;
	__FE_COUT__ << "numberOfWords = " << std::dec << (unsigned int)wordCount << __E__;
	__FE_COUTV__(incrementAddress);

	bool        found  = false;
	std::string result = "";
	for(auto& roc : rocs_)
	{
		if(usingRocMask)
			__FE_COUTT__ << "0x" << std::hex << (1 << (int(roc.second->getLinkID()) * 4))
			             << " vs rocLinkIndexVal = 0x" << std::hex << rocLinkIndexVal
			             << __E__;
		else
			__FE_COUTT__ << "Found link ID " << roc.second->getLinkID() << " looking for "
			             << rocLinkIndex << __E__;

		if((!usingRocMask &&  //use ROC index
		    (rocLinkIndex == DTCLib::DTC_Link_ID::DTC_Link_ALL ||
		     roc.second->getLinkID() == rocLinkIndex)) ||
		   (usingRocMask &&  //use ROC mask
		    ((1 << (int(roc.second->getLinkID()) * 4)) & rocLinkIndexVal)))
		{
			found = true;
			__FE_COUTT__ << "Doing " << roc.second->getLinkID() << __E__;
			std::vector<DTCLib::roc_data_t> readData;

			roc.second->readBlock(readData, address, wordCount, incrementAddress);

			std::string readDataString = "";
			{
				for(size_t i = 0; i < readData.size(); ++i)
				{
					if(i)
						readDataString += ", ";
					if(i && i % 8 == 0)
						readDataString += "\n\t";
					readDataString +=
					    BinaryStringMacros::binaryNumberToHexString(readData[i]);
				}
			}

			if(result.size())
				result += "";
			else  //init
			{
				std::stringstream ss;
				ss << "Block Read from ROC Address " << address << "(0x" << std::hex
				   << (unsigned int)address << ") for ROC(s):\n";
				result = ss.str();
			}
			if(rocLinkIndex == DTC_Link_ALL || usingRocMask)
				result += "(" +
				          std::to_string(static_cast<uint8_t>(roc.second->getLinkID())) +
				          ") ";
			result += readDataString + "\n";

			__FE_COUT__ << "readData"
			            << ": " << readDataString << __E__;
		}
	}  //end roc exec loop

	if(found)
	{
		__SET_ARG_OUT__("readData", result);
		return;
	}

	__FE_SS__ << "Target ROC or Mask 0x" << std::hex << rocLinkIndexVal << " not found!"
	          << __E__;
	__FE_SS_THROW__;

}  // end BlockReadROC()

//========================================================================
void DTCFrontEndInterface::BlockWriteROC(__ARGS__)
{
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	// macro commands section
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;

	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	uint32_t rocLinkIndexVal = __GET_ARG_IN__(
	    "Target ROC or Mask (Default = -1 := all ROCs, or 0x111111 := all)",
	    uint32_t,
	    -1 /* ALL */);
	bool usingRocMask = false;
	if(rocLinkIndexVal != uint32_t(-1) && rocLinkIndexVal > 5)
	{
		usingRocMask = true;
		__FE_COUT__ << "Using ROC Link Mask value: 0x" << std::hex
		            << (unsigned int)rocLinkIndexVal << std::dec << __E__;
	}

	DTCLib::DTC_Link_ID rocLinkIndex =
	    DTCLib::DTC_Link_ID(usingRocMask ? -1 : rocLinkIndexVal);
	__FE_COUT__ << "rocLinkIndexVal = 0x" << std::hex << rocLinkIndexVal << __E__;
	__FE_COUTV__(usingRocMask);
	__FE_COUTV__(rocLinkIndex);

	DTCLib::roc_address_t address     = __GET_ARG_IN__("address", DTCLib::roc_address_t);
	std::string           writeDataIn = __GET_ARG_IN__(
        "writeData (CSV-literal or CSV-filename of 16-bit words, or keyword + parameter "
	              "'AUTOGENERATE count')",
        std::string);
	bool incrementAddress = __GET_ARG_IN__("incrementAddress (Default = false)", bool);
	bool requestAck       = __GET_ARG_IN__("requestAck (Default = false)", bool);
	std::vector<DTCLib::roc_data_t> writeData;

	__FE_COUT__ << "address = 0x" << std::hex << (unsigned int)address << std::dec
	            << __E__;
	__FE_COUTV__(incrementAddress);
	__FE_COUTV__(requestAck);

	if(writeDataIn.find("AUTOGENERATE") != std::string::npos)
	{
		__FE_COUT__ << "Auto-generating ROC write block data... 2nd argument is count."
		            << __E__;
		std::vector<std::string> split = StringMacros::getVectorFromString(
		    writeDataIn, {',', '|', '&', ' '} /* delimeters */);
		__FE_COUTV__(StringMacros::vectorToString(split));
		if(split.size() != 2)
		{
			__FE_SS__ << "Illegal ROC write block auto-generate parameters found: "
			          << writeDataIn << __E__
			          << "Must be 'AUTOGENERATE <count>' where count is a number."
			          << __E__;
			__FE_SS_THROW__;
		}
		uint16_t count;
		StringMacros::getNumber(split[1], count);
		__FE_COUTV__(count);
		//generate incrementing counter data
		for(uint16_t i = 0; i < count; ++i)
			writeData.push_back(i);
	}
	else if(writeDataIn.find(',') == std::string::npos)
	{
		__FE_COUT__ << "Assuming write data is a CSV filename (because no comma found): "
		            << writeDataIn << __E__;
		FILE* fp = fopen(writeDataIn.c_str(), "r");
		if(fp)
		{
			fseek(fp, 0, SEEK_END);
			const unsigned long fileSize = ftell(fp);
			size_t              readSize;
			writeDataIn.resize(fileSize);
			rewind(fp);
			if((readSize = fread(&writeDataIn[0], 1, fileSize, fp)) != fileSize)
			{
				__FE_SS__ << "CSV filename (because no comma found) could not be read! "
				             "Wrong byte count returned: readSize="
				          << readSize << " vs fileSize=" << fileSize << __E__;
				__FE_SS_THROW__;
			}

			fclose(fp);
		}
		else
		{
			__FE_SS__ << "CSV filename (because no comma found) was not founnd: "
			          << writeDataIn << __E__;
			__FE_SS_THROW__;
		}
	}     //end CSV file handling
	else  //literal
	{
		__FE_COUT__ << "CSV literal: " << writeDataIn << __E__;
		std::vector<std::string> writeDataStrings =
		    StringMacros::getVectorFromString(writeDataIn);
		for(auto& writeDataString : writeDataStrings)
		{
			DTCLib::roc_data_t number;
			StringMacros::getNumber(writeDataString, number);
			writeData.push_back(number);
		}
	}

	__FE_COUTV__(StringMacros::vectorToString(writeData));
	__FE_COUT__ << "numberOfWords = " << std::dec << (unsigned int)writeData.size()
	            << __E__;

	bool        found  = false;
	std::string result = "";
	for(auto& roc : rocs_)
	{
		if(usingRocMask)
			__FE_COUTT__ << "0x" << std::hex << (1 << (int(roc.second->getLinkID()) * 4))
			             << " vs rocLinkIndexVal = 0x" << std::hex << rocLinkIndexVal
			             << __E__;
		else
			__FE_COUTT__ << "Found link ID " << roc.second->getLinkID() << " looking for "
			             << rocLinkIndex << __E__;

		if((!usingRocMask &&  //use ROC index
		    (rocLinkIndex == DTCLib::DTC_Link_ID::DTC_Link_ALL ||
		     roc.second->getLinkID() == rocLinkIndex)) ||
		   (usingRocMask &&  //use ROC mask
		    ((1 << (int(roc.second->getLinkID()) * 4)) & rocLinkIndexVal)))
		{
			found = true;
			__FE_COUTT__ << "Doing " << roc.second->getLinkID() << __E__;
			roc.second->writeBlock(writeData, address, incrementAddress, requestAck);

			// for(auto &argOut:argsOut)
			std::stringstream oss;
			oss << "Wrote " << writeData.size() << " words to address 0x" << std::hex
			    << address
			    << ", incrementingAddress=" << (incrementAddress ? "TRUE" : "FALSE")
			    << ", requestAck=" << (requestAck ? "TRUE" : "FALSE") << __E__;

			if(result.size())
				result += "\n";
			if(rocLinkIndex == DTC_Link_ALL || usingRocMask)
				result += "(" +
				          std::to_string(static_cast<uint8_t>(roc.second->getLinkID())) +
				          ") ";
			result += oss.str();
			__FE_COUT__ << oss.str();
		}
	}  //end roc exec loop

	if(found)
	{
		__SET_ARG_OUT__("Status", result);
		return;
	}

	__FE_SS__ << "Target ROC or Mask 0x" << std::hex << rocLinkIndexVal << " not found!"
	          << __E__;
	__FE_SS_THROW__;

}  // end BlockWriteROC()

//========================================================================
void DTCFrontEndInterface::DTCHighRateBlockCheck(__ARGS__)
{
	uint32_t rocLinkIndexVal = __GET_ARG_IN__(
	    "Target ROC or Mask (Default = -1 := all ROCs, or 0x111111 := all)",
	    uint32_t,
	    -1 /* ALL */);
	bool usingRocMask = false;
	if(rocLinkIndexVal != uint32_t(-1) && rocLinkIndexVal > 5)
	{
		usingRocMask = true;
		__FE_COUT__ << "Using ROC Link Mask value: 0x" << std::hex
		            << (unsigned int)rocLinkIndexVal << std::dec << __E__;
	}

	DTCLib::DTC_Link_ID rocLinkIndex =
	    DTCLib::DTC_Link_ID(usingRocMask ? -1 : rocLinkIndexVal);
	__FE_COUT__ << "rocLinkIndexVal = 0x" << std::hex << rocLinkIndexVal << __E__;
	__FE_COUTV__(usingRocMask);
	__FE_COUTV__(rocLinkIndex);

	unsigned int loops       = __GET_ARG_IN__("loops", unsigned int);
	unsigned int baseAddress = __GET_ARG_IN__("baseAddress", unsigned int);
	unsigned int correctRegisterValue0 =
	    __GET_ARG_IN__("correctRegisterValue0", unsigned int);
	unsigned int correctRegisterValue1 =
	    __GET_ARG_IN__("correctRegisterValue1", unsigned int);

	__FE_COUTV__(loops);
	__FE_COUTV__(baseAddress);
	__FE_COUTV__(correctRegisterValue0);
	__FE_COUTV__(correctRegisterValue1);

	bool found = false;
	for(auto& roc : rocs_)
	{
		if(usingRocMask)
			__FE_COUTT__ << "0x" << std::hex << (1 << (int(roc.second->getLinkID()) * 4))
			             << " vs rocLinkIndexVal = 0x" << std::hex << rocLinkIndexVal
			             << __E__;
		else
			__FE_COUTT__ << "Found link ID " << roc.second->getLinkID() << " looking for "
			             << rocLinkIndex << __E__;

		if((!usingRocMask &&  //use ROC index
		    (rocLinkIndex == DTCLib::DTC_Link_ID::DTC_Link_ALL ||
		     roc.second->getLinkID() == rocLinkIndex)) ||
		   (usingRocMask &&  //use ROC mask
		    ((1 << (int(roc.second->getLinkID()) * 4)) & rocLinkIndexVal)))
		{
			found = true;
			__FE_COUTT__ << "Doing " << roc.second->getLinkID() << __E__;

			roc.second->highRateBlockCheck(
			    loops, baseAddress, correctRegisterValue0, correctRegisterValue1);
		}
	}  //end roc exec loop

	if(!found)
	{
		__FE_SS__ << "Target ROC or Mask 0x" << std::hex << rocLinkIndexVal
		          << " not found!" << __E__;
		__FE_SS_THROW__;
	}

}  // end DTCHighRateBlockCheck()

//========================================================================
void DTCFrontEndInterface::DTCHighRateDCSCheck(__ARGS__)
{
	uint32_t rocLinkIndexVal = __GET_ARG_IN__(
	    "Target ROC or Mask (Default = -1 := all ROCs, or 0x111111 := all)",
	    uint32_t,
	    -1 /* ALL */);
	bool usingRocMask = false;
	if(rocLinkIndexVal != uint32_t(-1) && rocLinkIndexVal > 5)
	{
		usingRocMask = true;
		__FE_COUT__ << "Using ROC Link Mask value: 0x" << std::hex
		            << (unsigned int)rocLinkIndexVal << std::dec << __E__;
	}

	DTCLib::DTC_Link_ID rocLinkIndex =
	    DTCLib::DTC_Link_ID(usingRocMask ? -1 : rocLinkIndexVal);
	__FE_COUT__ << "rocLinkIndexVal = 0x" << std::hex << rocLinkIndexVal << __E__;
	__FE_COUTV__(usingRocMask);
	__FE_COUTV__(rocLinkIndex);

	unsigned int loops       = __GET_ARG_IN__("loops", unsigned int);
	unsigned int baseAddress = __GET_ARG_IN__("baseAddress", unsigned int);
	unsigned int correctRegisterValue0 =
	    __GET_ARG_IN__("correctRegisterValue0", unsigned int);
	unsigned int correctRegisterValue1 =
	    __GET_ARG_IN__("correctRegisterValue1", unsigned int);

	__FE_COUTV__(loops);
	__FE_COUTV__(baseAddress);
	__FE_COUTV__(correctRegisterValue0);
	__FE_COUTV__(correctRegisterValue1);

	bool found = false;
	for(auto& roc : rocs_)
	{
		if(usingRocMask)
			__FE_COUTT__ << "0x" << std::hex << (1 << (int(roc.second->getLinkID()) * 4))
			             << " vs rocLinkIndexVal = 0x" << std::hex << rocLinkIndexVal
			             << __E__;
		else
			__FE_COUTT__ << "Found link ID " << roc.second->getLinkID() << " looking for "
			             << rocLinkIndex << __E__;

		if((!usingRocMask &&  //use ROC index
		    (rocLinkIndex == DTCLib::DTC_Link_ID::DTC_Link_ALL ||
		     roc.second->getLinkID() == rocLinkIndex)) ||
		   (usingRocMask &&  //use ROC mask
		    ((1 << (int(roc.second->getLinkID()) * 4)) & rocLinkIndexVal)))
		{
			found = true;
			__FE_COUTT__ << "Doing " << roc.second->getLinkID() << __E__;
			roc.second->highRateCheck(
			    loops, baseAddress, correctRegisterValue0, correctRegisterValue1);
		}
	}  //end roc exec loop

	if(!found)
	{
		__FE_SS__ << "Target ROC or Mask 0x" << std::hex << rocLinkIndexVal
		          << " not found!" << __E__;
		__FE_SS_THROW__;
	}

}  // end DTCHighRateDCSCheck()

//========================================================================
void DTCFrontEndInterface::ResetLossOfLockCounter(__ARGS__)
{
	// write anything to reset
	// 0x93c8 is RX CDR Unlock counter (32-bit)
	getDTC()->ClearRXCDRUnlockCount(DTCLib::DTC_Link_ID::DTC_Link_CFO);
	// registerWrite(0x93c8, 0);

	// now check
	// uint32_t readData = registerRead(0x93c8);

	// char readDataStr[100];
	// sprintf(readDataStr, "%d", readData);
	// __SET_ARG_OUT__("Upstream Rx Lock Loss Count", readDataStr);
	__SET_ARG_OUT__("Upstream Rx Lock Loss Count",
	                getDTC()->FormatRXCDRUnlockCountCFOLink());
}  // end ResetLossOfLockCounter()

//========================================================================
void DTCFrontEndInterface::ReadLossOfLockCounter(__ARGS__)
{
	// 0x93c8 is RX CDR Unlock counter (32-bit)
	uint32_t readData =  //registerRead(0x93c8);
	    getDTC()->ReadRXCDRUnlockCount(DTCLib::DTC_Link_ID::DTC_Link_CFO);

	char readDataStr[100];
	sprintf(readDataStr, "%d", readData);

	// 0x9140 bit-6 is RX CDR is locked

	bool isUpstreamLocked = 1;
	for(int i = 0; i < 5; ++i)  //read 5x for multiple samples in case of instability
	{
		isUpstreamLocked &=
		    getDTC()->ReadSERDESRXCDRLock(DTCLib::DTC_Link_ID::DTC_Link_CFO);
		// readData = registerRead(0x9140);
		// isUpstreamLocked &=
		//     (readData >> 6) & 1;  //& to force unlocked for any unlocked reading
	}
	//__SET_ARG_OUT__("Upstream Rx CDR Lock Status",isUpstreamLocked?"LOCKED":"Not
	//Locked");

	// 0x9128 bit-6 is RX PLL
	// readData		    = registerRead(0x9128);
	bool isUpstreamPLLLocked =  //(readData >> 6) & 1;
	    getDTC()->ReadSERDESPLLLocked(DTCLib::DTC_Link_ID::DTC_Link_CFO);

	// Jitter attenuator has configurable "Free Running" mode
	// LOL == Loss of Lock, LOS == Loss of Signal (4-inputs to jitter attenuator)
	// 0x9308 bit-0 is reset, input select bit-5:4, bit-8 is LOL, bit-11:9 (input LOS)
	// readData          = //registerRead(0x9308);

	uint32_t val =
	    getDTC()->ReadJitterAttenuatorSelect().to_ulong();  //(readData >> 4) & 3;
	std::string JAsrc =
	    val == 0 ? "from emulated CFO" : (val == 1 ? "from RJ45" : "from FMC/SFP+");

	__SET_ARG_OUT__(
	    "Upstream Rx Lock Loss Count",
	    std::string(readDataStr) +
	        "... CDR = " + std::string(isUpstreamLocked ? " LOCKED" : " Not Locked") +
	        "... PLL = " + std::string(isUpstreamPLLLocked ? " LOCKED" : " Not Locked") +
	        "... JA = " + JAsrc);

}  // end ReadLossOfLockCounter()

//========================================================================
void DTCFrontEndInterface::SpyBuffer(__ARGS__)
{
	auto* device = getDTC()->GetDevice();
	__SS__ << "Triggered spy dump of DAQ DMA buffers:" << __E__;
	device->spy(DTC_DMA_Engine_DAQ,
	            3 /* for once */ | 8 /* for wide view */ | (1 << 28) /* force spy */,
	            ss);

	__COUT_MULTI_LBL__(0, ss.str(), "spy");
	__SET_ARG_OUT__("Result", ss.str());
}  // end SpyBuffer()

//========================================================================
void DTCFrontEndInterface::ReleaseAllDAQBuffers(__ARGS__)
{
	getDTC()->ReleaseAllBuffers(DTC_DMA_Engine_DAQ);
	__SET_ARG_OUT__("Result", "ReleaseAllBuffers(DTC_DMA_Engine_DAQ) called.");
}  // end ReleaseAllDAQBuffers()

//========================================================================
void DTCFrontEndInterface::GetLinkLockStatus(__ARGS__)
{
	std::stringstream outss;
	outss << getDTC()->FormatRXCDRLockStatus() << "\n\n" << getDTC()->FormatLinkEnable();
	__SET_ARG_OUT__("Lock Status", "\n" + outss.str());
}  // end GetLinkLockStatus()

//========================================================================
void DTCFrontEndInterface::WriteDTC(__ARGS__)
{
	// FIXME: Add optional validation, defaulted to true
	uint32_t   address   = __GET_ARG_IN__("address", uint32_t);
	uint32_t   writeData = __GET_ARG_IN__("writeData", uint32_t);
	const bool validate  = __GET_ARG_IN__("Do validation (Default := true)", bool, true);
	__FE_COUTV__((unsigned int)address);
	__FE_COUTV__((unsigned int)writeData);
	__FE_COUTV__(validate);

	int           errorCode(0);
	uint32_t      readData(0);
	constexpr int timeout_ms(100);  // for direct writes

	if(validate)
		readData = getDTC()->WriteRegister_(writeData, address);
	else
		errorCode = getDevice()->write_register(address, timeout_ms, writeData);
	if(errorCode != 0)
	{
		__FE_SS__ << "Error writing register 0x" << std::hex << std::setfill('0')
		          << std::setw(4) << address << ". Error code = " << errorCode
		          << " readData (if validated) = " << readData;
		__SS_THROW__;
	}

	std::stringstream ss;
	ss << "Wrote " << std::dec << writeData << " 0x" << std::hex << std::setfill('0')
	   << std::setw(8) << writeData << " to address 0x" << std::setw(4) << address << ".";
	__SET_ARG_OUT__("Status", ss.str());  // readDataStr);
}  // end WriteDTC()

//========================================================================
void DTCFrontEndInterface::ReadDTC(__ARGS__)
{
	uint32_t address = __GET_ARG_IN__("address", uint32_t);
	__FE_COUTV__((unsigned int)address);
	uint32_t readData;

	int errorCode = getDevice()->read_register(address, 100, &readData);
	if(errorCode != 0)
	{
		__FE_SS__ << "Error reading register 0x" << std::hex << address << " "
		          << errorCode;
		__SS_THROW__;
	}

	// converted to dec and hex display in FEVInterfacesManager handling of FE Macros
	std::stringstream ss;
	ss << "Read " << std::dec << readData << " 0x" << std::hex << std::setfill('0')
	   << std::setw(8) << readData << " from address 0x" << std::setw(4) << address
	   << ".";
	__SET_ARG_OUT__("readData", ss.str());  // readDataStr);
}  // end ReadDTC()

//========================================================================
void DTCFrontEndInterface::SetCFOEventModeRequiredMask(__ARGS__)
{
	uint32_t eventModeRequiredMask =
	    __GET_ARG_IN__("Event Mode Required Mask (Default := 0)", uint32_t, 0);
	__FE_COUTV__(eventModeRequiredMask);

	getDTC()->SetCFOEventModeRequiredMask(eventModeRequiredMask);

	std::stringstream ss;
	ss << "Set Event Mode Required Mask to 0x" << std::hex << std::setfill('0')
	   << std::setw(8) << eventModeRequiredMask << ".";
	__SET_ARG_OUT__("Result", ss.str());
}  // end SetCFOEventModeRequiredMask()

//========================================================================
void DTCFrontEndInterface::ReadCFOEventModeRequiredMask(__ARGS__)
{
	const uint32_t eventModeRequiredMask = getDTC()->ReadCFOEventModeRequiredMask();

	std::stringstream ss;
	ss << "Event Mode Required Mask: " << std::dec << eventModeRequiredMask << " (0x"
	   << std::hex << std::setfill('0') << std::setw(8) << eventModeRequiredMask << ")";
	__SET_ARG_OUT__("Event Mode Required Mask", ss.str());
}  // end ReadCFOEventModeRequiredMask()

//========================================================================
void DTCFrontEndInterface::RunROCFEMacro(__ARGS__)
{
	//	std::string feMacroName = __GET_ARG_IN__("ROC_FEMacroName", std::string);
	//	std::string rocUID = __GET_ARG_IN__("ROC_UID", std::string);
	//
	//	__FE_COUTV__(feMacroName);
	//	__FE_COUTV__(rocUID);

	auto feMacroIt = rocFEMacroMap_.find(feMacroStruct.feMacroName_);
	if(feMacroIt == rocFEMacroMap_.end())
	{
		__FE_SS__ << "Fatal error - ROC FE Macro name '" << feMacroStruct.feMacroName_
		          << "' not found in DTC's map!" << __E__;
		__FE_SS_THROW__;
	}

	const std::string& rocUID         = feMacroIt->second.first;
	const std::string& rocFEMacroName = feMacroIt->second.second;

	if(rocUID == "")
	{
		__FE_COUT__ << "Using ROC Link Index parameter to define ROC target" << __E__;

		uint32_t rocLinkIndexVal = __GET_ARG_IN__(
		    "Target ROC or Mask (Default = -1 := all ROCs, or 0x111111 := all)",
		    uint32_t,
		    -1 /* ALL */);
		bool usingRocMask = false;
		if(rocLinkIndexVal != uint32_t(-1) && rocLinkIndexVal > 5)
		{
			usingRocMask = true;
			__FE_COUT__ << "Using ROC Link Mask value: 0x" << std::hex
			            << (unsigned int)rocLinkIndexVal << std::dec << __E__;
		}

		DTCLib::DTC_Link_ID rocLinkIndex =
		    DTCLib::DTC_Link_ID(usingRocMask ? -1 : rocLinkIndexVal);
		__FE_COUT__ << "rocLinkIndexVal = 0x" << std::hex << rocLinkIndexVal << __E__;
		__FE_COUTV__(usingRocMask);
		__FE_COUTV__(rocLinkIndex);

		//remove ROC index from input args (since not in official Macro registration)
		std::vector<ots::FEVInterface::frontEndMacroArg_t> inputArgs_inst;
		for(size_t i = 1; i < argsIn.size(); ++i)
			inputArgs_inst.push_back(argsIn[i]);

		FEVInterface::frontEndMacroConstArgs_t inputArgs = inputArgs_inst;

		// Capture per-ROC outputs independently so each ROC macro can run in parallel
		// and the combined result can still be assembled in a deterministic order.
		struct RocMacroLaunchResult
		{
			DTCLib::DTC_Link_ID                                linkID;
			ROCCoreVInterface*                                 roc;
			std::vector<ots::FEVInterface::frontEndMacroArg_t> outputArgs;
			std::string                                        error;
		};

		// First collect the matching ROCs in map iteration order. That lets the
		// execution happen concurrently while preserving the original output ordering.
		std::vector<RocMacroLaunchResult> selectedRocs;
		for(auto& roc : rocs_)
		{
			if(usingRocMask)
				__FE_COUT__ << "0x" << std::hex
				            << (1 << (int(roc.second->getLinkID()) * 4))
				            << " vs rocLinkIndexVal = 0x" << std::hex << rocLinkIndexVal
				            << __E__;

			if((!usingRocMask &&  //use ROC index
			    (rocLinkIndex == DTCLib::DTC_Link_ID::DTC_Link_ALL ||
			     roc.second->getLinkID() == rocLinkIndex)) ||
			   (usingRocMask &&  //use ROC mask
			    ((1 << (int(roc.second->getLinkID()) * 4)) & rocLinkIndexVal)))
			{
				__FE_COUTV__(rocFEMacroName);
				__FE_COUTV__(roc.second->getLinkID());

				selectedRocs.push_back(
				    {roc.second->getLinkID(), roc.second.get(), {}, ""});

				for(size_t i = 1; i < argsOut.size(); ++i)
					selectedRocs.back().outputArgs.push_back(
					    make_pair(argsOut[i].first, ""));

				if(!usingRocMask && rocLinkIndex != DTCLib::DTC_Link_ID::DTC_Link_ALL)
					break;  //done with target ROC
			}               //end ROC match
		}                   //end ROC FEMacro launch loop

		if(selectedRocs.empty())
		{
			__FE_SS__ << "Fatal error - Target ROC or Mask '" << int(rocLinkIndexVal)
			          << " (0x" << std::hex << rocLinkIndexVal
			          << ")' not found in DTC's instantiated ROCs (make sure your ROC is "
			             "enabled)! Here is the list of "
			             "enabled ROC links: ";
			int i = 0;
			for(auto& roc : rocs_)
				ss << (i++ ? ", " : "") << roc.second->getLinkID();
			ss << __E__;
			__FE_SS_THROW__;
		}

		for(auto& argOut : argsOut)
			if(argOut.first !=
			   PLOTLY_PLOT /* defined at FEVinterface.h */)  //leave built-in arg as DEFAULT
				argOut.second = "";

		// Launch one worker thread per selected ROC FE Macro.
		std::vector<std::thread> launchThreads;
		launchThreads.reserve(selectedRocs.size());
		for(auto& selectedRoc : selectedRocs)
		{
			launchThreads.emplace_back([&inputArgs, &rocFEMacroName, &selectedRoc]() {
				try
				{
					__COUT__ << "ROC FE Macro thread start. rocLink="
					         << selectedRoc.linkID << " macro=" << rocFEMacroName
					         << " threadid=" << std::this_thread::get_id() << __E__;
					selectedRoc.roc->runSelfFrontEndMacro(
					    rocFEMacroName, inputArgs, selectedRoc.outputArgs);
					__COUT__ << "ROC FE Macro thread done. rocLink=" << selectedRoc.linkID
					         << " macro=" << rocFEMacroName
					         << " threadid=" << std::this_thread::get_id() << __E__;
				}
				catch(const std::exception& e)
				{
					selectedRoc.error = e.what();
				}
				catch(...)
				{
					selectedRoc.error = "Unknown exception while running ROC FE Macro.";
				}
			});
		}

		for(auto& launchThread : launchThreads)
			launchThread.join();

		for(const auto& selectedRoc : selectedRocs)
			if(!selectedRoc.error.empty())
			{
				__FE_SS__ << "ROC FE Macro '" << rocFEMacroName
				          << "' failed for ROC link " << selectedRoc.linkID << ": "
				          << selectedRoc.error << __E__;
				__FE_SS_THROW__;
			}

		// Merge per-ROC outputs after all threads complete, keeping the original
		// CSV/array formatting expected by the FE Macro response.
		bool arrayNotation = selectedRocs.size() > 1;
		bool openedArray   = false;
		for(size_t rocIndex = 0; rocIndex < selectedRocs.size(); ++rocIndex)
		{
			const auto& selectedRoc = selectedRocs[rocIndex];
			bool        found       = rocIndex != 0;

			if(found && arrayNotation && !openedArray)
				argsOut[0].second =
				    "[" + argsOut[0].second;  //add leading bracket for array notation
			argsOut[0].second +=              //add new value
			    (found ? ", " : "") + std::to_string(selectedRoc.linkID);
			__FE_COUTT__ << argsOut[0].first << ": " << argsOut[0].second << __E__;

			for(size_t i = 1; i < argsOut.size() && i - 1 < selectedRoc.outputArgs.size();
			    ++i)
			{
				if(found && arrayNotation && !openedArray)
					argsOut[i].second =
					    "[" + argsOut[i].second;  //add leading bracket for array notation
				argsOut[i].second +=              //add new value
				    (found ? ", " : "") + selectedRoc.outputArgs[i - 1].second;
				__FE_COUTT__ << argsOut[i].first << ": " << argsOut[i].second << __E__;
			}

			if(found && arrayNotation)
				openedArray = true;
		}

		//finalize array notation for output args
		if(arrayNotation)
		{
			for(auto& argOut : argsOut)
				if(argOut.first !=
				   PLOTLY_PLOT /* defined at FEVinterface.h */)  //leave built-in arg as DEFAULT
					argOut.second += "]";  //add trailing bracket for array notation
		}
	}
	else  //individual target defined by feMacroIt pair
	{
		__FE_COUTV__(rocUID);
		__FE_COUTV__(rocFEMacroName);

		auto rocIt = rocs_.find(rocUID);
		if(rocIt == rocs_.end())
		{
			__FE_SS__ << "Fatal error - ROC name '" << rocUID
			          << "' not found in DTC's map!" << __E__;
			__FE_SS_THROW__;
		}

		rocIt->second->runSelfFrontEndMacro(rocFEMacroName, argsIn, argsOut);
	}

}  // end RunROCFEMacro()

//========================================================================
void DTCFrontEndInterface::SetupROCs(__ARGS__)
{
	uint32_t rocLinkIndexVal = __GET_ARG_IN__(
	    "Target ROC or Mask (Default = -1 := all ROCs, or 0x111111 := all)",
	    uint32_t,
	    -1 /* ALL */);
	bool usingRocMask = false;
	if(rocLinkIndexVal != uint32_t(-1) && rocLinkIndexVal > 5)
	{
		usingRocMask = true;
		__FE_COUT__ << "Using ROC Link Mask value: 0x" << std::hex
		            << (unsigned int)rocLinkIndexVal << std::dec << __E__;
	}

	DTCLib::DTC_Link_ID rocLinkIndex =
	    DTCLib::DTC_Link_ID(usingRocMask ? -1 : rocLinkIndexVal);
	__FE_COUT__ << "rocLinkIndexVal = 0x" << std::hex << rocLinkIndexVal << __E__;
	__FE_COUTV__(usingRocMask);
	__FE_COUTV__(rocLinkIndex);
	__FE_COUTV__(rocs_.size());

	bool        found          = false;
	std::string result         = "";
	std::string setupRocResult = "";
	for(auto& roc : rocs_)
	{
		if(usingRocMask)
			__FE_COUTT__ << "0x" << std::hex << (1 << (int(roc.second->getLinkID()) * 4))
			             << " vs rocLinkIndexVal = 0x" << std::hex << rocLinkIndexVal
			             << __E__;
		else
			__FE_COUTT__ << "Found link ID " << roc.second->getLinkID() << " looking for "
			             << rocLinkIndex << __E__;

		if((!usingRocMask &&  //use ROC index
		    (rocLinkIndex == DTCLib::DTC_Link_ID::DTC_Link_ALL ||
		     roc.second->getLinkID() == rocLinkIndex)) ||
		   (usingRocMask &&  //use ROC mask
		    ((1 << (int(roc.second->getLinkID()) * 4)) & rocLinkIndexVal)))
		{
			found = true;
			__FE_COUTT__ << "Doing " << roc.second->getLinkID() << __E__;

			setupRocResult = SetupROCs(
			    roc.second->getLinkID(),
			    __GET_ARG_IN__("Set Link RX/TX Enable (Default := false)", bool, false),
			    __GET_ARG_IN__("Set Link Timing Enable (Default := false)", bool, false),
			    __GET_ARG_IN__(
			        "Set ROC Emulation Enable (Default := false)", bool, false),
			    DTCLib::DTC_ROC_Emulation_Type(
			        __GET_ARG_IN__("ROC Emulation Type (Default = 0: Internal, 1: "
			                       "Fiber-Loopback, 2: External)",
			                       uint8_t,
			                       0 /* internal */)),
			    __GET_ARG_IN__(
			        "ROC generated Data Payload fragment packet count (11-bits, "
			        "Default := 16)",
			        uint32_t,
			        16),
			    __GET_ARG_IN__(
			        "Block Null Heartbeats to ALL ROCs (Default := false)", bool, false),
			    __GET_ARG_IN__(
			        "Resequence Non-null Events for ALL ROCs (Default := false)",
			        bool,
			        false));

			if(result.size())
				result += ", ";
			if(rocLinkIndex == DTC_Link_ALL || usingRocMask)
				result += "(" +
				          std::to_string(static_cast<uint8_t>(roc.second->getLinkID())) +
				          ")";
			// result = setupRocResult; // not +=, always overwrite with last result;
			__FE_COUTV__(setupRocResult);
		}
	}  //end roc exec loop

	result += "\n" + setupRocResult;  // not +=, always overwrite with last result;

	if(found)
	{
		__SET_ARG_OUT__("Result", result);
		return;
	}

	__FE_SS__ << "Target ROC or Mask 0x" << std::hex << rocLinkIndexVal << " not found!"
	          << __E__;
	__FE_SS_THROW__;
}  // end SetEmulatedROCEventFragmentSize()

//========================================================================
std::string DTCFrontEndInterface::SetupROCs(
    DTCLib::DTC_Link_ID            rocLinkIndex,
    bool                           rocRxTxEnable,
    bool                           rocTimingEnable,
    bool                           rocEmulationEnable,
    DTCLib::DTC_ROC_Emulation_Type rocEmulationType,
    uint32_t                       size,
    bool                           blockNullHeartbeats,
    bool                           resequenceNonNullEvents)
{
	__FE_COUTV__(rocLinkIndex);
	__FE_COUTV__(rocRxTxEnable);
	__FE_COUTV__(rocTimingEnable);
	__FE_COUTV__(rocEmulationEnable);

	if(rocLinkIndex > 5 && rocLinkIndex != DTC_Link_ID(-1))
	{
		__FE_SS__ << "Illegal Target ROC input: " << rocLinkIndex << __E__;
		__FE_SS_THROW__;
	}

	for(DTC_Link_ID link =
	        (rocLinkIndex == DTC_Link_ID(-1) ? DTC_Link_ID(0) : rocLinkIndex);
	    link <= (rocLinkIndex == DTC_Link_ID(-1) ? DTC_Link_ID(5) : rocLinkIndex);
	    ++link)
	{
		if(rocRxTxEnable)
			getDTC()->EnableLink(link);
		else
			getDTC()->DisableLink(link);
	}

	if(rocTimingEnable)
		getDTC()->SetCFO40MHzClockMarkerEnable(
		    rocLinkIndex < 6 ? DTC_ROC_Links[rocLinkIndex] : DTC_Link_ALL, true);
	else
		getDTC()->SetCFO40MHzClockMarkerEnable(
		    rocLinkIndex < 6 ? DTC_ROC_Links[rocLinkIndex] : DTC_Link_ALL, false);

	__FE_COUTV__(rocEmulationType);

	for(DTC_Link_ID link =
	        (rocLinkIndex == DTC_Link_ID(-1) ? DTC_Link_ID(0) : rocLinkIndex);
	    link <= (rocLinkIndex == DTC_Link_ID(-1) ? DTC_Link_ID(5) : rocLinkIndex);
	    ++link)
	{
		if(rocEmulationEnable)
			getDTC()->EnableROCEmulator(link, rocEmulationType);
		else
			getDTC()->DisableROCEmulator(link, rocEmulationType);
	}

	// To change the size of the event, need to write to each ROC emulator
	// 0x91B0 (b0-10 roc0, b16-26 roc1), 0x91B4 (b0-10 roc2, b16-26 roc3), 0x91B8 (b0-10
	// roc4, b16-26 roc5)

	uint32_t wsize = size & 0x07FF;  // only allow 11 bits

	__FE_COUTV__((unsigned int)size);
	__FE_COUTV__((unsigned int)wsize);
	if(size != wsize)
	{
		__FE_SS__
		    << "Out-of-range write emulated ROC Data Payload packet count specified: "
		    << size << ". The field size is 11-bits, which would truncate value to "
		    << wsize << __E__;
		__FE_SS_THROW__;
	}

	for(DTC_Link_ID link =
	        (rocLinkIndex == DTC_Link_ID(-1) ? DTC_Link_ID(0) : rocLinkIndex);
	    link <= (rocLinkIndex == DTC_Link_ID(-1) ? DTC_Link_ID(5) : rocLinkIndex);
	    ++link)
		getDTC()->SetROCEmulationNumPackets(rocLinkIndex, wsize);

	// Set Block Null Heartbeats and Resequence Non-null Events (one bit for all ROCs)
	getDTC()->SetBlockNullHeartbeatsToROC(blockNullHeartbeats);
	getDTC()->SetResequenceNonNullEvents(resequenceNonNullEvents);

	return getDTC()->FormattedRegDump(0, getDTC()->formattedROCEmulationFunctions_);

}  // end SetEmulatedROCEventFragmentSize()

//========================================================================
void DTCFrontEndInterface::configureHardwareDevMode(__ARGS__)
{
	configureHardwareDevMode();
}  // end configureHardwareDevMode()

//========================================================================
void DTCFrontEndInterface::DTCCounters(__ARGS__)
{
	__SET_ARG_OUT__(
	    "Protocol Counters",
	    getDTC()->FormattedRegDump(130, getDTC()->formattedPacketCounterFunctions_));
	__SET_ARG_OUT__(
	    "Performance Counters",
	    getDTC()->FormattedRegDump(130, getDTC()->formattedPerformanceCounterFunctions_));
}  // end DTCCounters()

//========================================================================
void DTCFrontEndInterface::readRxDiagFIFO(__ARGS__)
{
	DTCLib::DTC_Link_ID LinkIndex =
	    DTCLib::DTC_Link_ID(__GET_ARG_IN__("LinkIndex", uint8_t));

	__SET_ARG_OUT__("Diagnostic RX FIFO",
	                getDTC()->FormatRXDiagFifo(DTCLib::DTC_ROC_Links[LinkIndex]));

}  //end readRxDiagFIFO()

//========================================================================
void DTCFrontEndInterface::readTxDiagFIFO(__ARGS__)
{
	DTCLib::DTC_Link_ID LinkIndex =
	    DTCLib::DTC_Link_ID(__GET_ARG_IN__("LinkIndex", uint8_t));

	__SET_ARG_OUT__("Diagnostic TX FIFO",
	                getDTC()->FormatTXDiagFifo(DTCLib::DTC_ROC_Links[LinkIndex]));

}  //end readTxDiagFIFO()

//========================================================================
void DTCFrontEndInterface::GetLinkErrors(__ARGS__)
{
	__SET_ARG_OUT__(
	    "Link Errors",
	    getDTC()->FormattedRegDump(0, getDTC()->formattedSERDESErrorFunctions_));
}  //end GetLinkErrors()

// //========================================================================
// void DTCFrontEndInterface::ROCDestroy(__ARGS__)
// {

//	rocs_.clear();

// } //end ROCDestroy()
// //========================================================================
// void DTCFrontEndInterface::ROCInstantiate(__ARGS__)
// {
//	createROCs();

// } //end ROCInstantiate()

//========================================================================
void DTCFrontEndInterface::DTCInstantiate(__ARGS__) { DTCInstantiate(); }

//========================================================================
void DTCFrontEndInterface::DTCInstantiate()
{
	if(thisDTC_)
		delete thisDTC_;
	thisDTC_ = nullptr;

	DTCLib::DTC_SimMode mode =
	    emulate_cfo_ ? DTCLib::DTC_SimMode_NoCFO : DTCLib::DTC_SimMode_Disabled;
	if(emulatorMode_)
		mode = DTCLib::
		    DTC_SimMode_Performance;  //This is simple ROC emulator style simulation

	unsigned dtc_class_roc_mask = 0;
	// create roc mask for DTC
	auto rocLink = Configurable::getSelfNode().getNode("LinkToROCGroupTable");
	if(!rocLink.isDisconnected())
	{
		std::vector<std::pair<std::string, ConfigurationTree>> rocChildren =
		    rocLink.getChildren();

		__FE_COUTV__(rocChildren.size());
		roc_mask_          = 0;
		roc_emulated_mask_ = 0;

		for(auto& roc : rocChildren)
		{
			bool enabled  = roc.second.getNode("Status").getValue<bool>();
			bool emulated = roc.second.getNode("EmulateInDTCHardware").getValue<bool>();

			__FE_COUT__ << "roc uid " << roc.first << (enabled ? " enabled" : "")
			            << (emulated ? " emulated" : "") << __E__;

			if(enabled)
			{
				int linkID = roc.second.getNode("linkID").getValue<int>();
				roc_mask_ |= (0x1 << linkID);
				if(emulated)
					roc_emulated_mask_ |= (0x1 << linkID);
				dtc_class_roc_mask |=
				    (0x1 << (linkID * 4));  // the DTC class instantiation expects each
				                            // ROC has its own hex nibble
			}
		}

		__FE_COUT__ << "DTC roc_mask_ = 0x" << std::hex << roc_mask_ << std::dec << __E__;
		__FE_COUT__ << "roc_mask to instantiate DTC class = 0x" << std::hex
		            << dtc_class_roc_mask << std::dec << __E__;

	}  // end create roc mask

	// DTC firmware design version must match ReadDesignDate()_ReadVivadoVersion() [ ignoring ReadDesignVersionNumber() for now ]
	// for example the string might be "Jun/13/2023 16:00	raw-data: 0x23061316" + "_22.1"
	std::string expectedDesignVersion = "";
	try
	{
		expectedDesignVersion =
		    getSelfNode().getNode("ExpectedFirmwareVersion").getValueWithDefault("");
	}
	catch(const std::runtime_error& e)
	{
		//ignoring missing field, so not enforcing firmware version
	}

	__FE_COUT__ << "DTC arguments..." << std::endl;
	__FE_COUTV__(mode);
	__FE_COUTV__(deviceIndex_);
	__FE_COUTV__(dtc_class_roc_mask);
	__FE_COUTV__(expectedDesignVersion);
	__FE_COUTV__(skipInit_);
	__FE_COUT__ << "END DTC arguments..." << std::endl;

	size_t dtcPos = getInterfaceUID().find("DTC");
	if(dtcPos != std::string::npos && dtcPos + 3 < getInterfaceUID().size())
	{
		__FE_COUT__ << "Checking that PCIe device matches guidance in UID '"
		            << getInterfaceUID() << "'..." << __E__;

		bool mismatch = false;
		if(getInterfaceUID()[dtcPos + 3] == '_' || getInterfaceUID()[dtcPos + 3] == '-')
		{
			__FE_COUTT__ << "Checking that PCIe device matches guidance in UID with _/- '"
			             << getInterfaceUID() << "'..." << __E__;
			if(dtcPos + 4 < getInterfaceUID().size() &&
			   uint8_t(getInterfaceUID()[dtcPos + 4]) - 48 !=
			       uint8_t(
			           deviceIndex_))  //convert ascii '0' '1' .. to number deviceIndex_
				mismatch = true;
		}
		else if(uint8_t(getInterfaceUID()[dtcPos + 3]) - 48 < 4 &&
		        uint8_t(getInterfaceUID()[dtcPos + 3]) - 48 !=
		            uint8_t(
		                deviceIndex_))  //convert ascii '0' '1' .. to number deviceIndex_
			mismatch = true;

		if(mismatch)
		{
			__FE_SS__
			    << "PCIe device index '" << deviceIndex_
			    << "' does not match guidance in UID '" << getInterfaceUID()
			    << "' - would expect 'DTC" << deviceIndex_
			    << "' in the UID string for this device. Please use DTC<device index> in "
			       "your naming convention, or remove the 'DTC' keyword from the UID."
			    << __E__;
			__FE_COUT_WARN__ << ss.str();
		}
	}

	// instantiate DTC with the appropriate ROCs enabled
	thisDTC_ = new DTCLib::DTC(
	    mode,
	    deviceIndex_,
	    dtc_class_roc_mask,
	    expectedDesignVersion,
	    mode != DTCLib::DTC_SimMode_Performance /* skipInit */
	    ,  //always skip init for real hardware, and use ots configure setup; allow init for simulation
	    "" /* simMemoryFile */,
	    getInterfaceUID());

	try  //attempt to print out firmware version to the log
	{
		std::string designVersion = getDTC()->ReadDesignVersion();
		__FE_COUTV__(designVersion);
	}
	catch(...)
	{
	}  //hide exception to finish instantiation (likely exception is from a need to reset PCIe)
	__FE_COUT__ << "Linux Kernel Driver Version: " << getDevice()->get_driver_version()
	            << __E__;

	createROCs();
	registerFEMacros();

	// DTC-specific info
	dtc_location_in_chain_ =
	    getSelfNode().getNode("LocationInChain").getValue<unsigned int>();

	__FE_COUT_INFO__ << "DTC instantiated with name: " << getInterfaceUID()
	                 << " dtc_location_in_chain_ = " << dtc_location_in_chain_
	                 << " talking to /dev/mu2e" << deviceIndex_ << __E__;

}  //end DTCInstantiate()

//========================================================================
void DTCFrontEndInterface::EnableDTCLink(__ARGS__)
{
	DTCLib::DTC_Link_ID linkIndex = DTCLib::DTC_Link_ID(
	    __GET_ARG_IN__("Target Link (Default = -1 := all links)", uint8_t, -1 /* ALL */));
	bool enable = __GET_ARG_IN__("Set Link RX/TX Enable (Default := false)", bool, false);

	__FE_COUTV__(linkIndex);
	__FE_COUTV__(enable);

	for(DTC_Link_ID link = (linkIndex == DTC_Link_ID(-1) ? DTC_Link_ID(0) : linkIndex);
	    link <= (linkIndex == DTC_Link_ID(-1) ? DTC_Link_ID(7) : linkIndex);
	    ++link)
	{
		if(enable)
			getDTC()->EnableLink(link);
		else
			getDTC()->DisableLink(link);
	}

	__SET_ARG_OUT__("Result", getDTC()->FormatLinkEnable());
}  //end EnableDTCLink()

//========================================================================
void DTCFrontEndInterface::ResetDTCLinks(__ARGS__)
{
	getDTC()->ResetSERDESTX(DTCLib::DTC_Link_ID::DTC_Link_ALL);
	getDTC()->ResetSERDESRX(DTCLib::DTC_Link_ID::DTC_Link_ALL);
	getDTC()->ResetSERDES(DTCLib::DTC_Link_ID::DTC_Link_ALL);
}  //end ResetDTCLinks()

//========================================================================
void DTCFrontEndInterface::ConfigureForTimingChain(__ARGS__)
{
	//call virtual readStatus

	int stepIndex = __GET_ARG_IN__("StepIndex", int);

	// do 0, then 1
	if(stepIndex == -1)
	{
		for(int i = 0; i < CONFIG_DTC_TIMING_CHAIN_STEPS; ++i)
		{
			configureForTimingChain(i);
			usleep(1000);
		}
	}
	else
		configureForTimingChain(stepIndex);

}  //end ConfigureForTimingChain()

//========================================================================
void DTCFrontEndInterface::ResetCFOLinkRx(__ARGS__)
{
	getDTC()->ResetSERDESRX(DTCLib::DTC_Link_ID::DTC_Link_CFO);
}  //end ResetCFOLinkRx()
//========================================================================
void DTCFrontEndInterface::ResetCFOLinkTx(__ARGS__)
{
	getDTC()->ResetSERDESTX(DTCLib::DTC_Link_ID::DTC_Link_CFO);
}  //end ReseResetCFOLinkTxtCFORx()
//========================================================================
void DTCFrontEndInterface::ResetCFOLinkRxPLL(__ARGS__)
{
	getDTC()->ResetSERDESPLL(DTCLib::DTC_PLL_ID::DTC_PLL_CFO_RX);
}  //end ResetCFOLinkRxPLL()
//========================================================================
void DTCFrontEndInterface::ResetCFOLinkTxPLL(__ARGS__)
{
	getDTC()->ResetSERDESPLL(DTCLib::DTC_PLL_ID::DTC_PLL_CFO_TX);
}  //end ResetCFOLinkTxPLL()

//========================================================================
void DTCFrontEndInterface::GetDTCIdAndEVBInfo(__ARGS__)
{
	__SET_ARG_OUT__("Result",
	                getDTC()->FormatEVBLocalParitionIDMACIndex() + std::string("\n") +
	                    getDTC()->FormatEVBClusterInfo());
}  //end GetDTCIdAndEVBInfo()

//========================================================================
void DTCFrontEndInterface::SetDTCIdAndEVBInfo(__ARGS__)
{
	uint8_t DTCid        = __GET_ARG_IN__("DTC ID", uint8_t);
	uint8_t evbMode      = __GET_ARG_IN__("EVB Mode", uint8_t);
	uint8_t evbPartition = __GET_ARG_IN__("EVB Partition ID", uint8_t);
	uint8_t evbMAC       = __GET_ARG_IN__("EVB Self MAC Address Last Byte", uint8_t);

	__FE_COUTV__((int)DTCid);
	__FE_COUTV__((int)evbMode);
	__FE_COUTV__((int)evbPartition);
	__FE_COUTV__((int)evbMAC);

	getDTC()->SetEVBInfo(DTCid, evbMode, evbPartition, evbMAC);

	uint16_t deadTime       = __GET_ARG_IN__("EVB Dead Time in Cluster", uint16_t);
	uint8_t  NumOfDTCs      = __GET_ARG_IN__("EVB Number of DTCs in Cluster", uint8_t, 1);
	uint8_t  evbBaseAddress = __GET_ARG_IN__("EVB Cluster Base DTC MAC Address", uint8_t);

	__FE_COUTV__(deadTime);
	__FE_COUTV__((int)NumOfDTCs);
	if(NumOfDTCs == 0)
	{
		__FE_SS__ << "Invalid input for Number of DTCs in Cluster: " << (int)NumOfDTCs
		          << ". This value must be at least 1." << __E__;
		__FE_SS_THROW__;
	}
	__FE_COUTV__((int)evbBaseAddress);
	getDTC()->SetEVBClusterInfo(deadTime, evbBaseAddress, NumOfDTCs);

	getDTC()
	    ->SoftReset();  //to invalidate destination address cycles, now need the first Event Window Marker to synchronize

	__SET_ARG_OUT__("Result",
	                getDTC()->FormatEVBLocalParitionIDMACIndex() + std::string("\n") +
	                    getDTC()->FormatEVBClusterInfo());
}  //end SetDTCIdAndEVBInfo()

// //========================================================================
// void DTCFrontEndInterface::ResetEVBLinkRx(__ARGS__)
// {
// 	getDTC()->ResetSERDESRX(DTCLib::DTC_Link_ID::DTC_Link_EVB);
// } //end ResetEVBLinkRx()
// //========================================================================
// void DTCFrontEndInterface::ResetEVBLinkTx(__ARGS__)
// {
// 	getDTC()->ResetSERDESTX(DTCLib::DTC_Link_ID::DTC_Link_EVB);
// } //end ReseResetEVBLinkTxtEVBRx()
// //========================================================================
// void DTCFrontEndInterface::ResetEVBLinkRxTxPLL(__ARGS__)
// {
// 	getDTC()->ResetSERDESPLL(DTCLib::DTC_PLL_ID::DTC_PLL_EVB_TXRX);
// } //end ResetEVBLinkRxTxPLL()

//========================================================================
void DTCFrontEndInterface::SetupCFOInterface(__ARGS__)
{
	__SET_ARG_OUT__(
	    "Result",
	    SetupCFOInterface(
	        __GET_ARG_IN__("Force External CFO Sample Clock Edge (0 for rising-edge, 1 "
	                       "for falling-edge, 2 for auto-find, Default := 0)",
	                       int,
	                       0),
	        __GET_ARG_IN__(
	            "Put DTC in CFO Emulation Mode (Default := false)", bool, false),
	        __GET_ARG_IN__(
	            "Also setup Jitter Attenuator (Default := false)", bool, false),
	        __GET_ARG_IN__("Set Link RX/TX Enable (Default := false)", bool, false),
	        __GET_ARG_IN__(
	            "Enable Auto-generation of Data Request Packets (Default := false)",
	            bool,
	            false),
	        __GET_ARG_IN__("Permanent Offset (-2 to 2, Default := 0)", int, 0)));
}  //end SetupCFOInterface()

//========================================================================
std::string DTCFrontEndInterface::SetupCFOInterface(int  forceCFOedge,
                                                    bool useCFOemulator,
                                                    bool alsoSetupJA,
                                                    bool cfoRxTxEnable,
                                                    bool enableAutogenDRP,
                                                    int  permanentOffset /* = 0 */)
{
	std::stringstream outSs;
	__FE_COUTV__(forceCFOedge);

	getDTC()->DisableCFOEmulation();
	getDTC()->SetExternalCFOSampleEdgeMode(forceCFOedge);  //forceCFOedge is a 2-bit value

	__FE_COUTV__(useCFOemulator);

	if(useCFOemulator)
	{
		outSs << "Setting up CFO emulator...\n\n";
		getDTC()->SetCFOEmulationMode();

		if(alsoSetupJA)
		{
			//force JA from local osc
			getCFOandDTCRegisters()->SetJitterAttenuatorSelect(0 /* select local osc */,
			                                                   false /* alsoResetJA */);
			for(int i = 0; i < 10; ++i)  //wait for JA to lock before reading
			{
				if(getCFOandDTCRegisters()->ReadJitterAttenuatorLocked())
					break;
				sleep(1);
			}
			outSs << "JA Status = "
			      << getCFOandDTCRegisters()->FormatJitterAttenuatorCSR() << __E__;
		}
	}
	else  //using external CFO!
	{
		outSs << "Setting up external CFO...\n\n";

		if(alsoSetupJA)
		{
			//force JA from RTF
			getCFOandDTCRegisters()->SetJitterAttenuatorSelect(1 /* select RJ45 */,
			                                                   false /* alsoResetJA */);
			for(int i = 0; i < 10; ++i)  //wait for JA to lock before reading
			{
				if(getCFOandDTCRegisters()->ReadJitterAttenuatorLocked())
					break;
				sleep(1);
			}
			outSs << "JA Status = "
			      << getCFOandDTCRegisters()->FormatJitterAttenuatorCSR() << __E__;
		}

		getDTC()->ClearCFOEmulationMode();
	}

	__FE_COUTV__(cfoRxTxEnable);

	if(cfoRxTxEnable)
	{
		getDTC()->EnableReceiveCFOLink();
		getDTC()->EnableTransmitCFOLink();
	}
	else
	{
		getDTC()->DisableReceiveCFOLink();
		getDTC()->DisableTransmitCFOLink();
	}

	__FE_COUTV__(enableAutogenDRP);

	if(enableAutogenDRP)
		getDTC()->EnableAutogenDRP();
	else
		getDTC()->DisableAutogenDRP();

	__FE_COUTV__(permanentOffset);
	getDTC()->SetCFOSamplePermanentOffset(permanentOffset);

	outSs << getDTC()->FormatDTCControl() << __E__ << getDTC()->FormatCFOLinkError()
	      << __E__;
	__FE_COUT_INFO__ << outSs.str();
	return outSs.str();
}  //end SetupCFOInterface()

//========================================================================
void DTCFrontEndInterface::SetCFOEmulatorOnOffSpillEmulation(__ARGS__)
{
	uint64_t startTag = __GET_ARG_IN__(
	    "Starting Event Window Tag (Default or -1 := start from 0 and continue)",
	    uint64_t,
	    -1);
	if(startTag == (uint64_t)-1)  //if DEFAULT, then continue from next tag position
	{
		__FE_COUTV__(next_starting_cfoem_event_window_tag_);
		startTag = next_starting_cfoem_event_window_tag_;
	}
	//else take user input

	__FE_COUTV__(startTag);

	uint32_t numberOfSuperCycles = __GET_ARG_IN__(
	    "Number of 1.4s super cycle repetitions (0 := infinite)", uint32_t);
	__FE_COUTV__(numberOfSuperCycles);

	//setup next tag calculation (245000 events per super cycle: 235K on-spill + 10K off-spill)
	next_starting_cfoem_event_window_tag_ = startTag + numberOfSuperCycles * 245000;
	__FE_COUTV__(next_starting_cfoem_event_window_tag_);

	__SET_ARG_OUT__(
	    "Result",
	    SetCFOEmulatorOnOffSpillEmulation(
	        __GET_ARG_IN__("Enable CFO Emulator (Default := true)", bool, true),
	        __GET_ARG_IN__("Use Detached Buffer Test (Default := false)", uint32_t),
	        numberOfSuperCycles,
	        startTag,
	        __GET_ARG_IN__("Enable Clock Markers (Default := false)", bool, false),
	        __GET_ARG_IN__(
	            "Enable Auto-generation of Data Request Packets (Default := false)",
	            bool,
	            false),
	        __GET_ARG_IN__(
	            "For Detached Buffer Test, Save Binary Data to File (Default: false)",
	            bool),
	        __GET_ARG_IN__("For Detached Buffer Test, Save Binary Data Filename",
	                       std::string),
	        __GET_ARG_IN__("For Detached Buffer Test, Save Subevent Header to Binary "
	                       "File (Default: false)",
	                       bool),
	        __GET_ARG_IN__(
	            "For Detached Buffer Test, Do NOT Reset Counters (Default: false)", bool),
	        __GET_ARG_IN__("For Detached Buffer Test, Skip-by-32 to Emulate Event "
	                       "Building (Default: false)",
	                       bool),
	        __GET_ARG_IN__("For Detached Buffer Test, Payload Packet Threshold for "
	                       "Saving Event (Default: 0)",
	                       uint32_t)));
}  //end SetCFOEmulatorOnOffSpillEmulation()

//========================================================================
// OnOff spill Run Plan is represented as 235K on-spill events and 10K off-spill events
std::string DTCFrontEndInterface::SetCFOEmulatorOnOffSpillEmulation(
    bool               enable,
    bool               useDetachedBufferTest,
    uint32_t           numberOfSuperCycles,
    uint64_t           initialEventWindowTag,
    bool               enableClockMarkers,
    bool               enableAutogenDRP,
    bool               saveBinaryDataToFile,
    const std::string& filename,
    bool               saveSubeventHeadersToDataFile,
    bool               doNotResetCounters,
    bool               skipBy32,
    uint32_t           packetThresholdToSave)
{
	__FE_COUTV__(enable);

	std::stringstream outSs;

	getDTC()->DisableCFOEmulation();
	getDTC()->DisableAutogenDRP();
	if(!enable)  //do not need to apply parameters if disabling
	{
		outSs << "Halted CFO Emulator!" << __E__;
		return outSs.str();
	}
	//else enabling, so apply parameters, then enable

	getDTC()->SoftReset();  //to reset event window tag starting point handling

	//ASSUME release buffers is handled by the reading code
	// // release of all the buffers
	// if(!useDetachedBufferTest)
	//	getDevice()->read_release(DTC_DMA_Engine_DAQ, 100);

	if(useDetachedBufferTest)
		initDetachedBufferTest(initialEventWindowTag,
		                       saveBinaryDataToFile,
		                       filename,
		                       saveSubeventHeadersToDataFile,
		                       doNotResetCounters,
		                       skipBy32,
		                       packetThresholdToSave);

	//If Event Window duration = 0, this specifies to execute the On/Off Spill emulation of Event Window intervals.
	getDTC()->SetCFOEmulationEventWindowInterval(0);

	__FE_COUTV__(numberOfSuperCycles);
	getDTC()->SetCFOEmulationNumHeartbeats(numberOfSuperCycles);

	__FE_COUTV__(initialEventWindowTag);
	getDTC()->SetCFOEmulationTimestamp(DTCLib::DTC_EventWindowTag(initialEventWindowTag));

	__FE_COUTV__(enableClockMarkers);
	getDTC()->SetCFO40MHzClockMarkerEnable(DTCLib::DTC_Link_ID::DTC_Link_ALL,
	                                       enableClockMarkers);

	__FE_COUTV__(skipBy32);
	if(skipBy32)
		getDTC()->EnableDropDataToEmulateEventBuilding();
	else
		getDTC()->DisableDropDataToEmulateEventBuilding();

	__FE_COUTV__(enableAutogenDRP);
	if(enableAutogenDRP)
		getDTC()->EnableAutogenDRP();
	else
		getDTC()->DisableAutogenDRP();

	getDTC()->EnableReceiveCFOLink();  //enable forwarding if CFO timing link to ROCs

	__COUTT__ << "Enabling CFO Emulation!" << __E__;
	getDTC()->EnableCFOEmulation();

	outSs << "Launched CFO Emulator!" << __E__;
	return outSs.str();
}  //end SetCFOEmulatorOnOffSpillEmulation()

//========================================================================
void DTCFrontEndInterface::SoftwareDataRequest(__ARGS__)
{
	uint64_t when = __GET_ARG_IN__("Event Window Tag", uint64_t);
	//uint8_t link	= __GET_ARG_IN__("LinkIndex", uint8_t);

	//getDTC()->SendDataRequestPacket(DTCLib::DTC_Link_ID(link),
	//				  DTCLib::DTC_EventWindowTag(when),
	//				  true, false); // link, EVT, quiet, debug

	getDTC()->EnableSoftwareDRP();
	getDTC()->SetSoftwareDataRequest(DTCLib::DTC_EventWindowTag(when));

	std::stringstream outSs;
	outSs << "Sent software DR for EVT " << std::hex << when << __E__;
	__SET_ARG_OUT__("Result", outSs.str());

}  // end SoftwareDataRequest()

void DTCFrontEndInterface::PunchedClock(__ARGS__)
{
	auto enable = __GET_ARG_IN__("Enable (Default := true)", bool, true);
	if(enable)
		getDTC()->SetPunchEnable();
	else
		getDTC()->ClearPunchEnable();
}  // end PunchedClock()

//========================================================================

//========================================================================
void DTCFrontEndInterface::SetCFOEmulatorFixedWidthEmulation(__ARGS__)
{
	uint64_t startTag = __GET_ARG_IN__(
	    "Starting Event Window Tag (Default or -1 := start from 0 and continue)",
	    uint64_t,
	    -1);
	if(startTag == (uint64_t)-1)  //if DEFAULT, then continue from next tag position
	{
		__FE_COUTV__(next_starting_cfoem_event_window_tag_);
		startTag = next_starting_cfoem_event_window_tag_;
	}
	//else take user input

	__FE_COUTV__(startTag);

	uint32_t numberOfEventWindowMarkers = __GET_ARG_IN__(
	    "Number of Event Window Markers to generate (0 := infinite)", uint32_t);
	__FE_COUTV__(numberOfEventWindowMarkers);

	//setup next tag calculation
	next_starting_cfoem_event_window_tag_ = startTag + numberOfEventWindowMarkers;
	__FE_COUTV__(next_starting_cfoem_event_window_tag_);

	__SET_ARG_OUT__(
	    "Result",
	    SetCFOEmulatorFixedWidthEmulation(
	        __GET_ARG_IN__("Enable CFO Emulator (Default := true)", bool, true),
	        __GET_ARG_IN__("Use Detached Buffer Test (Default := false)", bool),
	        __GET_ARG_IN__("Fixed-width Event Window Duration (s, ms, us, ns, and clocks "
	                       "allowed) [clocks := 25ns]",
	                       std::string,
	                       "0x44 clocks"),
	        numberOfEventWindowMarkers,
	        startTag,
	        __GET_ARG_IN__("Event Window Mode (Default := 1)", uint64_t, 1),
	        __GET_ARG_IN__("Enable Clock Markers (Default := false)", bool, false),
	        __GET_ARG_IN__(
	            "Enable Auto-generation of Data Request Packets (Default := false)",
	            bool,
	            false),
	        __GET_ARG_IN__(
	            "For Detached Buffer Test, Save Binary Data to File (Default: false)",
	            bool),
	        __GET_ARG_IN__("For Detached Buffer Test, Save Binary Data Filename",
	                       std::string),
	        __GET_ARG_IN__("For Detached Buffer Test, Save Subevent Header to Binary "
	                       "File (Default: false)",
	                       bool),
	        __GET_ARG_IN__(
	            "For Detached Buffer Test, Do NOT Reset Counters (Default: false)", bool),
	        __GET_ARG_IN__("For Detached Buffer Test, Skip-by-32 to Emulate Event "
	                       "Building (Default: false)",
	                       bool),
	        __GET_ARG_IN__("For Detached Buffer Test, Payload Packet Threshold for "
	                       "Saving Event (Default: 0)",
	                       uint32_t)));
}  //end SetCFOEmulatorFixedWidthEmulation()

//========================================================================
std::string DTCFrontEndInterface::SetCFOEmulatorFixedWidthEmulation(
    bool               enable,
    bool               useDetachedBufferTest,
    const std::string& eventDuration,
    uint32_t           numberOfEventWindowMarkers,
    uint64_t           initialEventWindowTag,
    uint64_t           eventWindowMode,
    bool               enableClockMarkers,
    bool               enableAutogenDRP,
    bool               saveBinaryDataToFile,
    const std::string& filename,
    bool               saveSubeventHeadersToDataFile,
    bool               doNotResetCounters,
    bool               skipBy32,
    uint32_t           packetThresholdToSave)
{
	__FE_COUTV__(enable);

	std::stringstream outSs;

	getDTC()->DisableCFOEmulation();
	getDTC()->DisableAutogenDRP();
	if(!enable)  //do not need to apply parameters if disabling
	{
		outSs << "Halted CFO Emulator!" << __E__;
		return outSs.str();
	}
	//else enabling, so apply parameters, then enable

	getDTC()->SoftReset();  //to reset event window tag starting point handling

	//ASSUME release buffers is handled by the reading code
	// // release of all the buffers
	// if(!useDetachedBufferTest)
	//	getDevice()->read_release(DTC_DMA_Engine_DAQ, 100);

	if(useDetachedBufferTest)
		initDetachedBufferTest(initialEventWindowTag,
		                       saveBinaryDataToFile,
		                       filename,
		                       saveSubeventHeadersToDataFile,
		                       doNotResetCounters,
		                       skipBy32,
		                       packetThresholdToSave);

	__FE_COUTV__(eventDuration);
	bool   foundUnits = false;
	size_t i;
	for(i = 0; i < eventDuration.size(); ++i)
		if(eventDuration[i] == 's' || eventDuration[i] == 'm' ||
		   eventDuration[i] == 'u' || eventDuration[i] == 'n' || eventDuration[i] == 'c')
		{
			foundUnits = true;
			break;
		}

	if(!foundUnits)
	{
		__FE_SS__
		    << "No units were found in the input parameters 'Fixed-width Event Window "
		       "Duration' value: "
		    << eventDuration
		    << ". Please use units when specifying event window duration (s, ms, us, ns, "
		       "and clocks are allowed). For example '1.7us' or '1675ns' would be valid."
		    << __E__;
		__FE_SS_THROW__;
	}
	std::string eventDurationSplitNumber = eventDuration.substr(0, i);
	std::string eventDurationSplitUnits  = eventDuration.substr(i);
	__FE_COUTV__(eventDurationSplitNumber);
	__FE_COUTV__(eventDurationSplitUnits);

	//copied from CFO_Compiler.cpp::transcribeInstructions() [L494]
	uint64_t value;
	if(!StringMacros::getNumber(eventDurationSplitNumber, value))
	{
		__FE_SS__ << "The duration parameter value '" << eventDurationSplitNumber << " "
		          << eventDurationSplitUnits << "' is not a valid number. "
		          << "Use 0x### to indicate hex and b### to indicate binary; otherwise, "
		             "decimal is inferred."
		          << __E__;
		__FE_SS_THROW__;
	}
	//test floating point in case integer conversion dropped something
	double timeValue = strtod(eventDurationSplitNumber.c_str(), 0);
	__FE_COUTV__(timeValue);
	if(timeValue < value)
		timeValue = value;

	__FE_COUTV__(FPGAClock_);
	__FE_COUTV__(value);
	__FE_COUTV__(timeValue);

	uint32_t eventDurationInClocks;

	if(eventDurationSplitUnits == "s")  // Wait wanted in seconds
		eventDurationInClocks = timeValue * 1e9 / FPGAClock_;
	else if(eventDurationSplitUnits == "ms")  // Wait wanted in milliseconds
		eventDurationInClocks = timeValue * 1e6 / FPGAClock_;
	else if(eventDurationSplitUnits == "us")  // Wait wanted in microseconds
		eventDurationInClocks = timeValue * 1e3 / FPGAClock_;
	else if(eventDurationSplitUnits == "ns")  // Wait wanted in nanoseconds
	{
		if((value % FPGAClock_) != 0)
		{
			__FE_SS__ << "FPGA can only wait in multiples of " << FPGAClock_
			          << " ns: the input event duration value '" << value
			          << "' yields a remainder of " << (value % FPGAClock_) << __E__;
			__FE_SS_THROW__;
		}
		eventDurationInClocks = value / FPGAClock_;
	}
	else if(eventDurationSplitUnits == "clocks")  // Wait wanted in FPGA clocks
		eventDurationInClocks = value;
	else  //impossible
	{
		__FE_SS__ << "The event duration input parameter is missing a valid unit type "
		             "after parameter: "
		          << eventDurationSplitUnits
		          << ". Accepted unit types are clocks, ns, us, ms, and s." << __E__;
		__FE_SS_THROW__;
	}
	if(eventDurationInClocks < 40)
	{
		__FE_SS__ << "The event duration input parameter can not evaluate to less than "
		             "40 clocks (1000ns). The input value '"
		          << eventDurationSplitNumber << " " << eventDurationSplitUnits
		          << "' evaluates to " << eventDurationInClocks << "clocks < 40."
		          << __E__;
		__FE_SS_THROW__;
	}

	__FE_COUTV__(eventDurationInClocks);
	getDTC()->SetCFOEmulationEventWindowInterval(eventDurationInClocks);

	__FE_COUTV__(numberOfEventWindowMarkers);
	getDTC()->SetCFOEmulationNumHeartbeats(numberOfEventWindowMarkers);

	__FE_COUTV__(initialEventWindowTag);
	getDTC()->SetCFOEmulationTimestamp(DTCLib::DTC_EventWindowTag(initialEventWindowTag));

	__FE_COUTV__(eventWindowMode);
	getDTC()->SetCFOEmulationEventMode(eventWindowMode);

	__FE_COUTV__(enableClockMarkers);
	getDTC()->SetCFO40MHzClockMarkerEnable(DTCLib::DTC_Link_ID::DTC_Link_ALL,
	                                       enableClockMarkers);

	__FE_COUTV__(skipBy32);
	if(skipBy32)
		getDTC()->EnableDropDataToEmulateEventBuilding();
	else
		getDTC()->DisableDropDataToEmulateEventBuilding();

	__FE_COUTV__(enableAutogenDRP);
	if(enableAutogenDRP)
		getDTC()->EnableAutogenDRP();
	else
		getDTC()->DisableAutogenDRP();

	getDTC()->EnableReceiveCFOLink();  //enable forwarding if CFO timing link to ROCs

	// If the detached buffer-test thread is running, wait until it has finished
	// its ReleaseAllBuffers() before turning on CFO emulation — otherwise emulation
	// can start filling DMA buffers while the driver is still draining the old ones.
	if(bufferTestThreadStruct_ && bufferTestThreadStruct_->running_)
	{
		constexpr int kWaitMs    = 10;
		constexpr int kTimeoutMs = 7000;  // > release_all's 5s internal cap, with slack
		int           waited     = 0;
		while(!bufferTestThreadStruct_->releaseAllComplete_ && waited < kTimeoutMs)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(kWaitMs));
			waited += kWaitMs;
		}
		if(!bufferTestThreadStruct_->releaseAllComplete_)
		{
			__SS__ << "Timed out (" << kTimeoutMs
			       << " ms) waiting for buffer-test thread ReleaseAllBuffers to complete!"
			       << __E__;
			__SS_THROW__;
		}
		else
			__COUTT__ << "Buffer-test thread ReleaseAllBuffers complete after " << waited
			          << " ms; proceeding to enable CFO emulation." << __E__;
	}

	__COUTT__ << "Enabling CFO Emulation!" << __E__;
	getDTC()->EnableCFOEmulation();

	outSs << "Launched CFO Emulator!" << __E__;
	return outSs.str();  //__SET_ARG_OUT__("Result", outSs.str());

}  //end SetCFOEmulatorFixedWidthEmulation()

//==============================================================================
void DTCFrontEndInterface::initDetachedBufferTest(
    uint64_t           initialEventWindowTag,
    bool               saveBinaryDataToFile,
    const std::string& saveBinaryDataFilename,
    bool               saveSubeventHeadersToDataFile,
    bool               doNotResetCounters,
    bool               skipBy32,
    uint32_t           packetThresholdToSave)
{
	__FE_COUTV__(saveBinaryDataToFile);
	__FE_COUTV__(doNotResetCounters);
	__FE_COUTV__(skipBy32);
	__FE_COUTV__(packetThresholdToSave);
	__FE_COUT__ << "Initializing detached buffer test!" << __E__;

	if(!bufferTestThreadStruct_)  //initialize shared pointer for first time
		bufferTestThreadStruct_ =
		    std::make_shared<DTCFrontEndInterface::DetachedBufferTestThreadStruct>();

	if(bufferTestThreadStruct_->running_)
	{
		__FE_COUT__ << "Found buffer test thread already running... so re-initializing"
		            << __E__;

		// start mutex scope
		{
			std::lock_guard<std::mutex> lock(bufferTestThreadStruct_->lock_);
			bufferTestThreadStruct_->inSubeventMode_         = true;
			bufferTestThreadStruct_->activeMatch_            = false;
			bufferTestThreadStruct_->expectedEventTag_       = initialEventWindowTag;
			bufferTestThreadStruct_->saveBinaryData_         = saveBinaryDataToFile;
			bufferTestThreadStruct_->saveBinaryDataFilename_ = saveBinaryDataFilename;
			bufferTestThreadStruct_->saveSubeventHeadersToBinaryData_ =
			    saveSubeventHeadersToDataFile;
			bufferTestThreadStruct_->exitThread_         = false;
			bufferTestThreadStruct_->resetStartEventTag_ = true;
			bufferTestThreadStruct_->releaseAllComplete_ =
			    false;  // arm; thread will set true after its next ReleaseAllBuffers
			bufferTestThreadStruct_->doNotResetCounters_    = doNotResetCounters;
			bufferTestThreadStruct_->skipBy32_              = skipBy32;
			bufferTestThreadStruct_->packetThresholdToSave_ = packetThresholdToSave;
		}
		__FE_COUT__ << "Found buffer test thread already running... so re-initializing "
		               "and reading data starting at event tag "
		            << initialEventWindowTag << " (0x" << std::hex
		            << initialEventWindowTag << ")" << __E__;
	}
	else
	{
		__FE_COUT__ << "Launching detached Buffer Test thread..." << __E__;
		// start mutex scope
		{
			std::lock_guard<std::mutex> lock(bufferTestThreadStruct_->lock_);
			bufferTestThreadStruct_->inSubeventMode_         = true;
			bufferTestThreadStruct_->activeMatch_            = false;
			bufferTestThreadStruct_->expectedEventTag_       = initialEventWindowTag;
			bufferTestThreadStruct_->saveBinaryData_         = saveBinaryDataToFile;
			bufferTestThreadStruct_->saveBinaryDataFilename_ = saveBinaryDataFilename;
			bufferTestThreadStruct_->saveSubeventHeadersToBinaryData_ =
			    saveSubeventHeadersToDataFile;
			bufferTestThreadStruct_->exitThread_         = false;
			bufferTestThreadStruct_->resetStartEventTag_ = false;
			bufferTestThreadStruct_->releaseAllComplete_ =
			    false;  // arm; thread will set true after its initial ReleaseAllBuffers
			bufferTestThreadStruct_->thisDTC_               = thisDTC_;
			bufferTestThreadStruct_->running_               = true;
			bufferTestThreadStruct_->error_                 = "";
			bufferTestThreadStruct_->doNotResetCounters_    = false;
			bufferTestThreadStruct_->skipBy32_              = skipBy32;
			bufferTestThreadStruct_->packetThresholdToSave_ = packetThresholdToSave;
		}
		std::thread(
		    [](std::shared_ptr<DTCFrontEndInterface::DetachedBufferTestThreadStruct>
		           threadStruct) {
			    DTCFrontEndInterface::detachedBufferTestThread(threadStruct);
		    },
		    bufferTestThreadStruct_)
		    .detach();
		__FE_COUT__ << "Launched detached Buffer Test thread and reading data DMA-0 "
		               "starting at event tag "
		            << initialEventWindowTag << " (0x" << std::hex
		            << initialEventWindowTag << ")" << __E__;
	}

	sleep(1);  //give time for buffer reading to be ready
}  //end initDetachedBufferTest()

//==============================================================================
uint64_t DTCFrontEndInterface::getDetachedBufferTestReceivedCount(
    std::shared_ptr<DTCFrontEndInterface::DetachedBufferTestThreadStruct> threadStruct)
{
	if(!threadStruct->inSubeventMode_)
		return threadStruct->eventsCount_;
	else
		return threadStruct->subeventsCount_;
}  //end getDetachedBufferTestReceivedCount()

//==============================================================================
std::string DTCFrontEndInterface::getDetachedBufferTestStatus(
    std::shared_ptr<DTCFrontEndInterface::DetachedBufferTestThreadStruct> threadStruct)
{
	//use mfSubject_ to label FE UID in GEN output macros
	std::string mfSubject_ = LOCAL_COUT_HDR;
	__GEN_COUT__ << "Get detached buffer test status..." << __E__;

	std::stringstream statusSs;

	// start mutex scope
	{
		std::lock_guard<std::mutex> lock(threadStruct->lock_);
		__GEN_COUT__ << "Have lock to read..." << __E__;

		if(threadStruct->error_ != "")
			statusSs << "Detached thread caught error:" << threadStruct->error_ << __E__;
		statusSs << "Detached thread running:"
		         << (threadStruct->running_ ? "true" : "false") << __E__;

		if(threadStruct->saveBinaryData_)
			statusSs << "Output file:"
			         << (std::string(__ENV__("OTSDAQ_DATA")) + "/" +
			             threadStruct->saveBinaryDataFilename_)
			         << __E__;

		statusSs << "Events count:" << threadStruct->eventsCount_ << __E__;
		statusSs << "Subevents count:" << threadStruct->subeventsCount_ << __E__;

		if(threadStruct->saveBinaryData_ && threadStruct->packetThresholdToSave_ > 0)
			statusSs << "Saved " << (threadStruct->inSubeventMode_ ? "subevent" : "event")
			         << " count:" << threadStruct->savedCount_
			         << " (data packet threshold = "
			         << threadStruct->packetThresholdToSave_ << ")" << __E__;

		statusSs << "Total Subevent Bytes Transferred (including Headers): "
		         << threadStruct->totalSubeventBytesTransferred_ << __E__;

		long long ns =
		    std::chrono::duration_cast<std::chrono::nanoseconds>(
		        threadStruct->transferEndTime_ - threadStruct->transferStartTime_)
		        .count();
		if(ns > 1000)  //prevent divide by 0
		{
			statusSs << "Data Transfer Duration: " << ns / 1000.0 / 1000.0 << " ms"
			         << __E__;
			statusSs << "Average Data Rate: "
			         << ((double)threadStruct->totalSubeventBytesTransferred_) /
			                (ns / 1000.0)
			         << " MB/s" << __E__;
		}
		else
			statusSs << "Data Transfer Duration too short to establish date rate."
			         << __E__;

		statusSs << "Starting Event Window Tag:" << threadStruct->expectedEventTag_
		         << __E__;
		statusSs << "Next Expected Event Window Tag:" << threadStruct->nextEventWindowTag_
		         << __E__;
		if(ns > 1000)  //prevent divide by 0
		{
			statusSs << "Average Event Rate: "
			         << (long long)((threadStruct->nextEventWindowTag_ -
			                         threadStruct->expectedEventTag_) /
			                        (ns / 1000.0 / 1000.0 / 1000.0))
			         << " Events/s" << __E__;
		}
		statusSs << "Mismatched Event Tags count:"
		         << threadStruct->mismatchedEventTagsCount_ << __E__;

		if(threadStruct->mismatchedEventTagJumps_.size() < 20)
		{
			statusSs << "\t Mismatched Tag Jumps..." << __E__;
			for(size_t i = 0; i < threadStruct->mismatchedEventTagJumps_.size(); ++i)
				statusSs << "\t\t Mismatch Jump-" << i << " Expected:"
				         << threadStruct->mismatchedEventTagJumps_[i].first << std::hex
				         << "(0x" << threadStruct->mismatchedEventTagJumps_[i].first
				         << ")" << std::dec << " Received:"
				         << threadStruct->mismatchedEventTagJumps_[i].second << std::hex
				         << "(0x" << threadStruct->mismatchedEventTagJumps_[i].second
				         << ")" << std::dec << __E__;
		}
		else
		{
			statusSs << "\t TOO MANY Mismatched Tag Jumps (showing 10)..." << __E__;
			for(size_t i = 0; i < 10; ++i)
				statusSs << "\t\t Mismatch Jump-" << i << " Expected:"
				         << threadStruct->mismatchedEventTagJumps_[i].first << std::hex
				         << "(0x" << threadStruct->mismatchedEventTagJumps_[i].first
				         << ")" << std::dec << " Received:"
				         << threadStruct->mismatchedEventTagJumps_[i].second << std::hex
				         << "(0x" << threadStruct->mismatchedEventTagJumps_[i].second
				         << ")" << std::dec << __E__;
		}

		statusSs << "ROC Fragments..." << __E__;
		for(size_t i = 0; i < threadStruct->rocFragmentsCount_.size(); ++i)
			statusSs << "\t Roc-" << i
			         << " Fragments count:" << threadStruct->rocFragmentsCount_[i]
			         << __E__;

		statusSs << "ROC Payload Empty count..." << __E__;
		for(size_t i = 0; i < threadStruct->rocPayloadEmptyCount_.size(); ++i)
			statusSs << "\t Roc-" << i
			         << " Payload Empty count:" << threadStruct->rocPayloadEmptyCount_[i]
			         << __E__;

		statusSs << "ROC Payload Byte count..." << __E__;
		for(size_t i = 0; i < threadStruct->rocPayloadByteCount_.size(); ++i)
			statusSs << "\t Roc-" << i
			         << " Payload bytes:" << threadStruct->rocPayloadByteCount_[i]
			         << __E__;

		statusSs << "ROC Subevent Header Timeouts..." << __E__;
		for(size_t i = 0; i < threadStruct->rocFragmentTimeoutsCount_.size(); ++i)
			statusSs << "\t Roc-" << i << " Subevent Timeouts count:"
			         << threadStruct->rocFragmentTimeoutsCount_[i] << __E__;

		statusSs << "ROC Fragment Header Timeouts..." << __E__;
		for(size_t i = 0; i < threadStruct->rocHeaderTimeoutsCount_.size(); ++i)
			statusSs << "\t Roc-" << i << " Fragment Header Timeouts count:"
			         << threadStruct->rocHeaderTimeoutsCount_[i] << __E__;

		size_t totalROCerrors = 0;
		statusSs << "ROC Errors (Timeouts + others)..." << __E__;
		for(size_t i = 0; i < threadStruct->rocFragmentErrorsCount_.size(); ++i)
		{
			statusSs << "\t Roc-" << i << " Fragment Errors count:"
			         << threadStruct->rocFragmentErrorsCount_[i] << __E__;
			totalROCerrors += threadStruct->rocFragmentErrorsCount_[i];
		}

		if(threadStruct->error_ != "" || totalROCerrors)
		{
			__SS__ << "Error identified in the detached buffer status";
			if(totalROCerrors)
				ss << ". Check the ROC Errors (Timeouts + others) section for details: ";
			else
				ss << ": ";
			ss << statusSs.str();
			__SS_THROW__;
		}
	}
	__GEN_COUT__ << "Done getting detached buffer test status..." << __E__;

	return statusSs.str();
}  //end getDetachedBufferTestStatus()

//==============================================================================
void DTCFrontEndInterface::handleDetachedSubevent(
    const DTCLib::DTC_SubEvent&                                           subeventIn,
    std::shared_ptr<DTCFrontEndInterface::DetachedBufferTestThreadStruct> threadStruct)
{
	//use mfSubject_ to label FE UID in GEN output macros
	std::string mfSubject_ = LOCAL_COUT_HDR;

	const DTCLib::DTC_SubEvent* subevent = &subeventIn;

	++(threadStruct->subeventsCount_);

#if 1
	// check the subevent tag window
	if(threadStruct->nextEventWindowTag_ !=
	   subevent->GetEventWindowTag().GetEventWindowTag(true))
	{
		++(threadStruct->mismatchedEventTagsCount_);
		std::stringstream ostr;
		ostr << "Mismatched event tag. Expected = " << threadStruct->nextEventWindowTag_
		     << " (0x" << std::hex << std::setw(4) << std::setfill('0')
		     << threadStruct->nextEventWindowTag_ << "), Received = " << std::dec
		     << subevent->GetEventWindowTag().GetEventWindowTag(true) << " (0x"
		     << std::hex << std::setw(4) << std::setfill('0')
		     << subevent->GetEventWindowTag().GetEventWindowTag(true) << ")";
		__GEN_COUTT__ << ostr.str();
		if(threadStruct->mismatchedEventTagJumps_.size() <
		   100)  //else too many, stop recording
			threadStruct->mismatchedEventTagJumps_.push_back(
			    std::make_pair<uint64_t, uint64_t>(
			        threadStruct->nextEventWindowTag_,
			        subevent->GetEventWindowTag().GetEventWindowTag(true)));
		else
			__GEN_COUTT__ << "Too many mismatches ("
			              << threadStruct->mismatchedEventTagJumps_.size()
			              << "), not recording this one." << __E__;

		if(threadStruct->activeMatch_)
		{
			__SS__ << ostr.str();
			__SS_THROW__;
		}
		//to freeze TRACE
		//TRACE_CNTL("modeM",0); // "freeze" like command line 'tmodeM 0'
		// TRACE_CNTL("modeM",1); // "unfreeze" like command line 'tmodeM 1'
	}
#endif

	if(threadStruct->inSubeventMode_)
	{
		if(threadStruct->skipBy32_)
			threadStruct->nextEventWindowTag_ =
			    subevent->GetEventWindowTag().GetEventWindowTag(true) +
			    32;  //increment for next +32
		else
			threadStruct->nextEventWindowTag_ =
			    subevent->GetEventWindowTag().GetEventWindowTag(true) +
			    1;  //increment for next
	}

	// print the subevent header
	// ostr << subevent->GetHeader()->toJson() << std::endl;
	__GEN_COUTT__ << subevent->GetHeader()->toJson() << __E__;

	//start mutex scope to change non-atomic status counters
	std::lock_guard<std::mutex> lock(threadStruct->lock_);

	if(threadStruct->transferStartTime_ ==
	   std::chrono::steady_clock::time_point::min())  //init start time
		threadStruct->transferStartTime_ = std::chrono::steady_clock::now();

	threadStruct->totalSubeventBytesTransferred_ +=
	    sizeof(DTCLib::DTC_SubEventHeader);  //for subevent header

#if 1

	bool doSaveSubevent = threadStruct->saveBinaryData_;
	if(threadStruct->packetThresholdToSave_ > 0)
	{
		//check if payload packet thershold is met
		// iterate over the data blocks and count packets
		uint32_t                           payloadPackets = 0;
		std::vector<DTCLib::DTC_DataBlock> dataBlocks     = subevent->GetDataBlocks();
		for(unsigned int j = 0; j < dataBlocks.size(); ++j)
		{
			// print the data block header
			DTCLib::DTC_DataHeaderPacket* dataHeader = dataBlocks[j].GetHeader().get();
			payloadPackets += (dataHeader->GetByteCount() - 16) / 16;
		}  //end payload packet count

		if(payloadPackets < threadStruct->packetThresholdToSave_)
			doSaveSubevent = false;
	}

	__GEN_COUTTV__(doSaveSubevent);
	__GEN_COUTTV__(threadStruct->saveSubeventHeadersToBinaryData_);

	//save raw subevent header
	if(threadStruct->saveSubeventHeadersToBinaryData_ && doSaveSubevent)
	{
		++(threadStruct->savedCount_);

		auto dataPtr = reinterpret_cast<const uint8_t*>(subevent->GetHeader());
		for(uint32_t l = 0; l < sizeof(DTCLib::DTC_SubEventHeader); l += 4)
		{
			if(threadStruct->fp_)
				fwrite(&dataPtr[l], sizeof(uint32_t), 1, threadStruct->fp_);
			// ostr << "\t0x" << std::hex << std::setw(8) << std::setfill('0') << *((uint32_t *)(&(dataPtr[l]))) << std::endl;
		}
	}  //end save raw subevent header

	// check if there is an error on the link in the subevent header
	for(auto r : DTCLib::DTC_ROC_Links)
	{
		if(subevent->GetHeader()->getLinkStatus(r) > 0)
		{
			// ostr << "Error: " << std::endl;

			std::bitset<8> link_status(subevent->GetHeader()->getLinkStatus(r));
			if(link_status.test(0))
			{
				++(threadStruct->rocFragmentTimeoutsCount_[r]);

				if(threadStruct->rocLinkEnabledLatch_
				       [r])  //only consider the timeout an error, if link is active
					++(threadStruct->rocFragmentErrorsCount_[r]);

				// ostr << "ROC Timeout Error!" << std::endl;
			}
			else
				++(threadStruct->rocFragmentErrorsCount_[r]);

			// if (link0_status.test(2))
			// {
			//	// ostr << "Packet sequence number Error!" << std::endl;
			// }
			// if (link0_status.test(3))
			// {
			//	// ostr << "CRC Error!" << std::endl;
			// }
			// if (link0_status.test(6))
			// {
			//	// ostr << "Fatal Error!" << std::endl;
			// }
		}
	}  //end link status error check

#endif

	// print the number of data blocks
	// ostr << "Number of Data Block (ROC Fragments): " << subevent->GetDataBlockCount() << std::endl;

	// iterate over the data blocks
	std::vector<DTCLib::DTC_DataBlock> dataBlocks = subevent->GetDataBlocks();
	__GEN_COUTTV__(dataBlocks.size());
	if(dataBlocks.size() != 6)
	{
		__GEN_SS__ << "Unexpected number of ROC fragments found in subevent (EWT="
		           << subevent->GetEventWindowTag() << "): " << dataBlocks.size()
		           << " ROC fragments found (expected 6)";
		__GEN_SS_THROW__;
	}

	__COUTTV__(doSaveSubevent);
	for(unsigned int j = 0; j < dataBlocks.size(); ++j)
	{
		// print the data block header
		DTCLib::DTC_DataHeaderPacket* dataHeader = dataBlocks[j].GetHeader().get();
		++(threadStruct->rocFragmentsCount_[dataHeader->GetLinkID()]);

		// ~~~	The Data Header Packet Status 8-bit field is defined as follows ~~~
		// Bit Position	Definition
		// 0	“Event Window has Data” flag indicates detector data present, else No Data for Event Window.
		// 1	“Invalid Event Window Request” flag indicates the ROC did not receive a Heartbeat packet corresponding to this Data Request.
		// 2	“I am corrupt” flag indicates the ROC has lost data or the ability to conduct detector readout has been compromised.
		// 3	“Timeout” flag indicates ROC retrieval of data did not respond before timeout occurred.
		// 4	“Overflow” flag indicates data is good, but not all data could be sent.
		// 7:5	Reserved

#if 1
		if((dataHeader->GetStatus() >> 3) & 1)
			++(threadStruct->rocHeaderTimeoutsCount_[dataHeader->GetLinkID()]);

		// print the data block ROC fragment header raw data
		{
			auto dataPtr =
			    reinterpret_cast<const uint8_t*>(dataBlocks[j].GetRawBufferPointer());
			// ostr << "Data header raw:" << std::endl;
			for(int l = 0; l < 16; l += 4)
			{
				if(doSaveSubevent && threadStruct->fp_)
					fwrite(&dataPtr[l], sizeof(uint32_t), 1, threadStruct->fp_);
				// ostr << "\t0x" << std::hex << std::setw(8) << std::setfill('0') << *((uint32_t *)(&(dataPtr[l]))) << std::endl;
			}
		}
#endif

		// print the data block payload raw data
		{
			threadStruct->rocPayloadByteCount_[dataHeader->GetLinkID()] +=
			    dataHeader->GetByteCount() - 16;
			threadStruct->totalSubeventBytesTransferred_ +=
			    dataHeader->GetByteCount();           //for Data Header + Payload
			if(dataHeader->GetByteCount() - 16 == 0)  //count empty payloads
				++(threadStruct->rocPayloadEmptyCount_[dataHeader->GetLinkID()]);

#if 1
			auto dataPtr = reinterpret_cast<const uint8_t*>(dataBlocks[j].GetData());

			__GEN_COUTTV__(dataHeader->GetByteCount() - 16);
			// if(displayPayloadAtGUI) ostr << "Data payload:" << std::endl;
			for(int l = 0; l < dataHeader->GetByteCount() - 16; l += 4)
			{
				if(doSaveSubevent && threadStruct->fp_)
					fwrite(&dataPtr[l], sizeof(uint32_t), 1, threadStruct->fp_);
				// if(displayPayloadAtGUI) ostr << "\t0x" << std::hex << std::setw(8) << std::setfill('0') << *((uint32_t *)(&(dataPtr[l]))) << std::endl;
			}
#endif
		}

		__GEN_COUTT__ << "Link-" << dataHeader->GetLinkID() << " Fragment #"
		              << threadStruct->rocFragmentsCount_[dataHeader->GetLinkID()]
		              << "\n"
		                 " Timeout #"
		              << threadStruct->rocHeaderTimeoutsCount_[dataHeader->GetLinkID()]
		              << "\n"
		                 " Empty #"
		              << threadStruct->rocPayloadEmptyCount_[dataHeader->GetLinkID()]
		              << "\n"
		              << dataHeader->toJSON() << __E__;
	}  //end Data Block ROC fragment loop

	// ostr << std::endl << std::endl;

	//update end time
	threadStruct->transferEndTime_ = std::chrono::steady_clock::now();
}  //end handleDetachedSubevent()

//==============================================================================
// detachedBufferTestThread
void DTCFrontEndInterface::detachedBufferTestThread(
    std::shared_ptr<DTCFrontEndInterface::DetachedBufferTestThreadStruct> threadStruct)
try
{
	//use mfSubject_ to label FE UID in GEN output macros
	std::string mfSubject_ = LOCAL_COUT_HDR;
	__GEN_COUT__ << "Buffer test thread established..." << __E__;

	if(threadStruct->fp_)
	{
		__GEN_SS__ << "Impossible?! File pointer already initialized?" << __E__;
		__GEN_SS_THROW__;
	}

	if(threadStruct->saveBinaryData_)
	{
		if(threadStruct->saveBinaryDataFilename_ == "Default" ||
		   threadStruct->saveBinaryDataFilename_ == "")
		{
			std::string filename = "macroOutput_" + std::to_string(time(0)) + "_" +
			                       std::to_string(clock()) + ".bin";
			threadStruct->saveBinaryDataFilename_ = filename;
		}
		else  //sanitize string
		{
			std::string tmp = "";
			for(const auto& c : threadStruct->saveBinaryDataFilename_)
				if(c == '/' || c == '\\')
					continue;
				else
					tmp += c;
			if(threadStruct->thisDTC_)
				threadStruct->saveBinaryDataFilename_ =
				    threadStruct->thisDTC_->getDeviceUID() + "_" + tmp;
			else
				threadStruct->saveBinaryDataFilename_ = "SIM_" + tmp;
		}
		__GEN_COUTV__(std::string(__ENV__("OTSDAQ_DATA")) + "/" +
		              threadStruct->saveBinaryDataFilename_);
		threadStruct->fp_ = fopen((std::string(__ENV__("OTSDAQ_DATA")) + "/" +
		                           threadStruct->saveBinaryDataFilename_)
		                              .c_str(),
		                          "wb");
		if(!threadStruct->fp_)
		{
			__GEN_SS__ << "Failed to open file to save macro output '"
			           << (std::string(__ENV__("OTSDAQ_DATA")) + "/" +
			               threadStruct->saveBinaryDataFilename_)
			           << "'..." << __E__;
			__GEN_SS_THROW__;
		}
	}
	//start with clean release
	if(threadStruct->thisDTC_)
	{
		threadStruct->releaseAllComplete_ = false;  // arm flag before the release
		threadStruct->thisDTC_->ReleaseAllBuffers(DTC_DMA_Engine_DAQ);
		threadStruct->releaseAllComplete_ = true;
		__GEN_COUTT__ << "ReleaseAllBuffers called!" << __E__;

		//latch link enable status for use in accounting (ignoring timeout errors)
		auto regVal = threadStruct->thisDTC_->ReadLinkEnabledData();
		for(auto r : DTCLib::DTC_ROC_Links)
		{
			auto re = threadStruct->thisDTC_->ReadLinkEnabled(r, regVal);
			if(re.TransmitEnable && re.ReceiveEnable)
				threadStruct->rocLinkEnabledLatch_[r] = true;
			else
				threadStruct->rocLinkEnabledLatch_[r] = false;
		}
		__GEN_COUTV__(StringMacros::mapToString(threadStruct->rocLinkEnabledLatch_));
	}

	std::vector<std::unique_ptr<DTCLib::DTC_Event>>    events;
	std::vector<std::unique_ptr<DTCLib::DTC_SubEvent>> subevents;
	uint64_t                                           ii = 0;

	// start mutex scope
	{
		std::lock_guard<std::mutex> lock(threadStruct->lock_);
		threadStruct->nextEventWindowTag_.store(
		    threadStruct->expectedEventTag_.load(std::memory_order_relaxed),
		    std::memory_order_relaxed);
		__GEN_COUT_INFO__
		    << "Starting detached buffer test thread looking for Event Window Tag = "
		    << threadStruct->nextEventWindowTag_ << std::endl;

		threadStruct->error_                    = "";
		threadStruct->eventsCount_              = 0;
		threadStruct->subeventsCount_           = 0;
		threadStruct->mismatchedEventTagsCount_ = 0;
		threadStruct->mismatchedEventTagJumps_.clear();
		threadStruct->rocFragmentsCount_             = {0, 0, 0, 0, 0, 0};
		threadStruct->rocPayloadEmptyCount_          = {0, 0, 0, 0, 0, 0};
		threadStruct->rocFragmentTimeoutsCount_      = {0, 0, 0, 0, 0, 0};
		threadStruct->rocFragmentErrorsCount_        = {0, 0, 0, 0, 0, 0};
		threadStruct->rocHeaderTimeoutsCount_        = {0, 0, 0, 0, 0, 0};
		threadStruct->rocPayloadByteCount_           = {0, 0, 0, 0, 0, 0};
		threadStruct->totalSubeventBytesTransferred_ = 0;
		threadStruct->transferStartTime_ = std::chrono::steady_clock::time_point::min();
		threadStruct->transferEndTime_   = std::chrono::steady_clock::time_point::min();
	}

	uint64_t lastCount = 0;

	//------------------------
	while(threadStruct->thisDTC_ && !threadStruct->exitThread_)
	{
		//check for new starting event tag
		{
			if(threadStruct->resetStartEventTag_)
			{
				if(threadStruct->doNotResetCounters_)
					__GEN_COUT_INFO__
					    << "NOT Resetting counters; previous status was as follows: \n"
					    << getDetachedBufferTestStatus(threadStruct) << __E__;
				else
					__GEN_COUT_INFO__
					    << "Resetting counters; previous status was as follows: \n"
					    << getDetachedBufferTestStatus(threadStruct) << __E__;

				// start mutex scope
				{
					std::lock_guard<std::mutex> lock(threadStruct->lock_);
					threadStruct->nextEventWindowTag_.store(
					    threadStruct->expectedEventTag_.load(std::memory_order_relaxed),
					    std::memory_order_relaxed);
					__GEN_COUT_INFO__
					    << "Restarting detached buffer test thread looking for "
					       "Event Window Tag = "
					    << threadStruct->nextEventWindowTag_ << std::endl;

					__GEN_COUTV__(threadStruct->saveBinaryDataFilename_);

					//reset counts and (re)open file
					if(!threadStruct->doNotResetCounters_)
					{
						//close and reopen file
						if(threadStruct->fp_)
						{
							fclose(threadStruct->fp_);
							threadStruct->fp_ = nullptr;
						}

						if(threadStruct->saveBinaryData_)
						{
							if(threadStruct->saveBinaryDataFilename_ == "Default" ||
							   threadStruct->saveBinaryDataFilename_ == "")
							{
								std::string filename = "macroOutput_" +
								                       std::to_string(time(0)) + "_" +
								                       std::to_string(clock()) + ".bin";
								threadStruct->saveBinaryDataFilename_ = filename;
							}
							else  //sanitize string
							{
								std::string tmp = "";
								for(const auto& c : threadStruct->saveBinaryDataFilename_)
									if(c == '/' || c == '\\')
										continue;
									else
										tmp += c;
								if(threadStruct->thisDTC_)
									threadStruct->saveBinaryDataFilename_ =
									    threadStruct->thisDTC_->getDeviceUID() + "_" +
									    tmp;
								else
									threadStruct->saveBinaryDataFilename_ = "SIM_" + tmp;
							}
							__GEN_COUTV__(std::string(__ENV__("OTSDAQ_DATA")) + "/" +
							              threadStruct->saveBinaryDataFilename_);
							threadStruct->fp_ =
							    fopen((std::string(__ENV__("OTSDAQ_DATA")) + "/" +
							           threadStruct->saveBinaryDataFilename_)
							              .c_str(),
							          "wb");
							if(!threadStruct->fp_)
							{
								__SS__ << "Failed to open file to save macro output '"
								       << (std::string(__ENV__("OTSDAQ_DATA")) + "/" +
								           threadStruct->saveBinaryDataFilename_)
								       << "'..." << __E__;
								__SS_THROW__;
							}
						}

						threadStruct->error_                    = "";
						threadStruct->eventsCount_              = 0;
						threadStruct->subeventsCount_           = 0;
						threadStruct->mismatchedEventTagsCount_ = 0;
						threadStruct->mismatchedEventTagJumps_.clear();
						threadStruct->rocFragmentsCount_             = {0, 0, 0, 0, 0, 0};
						threadStruct->rocPayloadEmptyCount_          = {0, 0, 0, 0, 0, 0};
						threadStruct->rocFragmentTimeoutsCount_      = {0, 0, 0, 0, 0, 0};
						threadStruct->rocFragmentErrorsCount_        = {0, 0, 0, 0, 0, 0};
						threadStruct->rocHeaderTimeoutsCount_        = {0, 0, 0, 0, 0, 0};
						threadStruct->rocPayloadByteCount_           = {0, 0, 0, 0, 0, 0};
						threadStruct->totalSubeventBytesTransferred_ = 0;
						threadStruct->transferStartTime_ =
						    std::chrono::steady_clock::time_point::min();
						threadStruct->transferEndTime_ =
						    std::chrono::steady_clock::time_point::min();
					}
					else  //do not reset counters and do not close file unless no longer saving binary data
					{
						//close and reopen file
						if(!threadStruct->saveBinaryData_ && threadStruct->fp_)
						{
							fclose(threadStruct->fp_);
							threadStruct->fp_ = nullptr;
							__GEN_COUT__ << "Binary file closed." << __E__;
						}
					}

					threadStruct->resetStartEventTag_ = false;  //clear mailbox
				}

				//release buffers for restart
				if(threadStruct->thisDTC_)
				{
					threadStruct->releaseAllComplete_ =
					    false;  // arm flag before the release
					threadStruct->thisDTC_->ReleaseAllBuffers(DTC_DMA_Engine_DAQ);
					threadStruct->releaseAllComplete_ = true;
					__GEN_COUTT__ << "ReleaseAllBuffers called!" << __E__;

					//latch link enable status for use in accounting (ignoring timeout errors)
					threadStruct->rocLinkEnabledLatch_.clear();
					auto regVal = threadStruct->thisDTC_->ReadLinkEnabledData();
					for(auto r : DTCLib::DTC_ROC_Links)
					{
						auto re = threadStruct->thisDTC_->ReadLinkEnabled(r, regVal);
						if(re.TransmitEnable && re.ReceiveEnable)
							threadStruct->rocLinkEnabledLatch_[r] = true;
						else
							threadStruct->rocLinkEnabledLatch_[r] = false;
					}
					__GEN_COUTV__(
					    StringMacros::mapToString(threadStruct->rocLinkEnabledLatch_));
				}
			}

		}  //done with check for starting event window tag

		if(!threadStruct->inSubeventMode_)  //treat as an Event
		{
			__GEN_COUTT__ << "get the data requested as events via ->GetData(...)"
			              << __E__;

			while((events = threadStruct->thisDTC_->GetData(
			           DTCLib::DTC_EventWindowTag(threadStruct->nextEventWindowTag_),
			           false /* EWT match */))
			          .size())
			{
				if(threadStruct->exitThread_)
				{
					__GEN_COUT__ << "exitThread received in Buffer Test" << __E__;
					break;
				}

				__GEN_COUTT__ << "Read iteration #" << ii++
				              << ": Events returned by the DTC: " << events.size()
				              << std::endl;
				if(events.empty())
					break;  //impossible!

				for(auto& eventPtr : events)
				{
					if(threadStruct->exitThread_)
					{
						__GEN_COUT__ << "exitThread received in Buffer Test" << __E__;
						break;
					}

					if(eventPtr == nullptr)
					{
						__GEN_COUT_ERR__ << "Error: Null pointer!" << std::endl;
						continue;
					}

					// get the event
					auto event = eventPtr.get();

					++(threadStruct->eventsCount_);

					// check the event tag window
					if(threadStruct->nextEventWindowTag_ !=
					   event->GetEventWindowTag().GetEventWindowTag(true))
					{
						++(threadStruct->mismatchedEventTagsCount_);
						std::stringstream ostr;
						ostr << "Mismatched event tag. Expected = "
						     << threadStruct->nextEventWindowTag_ << " (0x" << std::hex
						     << std::setw(4) << std::setfill('0')
						     << threadStruct->nextEventWindowTag_
						     << "), Received = " << std::dec
						     << event->GetEventWindowTag().GetEventWindowTag(true)
						     << " (0x" << std::hex << std::setw(4) << std::setfill('0')
						     << event->GetEventWindowTag().GetEventWindowTag(true) << ")";
						__GEN_COUTT__ << ostr.str();
						threadStruct->mismatchedEventTagJumps_.push_back(
						    std::make_pair<uint64_t, uint64_t>(
						        threadStruct->nextEventWindowTag_,
						        event->GetEventWindowTag().GetEventWindowTag(true)));
					}
					threadStruct->nextEventWindowTag_ =
					    event->GetEventWindowTag().GetEventWindowTag(true) +
					    1;  //increment for next

					// ostr << "Request event tag:\t" << "0x" << std::hex << std::setw(4) << std::setfill('0') << eventTag.GetEventWindowTag(true) + ii <<
					//	" (" << std::dec << eventTag.GetEventWindowTag(true) + ii << ")" << std::endl;
					// ostr << "Response event tag:\t" << "0x" << std::hex << std::setw(4) << std::setfill('0') << event->GetEventWindowTag().GetEventWindowTag(true) <<
					//	" (" << std::dec << event->GetEventWindowTag().GetEventWindowTag(true) << ")" << std::endl;

					// get the event and the relative sub events
					// DTCLib::DTC_EventHeader *eventHeader = event->GetHeader();
					std::vector<DTCLib::DTC_SubEvent> subevents = event->GetSubEvents();

					__GEN_COUTTV__(subevents.size());
					for(auto& subevent : subevents)
						handleDetachedSubevent(subevent, threadStruct);
				}

			}  //end primary event retrieval loop
			//if here, no more data in DMA buffer
			if(lastCount != threadStruct->eventsCount_ || ii % 100 == 0)
			{
				__GEN_COUT__
				    << "No more events found in DMA buffer... waiting... iteration #"
				    << ii << ", Events received so far = " << threadStruct->eventsCount_
				    << __E__;
				lastCount = threadStruct->eventsCount_;
			}
		}
		else if(0)  //Treat as Subevent
		{
			__GEN_COUTT__
			    << "get the data requested as subevents via ->GetSubEventData2(...)"
			    << " nextEventWindowTag=" << threadStruct->nextEventWindowTag_
			    << " iteration=" << ii
			    << " subeventsCount=" << threadStruct->subeventsCount_;

			while((subevents = threadStruct->thisDTC_->GetSubEventData(
			           DTCLib::DTC_EventWindowTag(threadStruct->nextEventWindowTag_),
			           false /* EWT match */))
			          .size())
			{
				if(threadStruct->exitThread_)
				{
					__GEN_COUT__ << "exitThread received in Buffer Test" << __E__;
					break;
				}

				__GEN_COUTT__ << "Read iteration #" << ii++
				              << ": SubEvents returned by the DTC: " << subevents.size()
				              << std::endl;

				if(subevents.empty())
					continue;  //impossible!

				for(auto& subeventPtr : subevents)
				{
					if(threadStruct->exitThread_)
					{
						__GEN_COUT__ << "exitThread received in Buffer Test" << __E__;
						break;
					}

					if(subeventPtr == nullptr)
					{
						__GEN_COUT_ERR__ << "Error: Subevent Null pointer!" << std::endl;
						continue;
					}

					// Print first and last 8 qwords of the returned subevent to confirm data is copied correctly
					if(TTEST(1))
					{
						const uint8_t* raw = reinterpret_cast<const uint8_t*>(
						    subeventPtr->GetRawBufferPointer());
						const size_t      rawBytes = subeventPtr->GetSubEventByteCount();
						std::stringstream fss, lss;
						fss << "GetSubEventData2 returned subevent rawBytes=" << rawBytes
						    << " first 8 qwords: ";
						for(int _i = 0; _i < 8 && (size_t)(_i * 8) < rawBytes; ++_i)
							fss << std::hex << std::setw(16) << std::setfill('0')
							    << *reinterpret_cast<const uint64_t*>(raw + _i * 8)
							    << " ";
						__GEN_COUTT__ << fss.str() << __E__;

						lss << "GetSubEventData2 returned subevent last 8 qwords: ";
						const size_t lastStart = (rawBytes >= 64) ? rawBytes - 64 : 0;
						for(size_t _i = lastStart; _i + 8 <= rawBytes; _i += 8)
							lss << std::hex << std::setw(16) << std::setfill('0')
							    << *reinterpret_cast<const uint64_t*>(raw + _i) << " ";
						__GEN_COUTT__ << lss.str() << __E__;
					}

					handleDetachedSubevent(*(subeventPtr.get()), threadStruct);
				}
				//threadStruct->thisDTC_->ReleaseBuffers(DTC_DMA_Engine_DAQ,subevents.size()); // This currently does not exist, but it would be most efficient to release here
			}  //end primary Sub Event loop
			//if here, no more data in DMA buffer
			if(lastCount != threadStruct->subeventsCount_ || ii % 2000 == 0)
			{
				__GEN_COUT__
				    << "No more subevents found in DMA buffer... waiting... iteration #"
				    << ii
				    << ", SubEvents received so far = " << threadStruct->subeventsCount_
				    << __E__;
				lastCount = threadStruct->subeventsCount_;
			}
		}     // end Sub Event handling
		else  //extract Subevent as Events
		{
			__GEN_COUTT__ << "get the data requested as subevents via "
			                 "->GetSubEventDataAsEvents(...)"
			              << " nextEventWindowTag=" << threadStruct->nextEventWindowTag_
			              << " iteration=" << ii
			              << " subeventsCount=" << threadStruct->subeventsCount_;

			if(threadStruct->exitThread_)
			{
				__GEN_COUT_INFO__ << "exitThread received in Buffer Test" << __E__;
				break;
			}
			auto events = threadStruct->thisDTC_->GetSubEventDataAsEvents(
			    DTCLib::DTC_EventWindowTag(threadStruct->nextEventWindowTag_),
			    false /* EWT match */);

			++ii;

			if(events.empty())
				continue;

			__GEN_COUTT__ << "Read iteration #" << ii
			              << ": Events returned by the DTC: " << events.size()
			              << std::endl;

			for(auto& eventPtr : events)
			{
				if(threadStruct->exitThread_)
				{
					__GEN_COUT__ << "exitThread received in Buffer Test" << __E__;
					break;
				}

				__GEN_COUTT__ << "Read iteration #" << ii
				              << ": EWT=" << eventPtr->GetEventWindowTag()
				              << ", w/Subevent count = "
				              << eventPtr->GetSubEvents().size() << std::endl;

				if(eventPtr->GetSubEvents().empty())
				{
					__SS__ << "Error: No subevents found in extracted event! EWT="
					       << eventPtr->GetEventWindowTag();
					__SS_THROW__;
				}

				handleDetachedSubevent(eventPtr->GetSubEvents().at(0), threadStruct);
			}  //end event extraction and subevent parsing loop

			//if here, no more data in DMA buffer
			if(lastCount != threadStruct->subeventsCount_ || ii % 2000 == 0)
			{
				__GEN_COUT__
				    << "No more subevents found in DMA buffer... waiting... iteration #"
				    << ii
				    << ", SubEvents received so far = " << threadStruct->subeventsCount_
				    << __E__;
				lastCount = threadStruct->subeventsCount_;
			}
		}  // end Sub Event as Event handling

		if(threadStruct->fp_)
			fflush(threadStruct->fp_);
		// usleep(100); //100 us sleep
		std::this_thread::yield();  //try to be nice to other threads
		++ii;
	}  //end primary loop -------

	if(threadStruct->fp_)
	{
		fclose(threadStruct->fp_);
		threadStruct->fp_ = nullptr;
	}

	__GEN_COUT_INFO__ << "Buffer test thread exited. "
	                  << " Events received = " << threadStruct->eventsCount_
	                  << ", SubEvents received = " << threadStruct->subeventsCount_
	                  << __E__;
	threadStruct->running_ = false;

}  //end detachedBufferTestThread()
catch(...)
{
	__COUT_ERR__ << LOCAL_COUT_HDR << "Exception caught in detachedBufferTestThread()."
	             << __E__;
	threadStruct->running_ = false;

	if(threadStruct->thisDTC_)
		threadStruct->thisDTC_->GetDevice()->spy(
		    DTC_DMA_Engine_DAQ,
		    3 /* for once */ | 8 /* for wide view */ | 16 /* for stack trace */);

	//close any open file
	if(threadStruct->fp_)
	{
		__COUT__ << LOCAL_COUT_HDR << "Closing open Buffer Test file on error." << __E__;
		fclose(threadStruct->fp_);
		threadStruct->fp_ = nullptr;
	}

	std::stringstream errSs;
	errSs << "Exception caught. Exiting detachedBufferTestThread()." << __E__;
	try
	{
		throw;
	}
	catch(const std::runtime_error& e)
	{
		errSs << "Error message: " << e.what() << __E__;
	}
	catch(const std::exception& e)
	{
		errSs << "Error message: " << e.what() << __E__;
	}
	catch(...)
	{
		errSs << "Unknown error." << __E__;
	}
	threadStruct->error_ += errSs.str();
	__COUT_ERR__ << LOCAL_COUT_HDR << errSs.str();
}  //end detachedBufferTestThread() exception handling

//========================================================================
void DTCFrontEndInterface::BufferTest_detached(__ARGS__)
{
	__FE_COUT__ << "Operation \"BufferTest_detached\"" << std::endl;

	// arguments
	std::string command = __GET_ARG_IN__(
	    "Command to 0/Status (to read counters, etc.), 1/Start, or 2/Halt (Default: "
	    "Status)",
	    std::string,
	    "Status");

	bool dataAreSubEvents =
	    __GET_ARG_IN__("Data are SubEvents (Default: true)", bool, true);
	// unsigned int numberOfEvents = __GET_ARG_IN__("Number of [Sub]Events (Default: 1)", uint32_t, 1);
	bool         activeMatch = __GET_ARG_IN__("Match Event Tags (Default: false)", bool);
	unsigned int timestampStart =
	    __GET_ARG_IN__("Starting Event Window Tag (Default: 0)", unsigned int);
	bool saveBinaryDataToFile =
	    __GET_ARG_IN__("Save Binary Data to File (Default: false)", bool);
	std::string saveBinaryDataFilename =
	    __GET_ARG_IN__("Save Binary Data Filename", std::string);
	bool saveSubeventHeadersToDataFile =
	    __GET_ARG_IN__("Save Subevent Header to Binary File (Default: false)", bool);
	// bool displayPayloadAtGUI = __GET_ARG_IN__("Display Payload at GUI (Default: true)", bool, true);
	unsigned int packetThresholdToSave = __GET_ARG_IN__(
	    "Payload Packet Threshold for Saving Event (Default: 0)", unsigned int);

	__FE_COUTV__(command);
	__FE_COUTV__(dataAreSubEvents);
	__FE_COUTV__(activeMatch);
	__FE_COUTV__(timestampStart);
	__FE_COUTV__(saveBinaryDataToFile);
	__FE_COUTV__(saveBinaryDataFilename);
	__FE_COUTV__(saveSubeventHeadersToDataFile);
	__FE_COUTV__(packetThresholdToSave);

	// print the result
	std::stringstream outSs;
	outSs << "Command: " << command << __E__;
	if(command == "1" || command == "Start")
	{
		__FE_COUT__ << "Detaching thread and reading data DMA-0 starting at event tag "
		            << timestampStart << " (0x" << std::hex << timestampStart << ")"
		            << __E__;

		if(!bufferTestThreadStruct_)  //initialize shared pointer for first time
			bufferTestThreadStruct_ =
			    std::make_shared<DTCFrontEndInterface::DetachedBufferTestThreadStruct>();

		if(bufferTestThreadStruct_->running_)
			outSs
			    << "Found buffer test thread already running, doing nothing. Please "
			       "'Halt' before restarting. Or run 'Status' to read the latest status."
			    << __E__;
		else
		{
			__FE_COUT__ << "Launching detached Buffer Test thread..." << __E__;

			// start mutex scope
			{
				std::lock_guard<std::mutex> lock(bufferTestThreadStruct_->lock_);
				bufferTestThreadStruct_->inSubeventMode_         = dataAreSubEvents;
				bufferTestThreadStruct_->activeMatch_            = activeMatch;
				bufferTestThreadStruct_->expectedEventTag_       = timestampStart;
				bufferTestThreadStruct_->saveBinaryData_         = saveBinaryDataToFile;
				bufferTestThreadStruct_->saveBinaryDataFilename_ = saveBinaryDataFilename;
				bufferTestThreadStruct_->saveSubeventHeadersToBinaryData_ =
				    saveSubeventHeadersToDataFile;
				bufferTestThreadStruct_->exitThread_         = false;
				bufferTestThreadStruct_->resetStartEventTag_ = false;
				bufferTestThreadStruct_->thisDTC_            = thisDTC_;
				bufferTestThreadStruct_->running_            = true;
				bufferTestThreadStruct_->error_              = "";
			}
			std::thread(
			    [](std::shared_ptr<DTCFrontEndInterface::DetachedBufferTestThreadStruct>
			           threadStruct) {
				    DTCFrontEndInterface::detachedBufferTestThread(threadStruct);
			    },
			    bufferTestThreadStruct_)
			    .detach();
			outSs << "Launched detached Buffer Test thread and reading data DMA-0 "
			         "starting at event tag "
			      << timestampStart << " (0x" << std::hex << timestampStart << ")"
			      << __E__;
		}
		sleep(1);
		outSs << "Reading status..." << __E__;
		outSs << DTCFrontEndInterface::getDetachedBufferTestStatus(
		    bufferTestThreadStruct_);
	}
	else if(command == "0" || command == "Status")
	{
		__FE_COUT__ << "Reading thread status..." << __E__;
		outSs << "Reading thread status..." << __E__;

		if(!bufferTestThreadStruct_)  //initialize shared pointer for first time
			bufferTestThreadStruct_ =
			    std::make_shared<DTCFrontEndInterface::DetachedBufferTestThreadStruct>();

		outSs << DTCFrontEndInterface::getDetachedBufferTestStatus(
		    bufferTestThreadStruct_);
	}
	else if(command == "2" || command == "Halt")
	{
		__FE_COUT__ << "Halting thread... " << __E__;

		if(!bufferTestThreadStruct_)  //initialize shared pointer for first time
			bufferTestThreadStruct_ =
			    std::make_shared<DTCFrontEndInterface::DetachedBufferTestThreadStruct>();

		// start mutex scope
		{
			std::lock_guard<std::mutex> lock(bufferTestThreadStruct_->lock_);
			bufferTestThreadStruct_->exitThread_ = true;
		}

		//check for thread to exit
		for(int i = 0; i < 10; ++i)
		{
			usleep(100 * 1000 /*100ms*/);  // sleep for exit time
			if(!bufferTestThreadStruct_->running_)
				break;
			__FE_COUT__ << "Waiting for thread to exit... #" << i << __E__;
		}

		if(bufferTestThreadStruct_->fp_)
		{
			__FE_COUT_WARN__ << "Buffer Test thread file was left open?! Closing..."
			                 << __E__;

			fclose(bufferTestThreadStruct_->fp_);
			bufferTestThreadStruct_->fp_ = nullptr;
		}

		if(!bufferTestThreadStruct_->running_)
			outSs << "Detached Buffer Test thread exited. " << __E__;
		else
			outSs << "Detached Buffer Test thread is stuck running. " << __E__;
		outSs << "Reading final status..." << __E__;
		try
		{
			outSs << DTCFrontEndInterface::getDetachedBufferTestStatus(
			    bufferTestThreadStruct_);
		}
		catch(const std::runtime_error& e)
		{
			__FE_COUT_WARN__ << "Ignoring buffer status error during HALT: " << e.what()
			                 << __E__;
		}
	}
	else
	{
		outSs << "Unrecognized command '" << command
		      << "' found. Valid commands are Start, Status, and Halt." << __E__;
	}
	// outSs << "Active Event Match: " << (activeMatch?"true":"false") << __E__;
	// // outSs << "Event Duration: " << cfoDelay << " = " << cfoDelay*25 << " ns" << __E__;
	// // outSs << "Reading back: " << (doNotReadBack?"false":"true") << __E__;
	// if(fp) outSs << "Binary data file saved at: " << filename << __E__;
	// outSs << ostr.str();

	if(TTEST(1))
		std::cout << "Untruncated output: \n"
		          << outSs.str() << __E__;  //for no truncation!

	__SET_ARG_OUT__("Result", outSs.str());
}  //end BufferTest_detached()

//========================================================================
void DTCFrontEndInterface::BufferTest(__ARGS__)
{
	__FE_COUT__ << "Operation \"buffer_test\"" << std::endl;

	// reset the dtc
	// DTCSoftReset();
	// configureHardwareDevMode();

	// // release of all the buffers
	// getDevice()->read_release(DTC_DMA_Engine_DAQ, 100);

	// stream to print the output
	std::stringstream ostr;
	ostr << std::endl;

	// arguments
	bool dataAreSubEvents =
	    __GET_ARG_IN__("Data are SubEvents (Default: true)", bool, true);
	unsigned int numberOfEvents =
	    __GET_ARG_IN__("Number of [Sub]Events (Default: 1)", uint32_t, 1);
	bool         activeMatch = __GET_ARG_IN__("Match Event Tags (Default: false)", bool);
	unsigned int timestampStart =
	    __GET_ARG_IN__("Starting Event Window Tag (Default: 0)", unsigned int);
	bool saveBinaryDataToFile =
	    __GET_ARG_IN__("Save Binary Data to File (Default: false)", bool);
	bool displayPayloadAtGUI =
	    __GET_ARG_IN__("Display Payload at GUI (Default: true)", bool, true);

	__FE_COUTV__(dataAreSubEvents);
	__FE_COUTV__(numberOfEvents);
	__FE_COUTV__(activeMatch);
	__FE_COUTV__(timestampStart);
	__FE_COUTV__(saveBinaryDataToFile);
	__FE_COUTV__(displayPayloadAtGUI);

	ostr << "Step 1 " << std::endl;

	// parameters
	// uint16_t debugPacketCount = 0;
	// uint32_t cfoDelay = __GET_ARG_IN__("eventDuration (Default := 400)", uint32_t, 400);	//400 -- delay in the frequency of the emulated CFO
	// bool doNotReadBack = __GET_ARG_IN__("doNotReadBack (bool)", bool);
	// uint32_t requestDelay = 0;
	// bool incrementTimestamp = true;		// this parameter is not working with emulated CFO
	// bool useCFOinDTCEmulator = !__GET_ARG_IN__("Software Generated Data Requests (bool)", bool);
	// bool stickyDebugType = true;
	// bool quiet = false;
	// bool asyncRR = false;
	// bool forceNoDebugMode = true;
	// bool doNotSendHeartbeats = __GET_ARG_IN__("Do Not Send Heartbeats (bool)", bool);
	// int requestsAhead = 0;
	// auto debugType = DTCLib::DTC_DebugType_SpecialSequence;	// enum (0)

	// // event window Tag used to bind the request to the response
	DTCLib::DTC_EventWindowTag eventTag =
	    DTCLib::DTC_EventWindowTag(static_cast<uint64_t>(timestampStart));

	// // create the emulated CFO instance
	// DTCLib::DTCSoftwareCFO* cfo = new DTCLib::DTCSoftwareCFO(getDTC(),
	//															useCFOinDTCEmulator,
	//															debugPacketCount,
	//															debugType,
	//															stickyDebugType,
	//															quiet,
	//															asyncRR,
	//															forceNoDebugMode);
	// // send the request for a range of events
	// cfo->SendRequestsForRange(numberOfEvents,
	//							eventTag,
	//							incrementTimestamp,
	//							cfoDelay,
	//							requestsAhead,
	//							16 /* heartbeatsAfter */,
	//							!doNotSendHeartbeats /* sendHeartbeats */);

	std::string filename = "/macroOutput_" + std::to_string(time(0)) + "_" +
	                       std::to_string(clock()) + ".bin";
	FILE* fp = nullptr;
	if(saveBinaryDataToFile)
	{
		filename = std::string(__ENV__("OTSDAQ_DATA")) + "/" + getInterfaceUID() + "_" +
		           filename;
		__FE_COUTV__(filename);
		fp = fopen(filename.c_str(), "wb");
		if(!fp)
		{
			__FE_SS__ << "Failed to open file to save macro output '" << filename
			          << "'..." << __E__;
			__FE_SS_THROW__;
		}
	}

	if(!dataAreSubEvents)  //treat as an Event
	{
		// get the data requested
		for(unsigned int ii = 0;  //!doNotReadBack &&
		    ii < numberOfEvents;
		    ++ii)
		{
			// get the data
			std::vector<std::unique_ptr<DTCLib::DTC_Event>> events =
			    getDTC()->GetData(eventTag + ii, activeMatch);
			ostr << "Read " << std::dec << ii
			     << ": Events returned by the DTC: " << events.size() << std::endl;
			if(!events.empty())
			{
				for(auto& eventPtr : events)
				{
					if(eventPtr == nullptr)
					{
						ostr << "Error: Null pointer!" << std::endl;
						continue;
					}

					// get the event
					auto event = eventPtr.get();

					// check the event tag window
					ostr << "Request event tag:\t"
					     << "0x" << std::hex << std::setw(4) << std::setfill('0')
					     << eventTag.GetEventWindowTag(true) + ii << " (" << std::dec
					     << eventTag.GetEventWindowTag(true) + ii << ")" << std::endl;
					ostr << "Response event tag:\t"
					     << "0x" << std::hex << std::setw(4) << std::setfill('0')
					     << event->GetEventWindowTag().GetEventWindowTag(true) << " ("
					     << std::dec << event->GetEventWindowTag().GetEventWindowTag(true)
					     << ")" << std::endl;

					// get the event and the relative sub events
					DTCLib::DTC_EventHeader*          eventHeader = event->GetHeader();
					std::vector<DTCLib::DTC_SubEvent> subevents   = event->GetSubEvents();

					// print the event header
					ostr << eventHeader->toJson() << std::endl
					     << "Subevents count: " << event->GetSubEventCount() << std::endl;

					// iterate over the subevents
					for(unsigned int i = 0; i < subevents.size(); ++i)
					{
						// print the subevents header
						DTCLib::DTC_SubEvent subevent = subevents[i];
						ostr << "Subevent [" << i << "]:" << std::endl;
						ostr << subevent.GetHeader()->toJson() << std::endl;

						// check if there is an error on the link
						if(subevent.GetHeader()->link0_status > 0)
						{
							ostr << "Error: " << std::endl;
							std::bitset<8> link0_status(
							    subevent.GetHeader()->link0_status);
							if(link0_status.test(0))
							{
								ostr << "ROC Timeout Error!" << std::endl;
							}
							if(link0_status.test(2))
							{
								ostr << "Packet sequence number Error!" << std::endl;
							}
							if(link0_status.test(3))
							{
								ostr << "CRC Error!" << std::endl;
							}
							if(link0_status.test(6))
							{
								ostr << "Fatal Error!" << std::endl;
							}

							continue;
						}

						// print the number of data blocks
						ostr << "Number of Data Block: " << subevent.GetDataBlockCount()
						     << std::endl;

						// iterate over the data blocks
						std::vector<DTCLib::DTC_DataBlock> dataBlocks =
						    subevent.GetDataBlocks();
						for(unsigned int j = 0; j < dataBlocks.size(); ++j)
						{
							ostr << "Data block [" << j << "]:" << std::endl;
							// print the data block header
							DTCLib::DTC_DataHeaderPacket* dataHeader =
							    dataBlocks[j].GetHeader().get();
							ostr << dataHeader->toJSON() << std::endl;

							// print the data block ROC fragment header raw data
							{
								auto dataPtr = reinterpret_cast<const uint8_t*>(
								    dataBlocks[j].GetRawBufferPointer());
								ostr << "Data header raw:" << std::endl;
								for(int l = 0; l < 16; l += 2)
								{
									if(fp)
										fwrite(&dataPtr[l - 16], sizeof(uint32_t), 1, fp);
									ostr << "\t0x" << std::hex << std::setw(8)
									     << std::setfill('0')
									     << *((uint32_t*)(&(dataPtr[l - 16])))
									     << std::endl;
								}
							}

							// print the data block payload raw data
							{
								auto dataPtr = reinterpret_cast<const uint8_t*>(
								    dataBlocks[j].GetData());
								if(displayPayloadAtGUI)
									ostr << "Data payload:" << std::endl;
								for(int l = 0; l < dataHeader->GetByteCount() - 16;
								    l += 2)
								{
									if(fp)
										fwrite(&dataPtr[l], sizeof(uint32_t), 1, fp);
									if(displayPayloadAtGUI)
										ostr << "\t0x" << std::hex << std::setw(8)
										     << std::setfill('0')
										     << *((uint32_t*)(&(dataPtr[l])))
										     << std::endl;
								}
							}
						}
					}
					ostr << std::endl << std::endl;
				}
			}
		}
	}
	else  //Treat as Subevent
	{
		uint32_t numberOfSubEvents         = numberOfEvents;
		uint32_t numberOfSubEventsReceived = 0;

		// get the data requested
		for(unsigned int ii = 0;  //!doNotReadBack &&
		    ii < numberOfSubEvents;
		    ++ii)
		{
			if(numberOfSubEventsReceived)
			{
				ostr << "Received requested number of SubEvents: " << std::dec
				     << numberOfSubEventsReceived << __E__;
				break;
			}

			// get the data
			std::vector<std::unique_ptr<DTCLib::DTC_SubEvent>> subevents =
			    getDTC()->GetSubEventData(eventTag + ii, activeMatch);
			numberOfSubEventsReceived += subevents.size();
			ostr << "Read " << std::dec << ii
			     << ": SubEvents returned by the DTC: " << subevents.size()
			     << ". Total received so far: " << numberOfSubEventsReceived << std::endl;
			if(subevents.empty())
				continue;

			for(auto& subeventPtr : subevents)
			{
				if(subeventPtr == nullptr)
				{
					ostr << "Error: Null pointer!" << std::endl;
					continue;
				}

				// get the subevent
				auto subevent = subeventPtr.get();

				// check the subevent tag window
				ostr << "Request subevent tag:\t"
				     << "0x" << std::hex << std::setw(4) << std::setfill('0')
				     << eventTag.GetEventWindowTag(true) + ii << " (" << std::dec
				     << eventTag.GetEventWindowTag(true) + ii << ")" << std::endl;
				ostr << "Response subevent tag:\t"
				     << "0x" << std::hex << std::setw(4) << std::setfill('0')
				     << subevent->GetEventWindowTag().GetEventWindowTag(true) << " ("
				     << std::dec << subevent->GetEventWindowTag().GetEventWindowTag(true)
				     << ")" << std::endl;

				// print the event header
				ostr << subevent->GetHeader()->toJson() << std::endl;

				// check if there is an error on the link
				if(subevent->GetHeader()->link0_status > 0)
				{
					ostr << "ROC-0 Error: " << std::endl;
					std::bitset<8> link0_status(subevent->GetHeader()->link0_status);
					if(link0_status.test(0))
					{
						ostr << "ROC Timeout Error!" << std::endl;
					}
					if(link0_status.test(2))
					{
						ostr << "Packet sequence number Error!" << std::endl;
					}
					if(link0_status.test(3))
					{
						ostr << "CRC Error!" << std::endl;
					}
					if(link0_status.test(6))
					{
						ostr << "Fatal Error!" << std::endl;
					}

					// continue;
				}

				if(subevent->GetHeader()->link1_status > 0)
				{
					ostr << "ROC-1 Error: " << std::endl;
					std::bitset<8> link1_status(subevent->GetHeader()->link1_status);
					if(link1_status.test(0))
					{
						ostr << "ROC Timeout Error!" << std::endl;
					}
					if(link1_status.test(2))
					{
						ostr << "Packet sequence number Error!" << std::endl;
					}
					if(link1_status.test(3))
					{
						ostr << "CRC Error!" << std::endl;
					}
					if(link1_status.test(6))
					{
						ostr << "Fatal Error!" << std::endl;
					}

					// continue;
				}

				if(subevent->GetHeader()->link2_status > 0)
				{
					ostr << "ROC-2 Error: " << std::endl;
					std::bitset<8> link2_status(subevent->GetHeader()->link2_status);
					if(link2_status.test(0))
					{
						ostr << "ROC Timeout Error!" << std::endl;
					}
					if(link2_status.test(2))
					{
						ostr << "Packet sequence number Error!" << std::endl;
					}
					if(link2_status.test(3))
					{
						ostr << "CRC Error!" << std::endl;
					}
					if(link2_status.test(6))
					{
						ostr << "Fatal Error!" << std::endl;
					}

					// continue;
				}

				// print the number of data blocks
				ostr << "Number of Data Block (ROC Fragments): "
				     << subevent->GetDataBlockCount() << std::endl;

				// iterate over the data blocks
				std::vector<DTCLib::DTC_DataBlock> dataBlocks = subevent->GetDataBlocks();
				for(unsigned int j = 0; j < dataBlocks.size(); ++j)
				{
					ostr << "Data block [" << j << "]:" << std::endl;
					// print the data block header
					DTCLib::DTC_DataHeaderPacket* dataHeader =
					    dataBlocks[j].GetHeader().get();
					ostr << dataHeader->toJSON() << std::endl;

					// print the data block ROC fragment header raw data
					{
						auto dataPtr = reinterpret_cast<const uint8_t*>(
						    dataBlocks[j].GetRawBufferPointer());
						ostr << "Data header raw:" << std::endl;
						for(int l = 0; l < 16; l += 4)
						{
							if(fp)
								fwrite(&dataPtr[l], sizeof(uint32_t), 1, fp);
							ostr << "\t0x" << std::hex << std::setw(8)
							     << std::setfill('0') << *((uint32_t*)(&(dataPtr[l])))
							     << std::endl;
						}
					}

					// print the data block payload raw data
					{
						auto dataPtr =
						    reinterpret_cast<const uint8_t*>(dataBlocks[j].GetData());
						if(displayPayloadAtGUI)
							ostr << "Data payload:" << std::endl;
						for(int l = 0; l < dataHeader->GetByteCount() - 16; l += 4)
						{
							if(fp)
								fwrite(&dataPtr[l], sizeof(uint32_t), 1, fp);
							if(displayPayloadAtGUI)
								ostr << "\t0x" << std::hex << std::setw(8)
								     << std::setfill('0') << *((uint32_t*)(&(dataPtr[l])))
								     << std::endl;
						}
					}

				}  //end Data Block ROC fragment loop

				ostr << std::endl << std::endl;
			}  //end Sub Event loop (should only be one Sub Event)
		}      //end primary Sub Event loop
	}          // end Sub Event handling

	if(fp)
		fclose(fp);  //close binary file

	// print the result
	std::stringstream outSs;
	outSs << "Number of events  requested: " + std::to_string(numberOfEvents)
	      << " from starting event tag " << timestampStart << __E__;
	outSs << "Active Event Match: " << (activeMatch ? "true" : "false") << __E__;
	// outSs << "Event Duration: " << cfoDelay << " = " << cfoDelay*25 << " ns" << __E__;
	// outSs << "Reading back: " << (doNotReadBack?"false":"true") << __E__;
	if(fp)
		outSs << "Binary data file saved at: " << filename << __E__;
	outSs << ostr.str();

	std::cout << "Untruncated output: \n" << outSs.str() << __E__;  //for no truncation!

	__SET_ARG_OUT__("Result", outSs.str());
	// delete cfo;
}  //end BufferTest()

//========================================================================
// TODO: print the packet with the correct event tag
void DTCFrontEndInterface::PatternTest(__ARGS__)
{
	__FE_COUT__ << "Operation \"buffer_test\"" << std::endl;

	// reset the dtc
	// DTCSoftReset();
	// configureHardwareDevMode();

	// // release of all the buffers
	// getDevice()->read_release(DTC_DMA_Engine_DAQ, 100);

	// stream to print the output
	std::stringstream ostr;
	ostr << std::endl;

	// arguments
	bool dataAreSubEvents =
	    __GET_ARG_IN__("Data are SubEvents (Default: true)", bool, true);
	unsigned int numberOfEvents =
	    __GET_ARG_IN__("Number of [Sub]Events (Default: 1)", uint32_t, 1);
	bool         activeMatch = __GET_ARG_IN__("Match Event Tags (Default: false)", bool);
	unsigned int timestampStart =
	    __GET_ARG_IN__("Starting Event Window Tag (Default: 0)", unsigned int);
	bool saveBinaryDataToFile =
	    __GET_ARG_IN__("Save Binary Data to File (Default: false)", bool);
	bool displayPayloadAtGUI =
	    __GET_ARG_IN__("Display Payload at GUI (Default: true)", bool, true);

	__FE_COUTV__(dataAreSubEvents);
	__FE_COUTV__(numberOfEvents);
	__FE_COUTV__(activeMatch);
	__FE_COUTV__(timestampStart);
	__FE_COUTV__(saveBinaryDataToFile);
	__FE_COUTV__(displayPayloadAtGUI);

	ostr << "Step 1 " << std::endl;

	// parameters
	// uint16_t debugPacketCount = 0;
	// uint32_t cfoDelay = __GET_ARG_IN__("eventDuration (Default := 400)", uint32_t, 400);	//400 -- delay in the frequency of the emulated CFO
	// bool doNotReadBack = __GET_ARG_IN__("doNotReadBack (bool)", bool);
	// uint32_t requestDelay = 0;
	// bool incrementTimestamp = true;		// this parameter is not working with emulated CFO
	// bool useCFOinDTCEmulator = !__GET_ARG_IN__("Software Generated Data Requests (bool)", bool);
	// bool stickyDebugType = true;
	// bool quiet = false;
	// bool asyncRR = false;
	// bool forceNoDebugMode = true;
	// bool doNotSendHeartbeats = __GET_ARG_IN__("Do Not Send Heartbeats (bool)", bool);
	// int requestsAhead = 0;
	// auto debugType = DTCLib::DTC_DebugType_SpecialSequence;	// enum (0)

	// // event window Tag used to bind the request to the response
	DTCLib::DTC_EventWindowTag eventTag =
	    DTCLib::DTC_EventWindowTag(static_cast<uint64_t>(timestampStart));

	// // create the emulated CFO instance
	// DTCLib::DTCSoftwareCFO* cfo = new DTCLib::DTCSoftwareCFO(getDTC(),
	//															useCFOinDTCEmulator,
	//															debugPacketCount,
	//															debugType,
	//															stickyDebugType,
	//															quiet,
	//															asyncRR,
	//															forceNoDebugMode);
	// // send the request for a range of events
	// cfo->SendRequestsForRange(numberOfEvents,
	//							eventTag,
	//							incrementTimestamp,
	//							cfoDelay,
	//							requestsAhead,
	//							16 /* heartbeatsAfter */,
	//							!doNotSendHeartbeats /* sendHeartbeats */);

	std::string filename = "/macroOutput_" + std::to_string(time(0)) + "_" +
	                       std::to_string(clock()) + "_pattern.bin";
	FILE* fp = nullptr;
	if(saveBinaryDataToFile)
	{
		filename = std::string(__ENV__("OTSDAQ_DATA")) + "/" + filename;
		__FE_COUTV__(filename);
		fp = fopen(filename.c_str(), "wb");
		if(!fp)
		{
			__FE_SS__ << "Failed to open file to save macro output '" << filename
			          << "'..." << __E__;
			__FE_SS_THROW__;
		}
	}

	int hit_in[64] = {1,  2,  3,  0,  0,  0,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
	                  0,  20, 21, 22, 12, 13, 11, 12, 0,  0,  8,  4,  12, 11, 12, 13,
	                  16, 6,  3,  1,  12, 0,  16, 17, 18, 19, 12, 1,  12, 12, 11, 11,
	                  0,  0,  0,  0,  13, 14, 10, 13, 11, 14, 14, 15, 8,  9,  10, 32};

	if(!dataAreSubEvents)  //treat as an Event
	{
		// get the data requested
		for(unsigned int ii = 0;  //!doNotReadBack &&
		    ii < numberOfEvents;
		    ++ii)
		{
			// get the data
			std::vector<std::unique_ptr<DTCLib::DTC_Event>> events =
			    getDTC()->GetData(eventTag + ii, activeMatch);
			ostr << "Read " << ii << ": Events returned by the DTC: " << events.size()
			     << std::endl;
			if(!events.empty())
			{
				for(auto& eventPtr : events)
				{
					if(eventPtr == nullptr)
					{
						ostr << "Error: Null pointer!" << std::endl;
						continue;
					}

					// get the event
					auto event = eventPtr.get();

					// check the event tag window
					ostr << "Request event tag:\t"
					     << "0x" << std::hex << std::setw(4) << std::setfill('0')
					     << eventTag.GetEventWindowTag(true) + ii << " (" << std::dec
					     << eventTag.GetEventWindowTag(true) + ii << ")" << std::endl;
					ostr << "Response event tag:\t"
					     << "0x" << std::hex << std::setw(4) << std::setfill('0')
					     << event->GetEventWindowTag().GetEventWindowTag(true) << " ("
					     << std::dec << event->GetEventWindowTag().GetEventWindowTag(true)
					     << ")" << std::endl;

					// get the event and the relative sub events
					//DTCLib::DTC_EventHeader *eventHeader = event->GetHeader();
					std::vector<DTCLib::DTC_SubEvent> subevents = event->GetSubEvents();

					// print the event header
					//ostr << eventHeader->toJson() << std::endl
					//		<< "Subevents count: " << event->GetSubEventCount() << std::endl;

					// iterate over the subevents
					for(unsigned int i = 0; i < subevents.size(); ++i)
					{
						// print the subevents header
						DTCLib::DTC_SubEvent subevent = subevents[i];
						ostr << "Subevent [" << i << "]:" << std::endl;
						//ostr << subevent.GetHeader()->toJson() << std::endl;

						// check if there is an error on the link
						if(subevent.GetHeader()->link0_status > 0)
						{
							ostr << "Error: " << std::endl;
							std::bitset<8> link0_status(
							    subevent.GetHeader()->link0_status);
							if(link0_status.test(0))
							{
								ostr << "ROC Timeout Error!" << std::endl;
							}
							if(link0_status.test(2))
							{
								ostr << "Packet sequence number Error!" << std::endl;
							}
							if(link0_status.test(3))
							{
								ostr << "CRC Error!" << std::endl;
							}
							if(link0_status.test(6))
							{
								ostr << "Fatal Error!" << std::endl;
							}

							continue;
						}

						// print the number of data blocks
						ostr << "Number of Data Block: " << subevent.GetDataBlockCount()
						     << std::endl;

						// iterate over the data blocks
						std::vector<DTCLib::DTC_DataBlock> dataBlocks =
						    subevent.GetDataBlocks();
						for(unsigned int j = 0; j < dataBlocks.size(); ++j)
						{
							ostr << "Data block [" << j << "]:" << std::endl;
							// print the data block header
							DTCLib::DTC_DataHeaderPacket* dataHeader =
							    dataBlocks[j].GetHeader().get();
							//ostr << dataHeader->toJSON() << std::endl;

							// print the data block ROC fragment header raw data
							{
								auto dataPtr = reinterpret_cast<const uint8_t*>(
								    dataBlocks[j].GetRawBufferPointer());
								//ostr << "Data header raw:" << std::endl;
								for(int l = 0; l < 16; l += 2)
								{
									if(fp)
										fwrite(&dataPtr[l - 16], sizeof(uint32_t), 1, fp);
									//ostr << "\t0x" << std::hex << std::setw(8) << std::setfill('0') << *((uint32_t *)(&(dataPtr[l-16]))) << std::endl;
								}
							}

							// print the data block payload raw data
							{
								auto dataPtr = reinterpret_cast<const uint8_t*>(
								    dataBlocks[j].GetData());
								if(displayPayloadAtGUI)
									ostr << "Data payload:" << std::endl;
								for(int l = 0; l < dataHeader->GetByteCount() - 16;
								    l += 2)
								{
									if(fp)
										fwrite(&dataPtr[l], sizeof(uint32_t), 1, fp);
									if(displayPayloadAtGUI)
										ostr << "\t0x" << std::hex << std::setw(8)
										     << std::setfill('0')
										     << *((uint32_t*)(&(dataPtr[l])))
										     << std::endl;
								}
							}
						}
					}
					ostr << std::endl << std::endl;
				}
			}
		}
	}
	else  //Treat as Subevent
	{
		uint32_t numberOfSubEvents         = numberOfEvents;
		uint32_t numberOfSubEventsReceived = 0;

		uint32_t last_data = -1;

		// get the data requested
		for(unsigned int ii = 0;  //!doNotReadBack &&
		    ii < numberOfSubEvents;
		    ++ii)
		{
			// get the data
			std::vector<std::unique_ptr<DTCLib::DTC_SubEvent>> subevents =
			    getDTC()->GetSubEventData(eventTag + ii, activeMatch);
			numberOfSubEventsReceived += subevents.size();
			ostr << "Read " << ii
			     << ": SubEvents returned by the DTC: " << subevents.size()
			     << ". Total received so far: " << numberOfSubEventsReceived << std::endl;
			if(subevents.empty())
				continue;

			for(auto& subeventPtr : subevents)
			{
				if(subeventPtr == nullptr)
				{
					ostr << "Error: Null pointer!" << std::endl;
					continue;
				}

				// get the subevent
				auto subevent = subeventPtr.get();

				// check the subevent tag window
				//ostr << "Request subevent tag:\t" << "0x" << std::hex << std::setw(4) << std::setfill('0') << eventTag.GetEventWindowTag(true) + ii <<
				//" (" << std::dec << eventTag.GetEventWindowTag(true) + ii << ")" << std::endl;
				//ostr << "Response subevent tag:\t" << "0x" << std::hex << std::setw(4) << std::setfill('0') << subevent->GetEventWindowTag().GetEventWindowTag(true) <<
				//	" (" << std::dec << subevent->GetEventWindowTag().GetEventWindowTag(true) << ")" <<std::endl;

				// print the event header
				//ostr << subevent->GetHeader()->toJson() << std::endl;

				// check if there is an error on the link
				if(subevent->GetHeader()->link0_status > 0)
				{
					ostr << "ROC-0 Error: ";
					std::bitset<8> link0_status(subevent->GetHeader()->link0_status);
					if(link0_status.test(0))
					{
						ostr << "ROC Timeout Error!" << std::endl;
					}
					if(link0_status.test(2))
					{
						ostr << "Packet sequence number Error!" << std::endl;
					}
					if(link0_status.test(3))
					{
						ostr << "CRC Error!" << std::endl;
					}
					if(link0_status.test(6))
					{
						ostr << "Fatal Error!" << std::endl;
					}

					// continue;
				}

				if(subevent->GetHeader()->link1_status > 0 and 0)
				{
					ostr << "ROC-1 Error: ";
					std::bitset<8> link1_status(subevent->GetHeader()->link1_status);
					if(link1_status.test(0))
					{
						ostr << "ROC Timeout Error!" << std::endl;
					}
					if(link1_status.test(2))
					{
						ostr << "Packet sequence number Error!" << std::endl;
					}
					if(link1_status.test(3))
					{
						ostr << "CRC Error!" << std::endl;
					}
					if(link1_status.test(6))
					{
						ostr << "Fatal Error!" << std::endl;
					}

					// continue;
				}

				// print the number of data blocks
				//ostr << "Number of Data Block (ROC Fragments): " << subevent->GetDataBlockCount() << std::endl;

				// iterate over the data blocks
				std::vector<DTCLib::DTC_DataBlock> dataBlocks = subevent->GetDataBlocks();
				for(unsigned int j = 0; j < dataBlocks.size(); ++j)
				{
					//ostr << "Data block [" << j << "]:" << std::endl;
					// print the data block header
					DTCLib::DTC_DataHeaderPacket* dataHeader =
					    dataBlocks[j].GetHeader().get();
					//ostr << dataHeader->toJSON() << std::endl;

					// print the data block ROC fragment header raw data
					{
						auto dataPtr = reinterpret_cast<const uint8_t*>(
						    dataBlocks[j].GetRawBufferPointer());
						//ostr << "Data header raw:" << std::endl;
						for(int l = 0; l < 16; l += 4)
						{
							if(fp)
								fwrite(&dataPtr[l], sizeof(uint32_t), 1, fp);
							//ostr << "\t0x" << std::hex << std::setw(8) << std::setfill('0') << *((uint32_t *)(&(dataPtr[l]))) << std::endl;
						}
					}

					int payload_size = ((dataHeader->GetByteCount() - 16) / 4);
					//ostr << payload_size << std::endl;
					//ostr << hit_in[ii%64] * 8 << std::endl;
					if(j == 0 and payload_size != hit_in[ii % 64] * 8)
					{
						ostr << "##################################################"
						     << std::endl;
						ostr << "Wrong size of pattern at event " << ii << std::endl;
						ostr << "##################################################"
						     << std::endl;
					}

					// print the data block payload raw data
					{
						auto dataPtr =
						    reinterpret_cast<const uint8_t*>(dataBlocks[j].GetData());
						//if(displayPayloadAtGUI) ostr << "Data payload:" << std::endl;
						for(int l = 0; l < dataHeader->GetByteCount() - 16; l += 4)
						{
							uint32_t pdata = *((uint32_t*)(&(dataPtr[l])));
							//ostr << last_data << "   " << pdata << std::endl;

							if(j == 0 and pdata != last_data + 1)
							{
								ostr << "################################################"
								        "##"
								     << std::endl;
								ostr << "Wrong number in the pattern stream of event "
								     << ii << std::endl;
								ostr << "################################################"
								        "##"
								     << std::endl;

								last_data = 0;
								for(unsigned int i = 0; i <= ii % 64; i++)
								{
									last_data += (hit_in[i] * 8) - 1;
								}
							}
							else
							{
								last_data = pdata;
							}

							if(fp)
								fwrite(&dataPtr[l], sizeof(uint32_t), 1, fp);
							//if(displayPayloadAtGUI) ostr << "\t0x" << std::hex << std::setw(8) << std::setfill('0') << *((uint32_t *)(&(dataPtr[l]))) << std::endl;
						}
					}

				}  //end Data Block ROC fragment loop

				ostr << std::endl << std::endl;
			}  //end Sub Event loop (should only be one Sub Event)
		}      //end primary Sub Event loop
	}          // end Sub Event handling

	if(fp)
		fclose(fp);  //close binary file

	// print the result
	std::stringstream outSs;
	outSs << "Number of events  requested: " + std::to_string(numberOfEvents)
	      << " from starting event tag " << timestampStart << __E__;
	outSs << "Active Event Match: " << (activeMatch ? "true" : "false") << __E__;
	// outSs << "Event Duration: " << cfoDelay << " = " << cfoDelay*25 << " ns" << __E__;
	// outSs << "Reading back: " << (doNotReadBack?"false":"true") << __E__;
	if(fp)
		outSs << "Binary data file saved at: " << filename << __E__;
	outSs << ostr.str();

	std::cout << "Untruncated output: \n" << outSs.str() << __E__;  //for no truncation!

	__SET_ARG_OUT__("Result", outSs.str());
	// delete cfo;
}  //end PatternTest()

//========================================================================
void DTCFrontEndInterface::CFOEmulatorLoopbackTest(__ARGS__)
{
	__FE_COUT__ << "CFO Emulator Loopback Test run" << __E__;

	getDTC()->EnableCFOLoopback();
	getDTC()->RunCableDelayLoopbackTest();

	// std::stringstream outSs;
	// outSs << ;

	__SET_ARG_OUT__("Result", getDTC()->FormatCFOEmulationLoopbackDelayMeasure());

	//to get loopback value
	//uint32_t loopbackValue = getDTC()->ReadCFOEmulationLoopbackDelayMeasure();

}  //end CFOEmulatorLoopbackTest()

//========================================================================
void DTCFrontEndInterface::CFOEmulatorLoopbackTests(__ARGS__)
{
	__FE_COUT__ << "CFO Emulator Loopback Test runs" << __E__;
	const int  numberOfTests = __GET_ARG_IN__("numberOfTests", int);
	const bool writeFile =
	    __GET_ARG_IN__("Write ROOT file (Default := false)", bool, false);
	const std::string fileName = __GET_ARG_IN__(
	    "ROOT file name (Default := loopback.root)", std::string, "loopback.root");

	double delay_sum = 0.;
	double max_value(0), min_value(1.e10);
	// double results[numberOfTests], tests[numberOfTests];
	std::vector<double> results(numberOfTests), tests(numberOfTests);
	for(int itest = 0; itest < numberOfTests; ++itest)
	{
		getDTC()->EnableCFOLoopback();
		getDTC()->RunCableDelayLoopbackTest();
		// const DTCLib::RegisterFormatter loopbackValue = getDTC()->FormatCFOEmulationLoopbackDelayMeasure();
		// const uint32_t loopbackValue = (getDTC()->FormatCFOEmulationLoopbackDelayMeasure().value & (~(1 << 31))) * 5. / 8.;
		const double loopbackValue =
		    getDTC()->ReadCFOEmulationLoopbackDelayMeasure() * 5. / 8.;
		delay_sum += loopbackValue;
		if(max_value < loopbackValue)
			max_value = loopbackValue;
		if(min_value > loopbackValue)
			min_value = loopbackValue;

		// For plotting results
		tests[itest]   = itest;
		results[itest] = loopbackValue;
		printf("Test %3i: Result = %.2f\n", itest, results[itest]);
	}

	// Save distributions if requested
	if(writeFile)
	{
		TFile* f = new TFile(fileName.c_str(), "RECREATE");
		f->cd();
		const double xmin = (max_value > min_value)
		                        ? min_value - 0.05 * (max_value - min_value)
		                        : min_value * 0.99;
		const double xmax = (max_value > min_value)
		                        ? max_value + 0.05 * (max_value - min_value)
		                        : min_value * 1.01;
		TH1*         h_results =
		    new TH1F("hLoopbacks", "Loop-back time;loop-back [ns];", 100, xmin, xmax);
		for(int itest = 0; itest < numberOfTests; ++itest)
		{
			h_results->Fill(results[itest]);
		}
		TGraph* g = new TGraph(numberOfTests, tests.data(), results.data());
		g->SetTitle("Loop-back time;Test;Loop-back [ns]");
		g->SetName("gLoopbacks");
		g->SetLineWidth(2);
		g->SetLineColor(kRed);
		g->SetMarkerStyle(20);
		g->SetMarkerSize(0.8);
		g->SetMarkerColor(kRed);
		g->Write();
		h_results->Write();
		// f->Add(h_results);
		// f->Add(g);
		// f->Write();
		f->Close();
	}

	const double result = (numberOfTests > 0) ? delay_sum / numberOfTests : 0.;
	__SET_ARG_OUT__("Average", std::format("{:.2f} ns", result));
	__SET_ARG_OUT__("Maximum", std::format("{:.2f} ns", max_value));
	__SET_ARG_OUT__("Minimum", std::format("{:.2f} ns", min_value));
}  //end CFOEmulatorLoopbackTests()

//========================================================================
void DTCFrontEndInterface::ManualLoopbackSetup(__ARGS__)
{
	bool      setAsPassthrough = __GET_ARG_IN__("setAsPassthrough", bool);
	const int ROC_Link         = __GET_ARG_IN__("ROC_Link", int);

	__COUTV__(setAsPassthrough);
	__COUTV__(ROC_Link);

	getDTC()->EnableLink(DTCLib::DTC_Link_CFO);
	getDTC()->DisableLink(DTCLib::DTC_Link_EVB);
	for(size_t i = 0; i < DTCLib::DTC_ROC_Links.size(); ++i)
		getDTC()->DisableLink(DTCLib::DTC_ROC_Links[i]);

	if(setAsPassthrough)
	{
		getDTC()->DisableCFOLoopback();
		return;
	}
	else
		getDTC()->EnableCFOLoopback();

	getDTC()->EnableLink(DTCLib::DTC_ROC_Links[ROC_Link]);

}  //end ManualLoopbackSetup()

//========================================================================
void DTCFrontEndInterface::ValidateDTCControlRegisters(__ARGS__)
{
	constexpr uint32_t control_address = 0x9100;
	int                errorCode(0), resultCode(0);
	uint32_t           writeData, readData;

	constexpr int timeout = 100;  // for reads/writes
	constexpr int nloops =
	    100;  // test each register 100 times to catch intermittent issues
	for(int iloop = 0; iloop < nloops; ++iloop)
	{
		for(int bit = 1; bit <= 32; ++bit)
		{
			const int real_bit = (bit == 32) ? 0 : bit;  // moved bit 0 to last

			// write the data
			writeData = (real_bit == 0) ? 0 : (1u << real_bit);  // do the hard reset last
			errorCode = getDevice()->write_register(control_address, timeout, writeData);
			if(errorCode != 0)
			{
				__FE_SS__ << "Error writing register 0x" << std::hex << std::setfill('0')
				          << std::setw(4) << control_address << " bit " << std::dec
				          << real_bit << ". Error code = " << errorCode;
				__SS_THROW__;
			}

			// test the data
			errorCode = getDevice()->read_register(control_address, timeout, &readData);
			if(errorCode != 0)
			{
				__FE_SS__ << "Error reading register 0x" << std::hex << std::setfill('0')
				          << std::setw(4) << control_address << " for bit " << std::dec
				          << real_bit << ". Error code = " << errorCode;
				__SS_THROW__;
			}
			resultCode = 0;
			if(real_bit == 25)
			{  // special bit: auto-clear, resets to 0
				if(readData != 0)
					resultCode = 1;
			}
			else if(real_bit == 31)
			{  // special bit: soft reset
				if(readData != 0)
					resultCode = 1;
			}
			else if(real_bit == 0)
			{   // special bit: hard reset
				// no clear value it should have
				// if(readData != 0x10008204) errorCode = 1;
			}
			else if(readData != writeData)
				resultCode = 1;

			if(resultCode != 0)
			{
				__FE_SS__ << "Error validating register 0x" << std::hex
				          << std::setfill('0') << std::setw(4) << control_address
				          << " write + read for bit " << std::dec << real_bit
				          << ". Write = " << writeData << " and read = " << readData;
				__SS_THROW__;
			}
		}  // end bit loop
	}      // end iloop loop

	// Test block writes/reads

	// set the test status
	__SET_ARG_OUT__("Status", std::string("success"));
}  //end ValidateDTCControlRegisters

//========================================================================
/// Dummy function
void DTCFrontEndInterface::HeaderFormatTest(__ARGS__)
{
	//Dummy function
	__COUT__ << "Start..." << __E__;
	std::string result = "";
	// sleep(15);
	{
		std::vector<frontEndMacroArg_t> argsOut;
		std::vector<frontEndMacroArg_t> argsIn;
		__SET_ARG_IN__("Target ROC (Default = -1 := all ROCs)", (unsigned int)1);

		__FE_COUTV__(StringMacros::vectorToString(argsIn));
		try
		{
			runFrontEndMacro(
			    "DTC_0",                         //const std::string& targetInterfaceID,
			    "ROC FEMacro - Get ROC Status",  //const std::string& feMacroName,
			    argsIn,  //const std::vector<FEVInterface::frontEndMacroArg_t>& inputArgs,
			    argsOut);  //std::vector<FEVInterface::frontEndMacroArg_t>& outputArgs) const;

			result += StringMacros::vectorToString(argsOut) + "\n\n";
		}
		catch(const std::exception& e)
		{
			result += e.what() + std::string("\n\n");
		}

		__FE_COUTV__(StringMacros::vectorToString(argsOut));
	}

	{
		std::vector<frontEndMacroArg_t> argsOut;
		std::vector<frontEndMacroArg_t> argsIn;
		__SET_ARG_IN__("Target ROC (Default = -1 := all ROCs)", (unsigned int)1);

		__FE_COUTV__(StringMacros::vectorToString(argsIn));
		try
		{
			runFrontEndMacro(
			    "DTC_1",                         //const std::string& targetInterfaceID,
			    "ROC FEMacro - Get ROC Status",  //const std::string& feMacroName,
			    argsIn,  //const std::vector<FEVInterface::frontEndMacroArg_t>& inputArgs,
			    argsOut);  //std::vector<FEVInterface::frontEndMacroArg_t>& outputArgs) const;

			result += StringMacros::vectorToString(argsOut) + "\n\n";
		}
		catch(const std::exception& e)
		{
			result += e.what() + std::string("\n\n");
		}

		__FE_COUTV__(StringMacros::vectorToString(argsOut));
	}

	{
		std::vector<frontEndMacroArg_t> argsOut;
		std::vector<frontEndMacroArg_t> argsIn;

		__FE_COUTV__(StringMacros::vectorToString(argsIn));
		try
		{
			runFrontEndMacro(
			    "DTC_1",                 //const std::string& targetInterfaceID,
			    "Get Firmware Version",  //const std::string& feMacroName,
			    argsIn,  //const std::vector<FEVInterface::frontEndMacroArg_t>& inputArgs,
			    argsOut);  //std::vector<FEVInterface::frontEndMacroArg_t>& outputArgs) const;

			result +=
			    StringMacros::decodeURIComponent(StringMacros::vectorToString(argsOut)) +
			    "\n\n";
		}
		catch(const std::exception& e)
		{
			result += e.what() + std::string("\n\n");
		}

		__FE_COUTV__(StringMacros::vectorToString(argsOut));
	}

	{
		std::vector<frontEndMacroArg_t> argsOut;
		std::vector<frontEndMacroArg_t> argsIn;
		__SET_ARG_IN__("rocLinkIndex", (unsigned int)1);
		__SET_ARG_IN__("address", (unsigned int)3);

		__FE_COUTV__(StringMacros::vectorToString(argsIn));
		try
		{
			runFrontEndMacro(
			    "DTC_1",     //const std::string& targetInterfaceID,
			    "ROC Read",  //const std::string& feMacroName,
			    argsIn,  //const std::vector<FEVInterface::frontEndMacroArg_t>& inputArgs,
			    argsOut);  //std::vector<FEVInterface::frontEndMacroArg_t>& outputArgs) const;

			result += StringMacros::vectorToString(argsOut) + "\n\n";
		}
		catch(const std::exception& e)
		{
			result += e.what() + std::string("\n\n");
		}

		__FE_COUTV__(StringMacros::vectorToString(argsOut));
	}

	__SET_ARG_OUT__("setRegister", result);
	__COUT__ << "End." << __E__;
}  //end HeaderFormatTest()

//========================================================================
void DTCFrontEndInterface::loopbackTest(int step)
{
	// TODO: read from configuration
	const int          ROCsPerDTC   = 6;
	const unsigned int DTCsPerChain = 8;  //getConfigurationManager()
	    //->getNode("/Mu2eGlobalsTable/SyncDemoConfig/DTCsPerChain").getValue<unsigned int>();

	unsigned int n_steps = DTCsPerChain * ROCsPerDTC;  // 6 * 10 = 60

	//call virtual readStatus
	if(step == -1)
		step = getIterationIndex();  // get the current index

	// alternate with the CFO
	if((step % 2) != 0)
	{
		indicateIterationWork();
		__FE_COUT__ << "Step " << step << " is odd, letting the CFO have a turn" << __E__;
		return;
	}
	unsigned int loopback_step = step / 2;
	// end by restoring the status of the registers
	if(loopback_step >= n_steps)
	{
		__FE_COUT__ << "Loopback over!" << __E__;
		return;
	}

	// select the active DTC AND ROC
	int active_DTC = loopback_step / ROCsPerDTC;  // each DTC can have up to 6 ROCs
	int active_ROC = loopback_step % ROCsPerDTC;  // [0,5] possible link of the DTC

	__FE_COUT__ << "step " << loopback_step << ") active DTC: " << active_DTC
	            << " active ROC on link: " << active_ROC << __E__;

	// set up the DTC based on its position in the chain
	if(active_DTC == dtc_location_in_chain_)
	{
		// 0x9100 set bit 28 = 1
		__FE_COUT__ << "DTC" << active_DTC << "loopback mode ENABLE" << __E__;
		getDTC()->DisableCFOLoopback();
		// getDTC()->EnableCFOLoopback();
	}
	else
	{
		// 0x9100 set bit 28 = 0
		__FE_COUT__ << "active DTC = " << active_DTC
		            << " is NOT this DTC = " << dtc_location_in_chain_
		            << "... pass signal through" << __E__;
		getDTC()->EnableCFOLoopback();
		// getDTC()->DisableCFOLoopback();
	}
	// enable the links of the DTC
	DTCLib::DTC_Link_ID link = static_cast<DTCLib::DTC_Link_ID>(active_ROC);
	getDTC()->EnableReceiveCFOLink();
	getDTC()->EnableTransmitCFOLink();
	getDTC()->EnableLink(link,
	                     DTCLib::DTC_LinkEnableMode(true, true));  // enable Tx and Rx

	indicateIterationWork();
}  //end loopbackTest()

//========================================================================
/// Macro needed by OTS:
///
/// 1) Write directory of flash (file flash_map.txt)
/// 	a. Use action 9 using file entries
///
/// 2) Program_flash(image_index, filename,n times, verify)
///		0. Readback current SPI Flash directory index map
///		1. If map path given, verify they match, else if not given use existing map as address lookup
///		11. If map given and no match, then write given map, and verify again, then error if no match
/// 	a. Read file, calculate length in bytes
/// 	b. Erase flash calling action 3 (address from the flash_map.txt, length)
/// 	c. Poll register 128 until equal 0x8000
/// 	d. Start writing blocks in 1 KB size calling action 8 (address+ offset)
/// 	e. Poll register 128 until equal 0x8000
/// 	f. Check register 132 = 0x0 for errors, if errors repeat block write n times
/// 	g. Repeat from d until the end of the file
/// 	h. If verify read back the all flash sector using action 7, in blocks of 128 bytes
/// 	i. Check against the file
///
/// Note: COULD TAKE 1 HOUR (in May 2025 HEERC tests, takes about 15 minutes)
///
/// 3) start programming the fpga with action 4 (index)
///
/// 4) readback function (index, size in byte, output file name) reads the flash sector using action 7 and writes in the file
void DTCFrontEndInterface::ProgramROCs(__ARGS__)
{
	uint32_t rocLinkIndexVal = __GET_ARG_IN__(
	    "Target ROC or Mask (Default = -1 := all ROCs, or 0x111111 := all)",
	    uint32_t,
	    -1 /* ALL */);
	bool usingRocMask = false;
	if(rocLinkIndexVal != uint32_t(-1) && rocLinkIndexVal > 5)
	{
		usingRocMask = true;
		__FE_COUT__ << "Using ROC Link Mask value: 0x" << std::hex
		            << (unsigned int)rocLinkIndexVal << std::dec << __E__;
	}

	DTCLib::DTC_Link_ID rocLinkIndex =
	    DTCLib::DTC_Link_ID(usingRocMask ? -1 : rocLinkIndexVal);
	__FE_COUT__ << "rocLinkIndexVal = 0x" << std::hex << rocLinkIndexVal << __E__;
	__FE_COUTV__(usingRocMask);
	__FE_COUTV__(rocLinkIndex);

	std::string mapPath =
	    __GET_ARG_IN__("Path to Directory map file (Default := do not use)", std::string);
	bool writeMap =
	    __GET_ARG_IN__("Write Directory map to SPI Flash (Default := false)", bool);
	bool    verifyMap  = __GET_ARG_IN__("Verify Directory map (Default := false)", bool);
	uint8_t imageIndex = __GET_ARG_IN__("Image Index (Default := 0)", uint8_t);
	std::string bitfilePath = __GET_ARG_IN__(
	    "Path to Bitfile (Default := do not write bitfile, only program from Image "
	    "Index)",
	    std::string);
	bool write  = __GET_ARG_IN__("Write Bitfile to SPI Flash (Default := false)", bool);
	bool verify = __GET_ARG_IN__("Verify with Bitfile Readback (Default := false)", bool);
	bool program = __GET_ARG_IN__("Do program from Image Index (Default := false)", bool);
	uint32_t debugForceSize =
	    __GET_ARG_IN__("For Debug, force Write size (Default := do not force)", uint32_t);

	__FE_COUTV__(mapPath);
	__FE_COUTV__(writeMap);
	__FE_COUTV__(verifyMap);
	__FE_COUTV__((int)imageIndex);
	__FE_COUTV__(bitfilePath);
	__FE_COUTV__(write);
	__FE_COUTV__(verify);
	__FE_COUTV__(program);
	__FE_COUTV__(debugForceSize);

	std::stringstream resultsSs;
	resultsSs << __E__;

	std::vector<std::string /* ROC UID */> targetROCs;
	for(auto& roc : rocs_)
	{
		if(usingRocMask)
			__FE_COUT__ << "0x" << std::hex << (1 << (int(roc.second->getLinkID()) * 4))
			            << " vs rocLinkIndexVal = 0x" << std::hex << rocLinkIndexVal
			            << __E__;

		if((!usingRocMask &&  //use ROC index
		    (rocLinkIndex == DTCLib::DTC_Link_ID::DTC_Link_ALL ||
		     roc.second->getLinkID() == rocLinkIndex)) ||
		   (usingRocMask &&  //use ROC mask
		    ((1 << (int(roc.second->getLinkID()) * 4)) & rocLinkIndexVal)))
		{
			targetROCs.push_back(roc.first);
			__FE_COUTV__(roc.first);
			__FE_COUTV__(roc.second->getLinkID());
		}
	}  //end target ROC search loop
	if(!targetROCs.size())
	{
		__FE_SS__ << "Fatal error - Target ROC or Mask '" << int(rocLinkIndexVal)
		          << " (0x" << std::hex << rocLinkIndexVal
		          << ")' not found in DTC's instantiated ROCs (make sure your ROC is "
		             "enabled)! Here is the list of "
		             "enabled ROC links: ";
		int i = 0;
		for(auto& roc : rocs_)
			ss << (i++ ? ", " : "") << roc.second->getLinkID();
		ss << __E__;
		__FE_SS_THROW__;
	}

	//handle directory map
	std::vector<uint32_t> mapWriteData;
	if(mapPath != "Default" && mapPath != "")
	{
		__FE_COUT_INFO__ << "SPI directory map load start: path='" << mapPath << "'"
		                 << " writeMap=" << writeMap << " verifyMap=" << verifyMap
		                 << " targetROCs=" << targetROCs.size() << __E__;
		__COUTV__(mapPath);

		char       line[100];
		std::FILE* fp = std::fopen(mapPath.c_str(), "r");
		if(!fp)
		{
			__FE_SS__ << "Could not open file at " << mapPath << ". Error: " << errno
			          << " - " << strerror(errno) << __E__;
			__FE_SS_THROW__;
		}

		//each line is 32-bit address
		while(fgets(line, 100, fp))
		{
			uint32_t value = std::stoi(line, nullptr, 16);
			__FE_COUT__ << value << " 0x" << std::hex << value << __E__;
			mapWriteData.push_back(value);
		}
		fclose(fp);

		__FE_COUT_INFO__ << "SPI directory map loaded: entries=" << mapWriteData.size()
		                 << __E__;
		__FE_COUTV__(StringMacros::vectorToString(mapWriteData));
		if(writeMap)
		{
			__FE_COUT_INFO__ << "SPI directory map write start: targetROCs="
			                 << targetROCs.size() << __E__;
			for(auto& roc : targetROCs)
			{
				__FE_COUT_INFO__ << "SPI directory map write ROC start: roc='" << roc
				                 << "' link=" << rocs_.at(roc)->getLinkID() << __E__;
				__FE_COUTV__(roc);
				__FE_COUTV__(rocs_.at(roc)->getLinkID());
				rocs_.at(roc)->writeSPIFlashDirectory(mapWriteData);
				__FE_COUT_INFO__ << "SPI directory map write ROC done: roc='" << roc
				                 << "' link=" << rocs_.at(roc)->getLinkID() << __E__;
			}  //end roc loop to write map
			__FE_COUT_INFO__ << "SPI directory map write done: targetROCs="
			                 << targetROCs.size() << __E__;
		}
		else
			__FE_COUT_INFO__ << "SPI directory map write skipped" << __E__;

		if(verifyMap)
		{
			__FE_COUT_INFO__ << "SPI directory map verify start: targetROCs="
			                 << targetROCs.size() << __E__;
			for(auto& roc : targetROCs)
			{
				__FE_COUT_INFO__ << "SPI directory map verify ROC start: roc='" << roc
				                 << "' link=" << rocs_.at(roc)->getLinkID() << __E__;
				__FE_COUTV__(roc);
				__FE_COUTV__(rocs_.at(roc)->getLinkID());

				std::vector<uint16_t> readData;
				rocs_.at(roc)->readSPIFlashBlock(readData,
				                                 0 /* directory map location */,
				                                 16 * 4 /* max map location */);

				size_t i = 0;
				for(; i < mapWriteData.size(); ++i)
				{
					if(readData[i * 2] != uint16_t(mapWriteData[i]) ||
					   readData[i * 2 + 1] != uint16_t(mapWriteData[i] >> 16))
					{
						__FE_SS__ << roc << " link=" << rocs_.at(roc)->getLinkID()
						          << ", Mismatch in Directory Map! Expected 0x "
						          << std::hex << std::setw(4) << std::setfill('0')
						          << uint16_t(mapWriteData[i]) << " "
						          << uint16_t(mapWriteData[i] >> 16) << " and read: 0x"
						          << std::hex << std::setw(4) << std::setfill('0')
						          << readData[i * 2] << " " << readData[i * 2 + 1]
						          << __E__;
						__FE_SS_THROW__;
					}
				}
				i *= 2;  //jumpt to 16-bit indices
				for(; i < 16 * 2; ++i)
					if(readData[i] != uint16_t(-1))
					{
						__FE_SS__ << roc << " link=" << rocs_.at(roc)->getLinkID()
						          << ", Mismatch in Directory Map! Expected no further "
						             "addresses (i.e., -1) and read: 0x"
						          << std::hex << std::setw(4) << std::setfill('0')
						          << readData[i];
						__FE_SS_THROW__;
					}

				__FE_COUT_INFO__ << "SPI directory map verify ROC done: roc='" << roc
				                 << "' link=" << rocs_.at(roc)->getLinkID() << __E__;
				resultsSs << roc << " link=" << rocs_.at(roc)->getLinkID()
				          << ", Directory map verified." << __E__;
			}  //end roc loop to verify map
			__FE_COUT_INFO__ << "SPI directory map verify done: targetROCs="
			                 << targetROCs.size() << __E__;
		}
	}  //end directory map handling

	if(imageIndex >= mapWriteData.size())
	{
		__FE_SS__ << "Illegal image index, must be less than Directory size: "
		          << imageIndex << " must be < " << mapWriteData.size() << __E__;
		__FE_SS_THROW__;
	}
	uint32_t startAddress = mapWriteData[imageIndex];

	__FE_COUT_INFO__ << "SPI start address selected: imageIndex=" << int(imageIndex)
	                 << " startAddress=0x" << std::hex << std::setw(8) << std::setfill('0')
	                 << startAddress << std::dec << __E__;

	std::string contents, fullpath;
	if(bitfilePath != "Default" && bitfilePath != "")
		fullpath = bitfilePath;

	if(fullpath != "")
	{
		__FE_COUT_INFO__ << "SPI bitfile load start: path='" << fullpath << "'" << __E__;
		__COUTV__(fullpath);

		std::FILE* fp = std::fopen(fullpath.c_str(), "rb");
		if(!fp)
		{
			__FE_SS__ << "Could not open file at " << fullpath << ". Error: " << errno
			          << " - " << strerror(errno) << __E__;
			__FE_SS_THROW__;
		}

		std::fseek(fp, 0, SEEK_END);
		contents.resize(std::ftell(fp));
		std::rewind(fp);
		std::fread(&contents[0], 1, contents.size(), fp);
		std::fclose(fp);

		__FE_COUTV__(contents.size());

		__FE_COUT_INFO__ << "SPI bitfile load done: bytes=" << contents.size() << __E__;
		resultsSs << "Loaded file '" << fullpath << "' of size=" << contents.size()
		          << __E__;
	}

	// b. Erase flash calling action 3 (address from the dlash_map.txt, length)

	if(debugForceSize && contents.size() > debugForceSize)
	{
		__FE_COUT__ << "Forcing size to " << debugForceSize << __E__;
		contents.resize(debugForceSize);  //force for debugging
	}

	__FE_COUT_INFO__ << "SPI programming request: bytes=" << contents.size()
	                 << " startAddress=0x" << std::hex << startAddress << std::dec
	                 << " write=" << write << " verify=" << verify
	                 << " program=" << program << " targetROCs=" << targetROCs.size()
	                 << __E__;

	setFEMacroPercentDone(0);

	//first launch erase
	if(write && contents.size())
	{
		__FE_COUT_INFO__ << "SPI erase start: bytes=" << contents.size()
		                 << " startAddress=0x" << std::hex << startAddress << std::dec
		                 << " targetROCs=" << targetROCs.size() << __E__;
		std::chrono::time_point<std::chrono::steady_clock> eraseStartTime =
		    std::chrono::steady_clock::now();
		std::map<std::string, bool> eraseLockHeld;
		try
		{
			for(auto& roc : targetROCs)
			{
				__FE_COUT_INFO__ << "SPI erase launch: roc='" << roc
				                 << "' link=" << rocs_.at(roc)->getLinkID() << __E__;
				rocs_.at(roc)->eraseSPIFlashBlock(
				    contents.size(), startAddress, false /* waitForDone */);
				eraseLockHeld[roc] = true;
			}

			for(auto& roc : targetROCs)
			{
				size_t acceptPolls = 0;
				while(rocs_.at(roc)->isActionDone())
				{
					if(acceptPolls > 5 * 100 /* 5 seconds */)
					{
						__FE_SS__ << "SPI ERASE TIMEOUT: roc='" << roc
						          << "' link=" << rocs_.at(roc)->getLinkID()
						          << " phase=command-accepted timeout=5s" << __E__;
						__FE_SS_THROW__;
					}
					usleep(1000 * 10 /* 10 ms */);
					++acceptPolls;
				}
				__FE_COUT_INFO__ << "SPI erase accepted: roc='" << roc
				                 << "' link=" << rocs_.at(roc)->getLinkID() << __E__;
			}

			for(auto& roc : targetROCs)
			{
				size_t donePolls = 0;
				while(!rocs_.at(roc)->isActionDone(nullptr, true /* releaseLockOnDone */))
				{
					if(donePolls > 180 * 100 /* 180 seconds */)
					{
						__FE_SS__ << "SPI ERASE TIMEOUT: roc='" << roc
						          << "' link=" << rocs_.at(roc)->getLinkID()
						          << " phase=complete timeout=180s" << __E__;
						__FE_SS_THROW__;
					}
					usleep(1000 * 10 /* 10 ms */);
					++donePolls;
				}
				eraseLockHeld[roc] = false;
				long long eraseMs = std::chrono::duration_cast<std::chrono::milliseconds>(
				                        std::chrono::steady_clock::now() - eraseStartTime)
				                        .count();
				__FE_COUT_INFO__ << "SPI erase done: roc='" << roc
				                 << "' link=" << rocs_.at(roc)->getLinkID()
				                 << " elapsedMs=" << eraseMs << __E__;
			}
		}
		catch(...)
		{
			for(auto& rocLock : eraseLockHeld)
				if(rocLock.second)
				{
					__FE_COUT_WARN__ << "Force-clearing pending ROC action lock for '"
					                 << rocLock.first << "'" << __E__;
					rocs_.at(rocLock.first)->forceClearActionLock();
				}
			throw;
		}

		setFEMacroPercentDone(10);

		// d. Start writing blocks in 1 KB size calling action 8 (address+ offset)
		__FE_COUT_INFO__ << "SPI write start: bytes=" << contents.size()
		                 << " blockSize=1024 startAddress=0x" << std::hex << startAddress
		                 << std::dec << __E__;
		// return; //block writing bitfile

		std::chrono::time_point<std::chrono::steady_clock> transferStartTime =
		    std::chrono::steady_clock::now();

		for(size_t i = 0; i < contents.size(); i += 1024)
		{
			size_t writeSize = contents.size() - i;
			if(writeSize > 1024)
				writeSize = 1024;
			__FE_COUTT__ << "SPI write chunk start: offset=" << i << " size=" << writeSize
			             << " flashAddr=0x" << std::hex << (startAddress + i) << std::dec
			             << __E__;

			{
				std::vector<uint16_t> writeData;
				for(size_t j = 0; j < writeSize; j += 2)
				{
					writeData.push_back(uint16_t(contents[i + j]) & 0xFF);
					writeData.back() |= (contents[i + j + 1] << 8);
				}

				if(TTEST(32))
				{
					std::stringstream outss;
					for(auto& val : writeData)
						outss << std::hex << " 0x" << val;
					__FE_COUTV__(outss.str());
				}

				for(auto& roc : targetROCs)
				{
					__FE_COUTV__(roc);
					__FE_COUTV__(rocs_.at(roc)->getLinkID());
					rocs_.at(roc)->writeSPIFlashBlock(
					    writeData, startAddress + i, true /* waitForDone */);
					// Note: writeSPIFlashBlock with waitForDone=true already
					// waits for completion, checks status, and throws on error.
					// No second polling loop needed (it was causing a double-unlock
					// of actionLock_ leading to undefined behavior and 0xffff reads).
					__FE_COUTT__ << "SPI write chunk done: roc='" << roc
					             << "' link=" << rocs_.at(roc)->getLinkID()
					             << " offset=" << i << " size=" << writeSize
					             << " flashAddr=0x" << std::hex << (startAddress + i)
					             << std::dec << __E__;
				}  //end ROC write SPI block loop
			}

			if(writeSize)
			{
				setFEMacroPercentDone(10 + 70 * (i + writeSize) / contents.size());
				size_t currentPercent = (i + writeSize) * 100 / contents.size();
				size_t prevPercent    = i > 0 ? (i * 100 / contents.size()) : 0;

				__FE_COUTT__ << "SPI write chunk #" << int(i / 1024)
				             << " done: offset=" << i << " size=" << writeSize
				             << " totalBytes=" << contents.size()
				             << " progress=" << currentPercent << "%" << __E__;

				// Log at INFO level every 10% so it is visible in the message viewer
				if(currentPercent / 10 != prevPercent / 10 ||
				   i + writeSize >= contents.size())
					__FE_COUT_INFO__ << "SPI write progress: " << currentPercent << "% "
					                 << "(" << (i + writeSize) << "/" << contents.size()
					                 << " bytes)" << __E__;
			}

			long long ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
			                   std::chrono::steady_clock::now() - transferStartTime)
			                   .count();
			if(ns > 1000)  //prevent divide by 0
			{
				__FE_COUTT__ << "SPI write elapsedMs=" << ns / 1000.0 / 1000.0
				             << " averageRateMBps="
				             << ((double)(i + writeSize)) / (ns / 1000.0) << __E__;
			}

			// if (i > 4000)
			// 	break; //debug, stop after first write
		}  //end write bitfile loop

		resultsSs << "Write of bitfile to address 0x" << std::hex << std::setw(8)
		          << std::setfill('0') << startAddress << __E__;
		long long writeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		                        std::chrono::steady_clock::now() - transferStartTime)
		                        .count();
		__FE_COUT_INFO__ << "SPI write done: bytes=" << contents.size()
		                 << " elapsedMs=" << writeMs << __E__;
	}
	else
		__FE_COUT_INFO__ << "SPI erase/write skipped: write=" << write
		                 << " bytes=" << contents.size() << __E__;

	// return; //for debug

	// h. If verify read back the all flash sector using action 7, in blocks of 1016 bytes
	constexpr size_t VERIFY_CHUNK_SIZE = 1016;
	if(verify && contents.size())
	{
		setFEMacroPercentDone(80);
		__FE_COUT_INFO__ << "SPI verify start: bytes=" << contents.size()
		                 << " chunkSize=" << VERIFY_CHUNK_SIZE << " startAddress=0x"
		                 << std::hex << startAddress << std::dec << __E__;

		for(auto& roc : targetROCs)
		{
			__FE_COUTV__(roc);
			__FE_COUTV__(rocs_.at(roc)->getLinkID());
			std::vector<uint16_t> readData;  //full bitfile is assembled here
			size_t                lastVerifyPercent = 0;
			std::chrono::time_point<std::chrono::steady_clock> verifyStartTime =
			    std::chrono::steady_clock::now();

			for(size_t i = 0; i < contents.size(); i += VERIFY_CHUNK_SIZE)
			{
				size_t readSize = contents.size() - i;
				if(readSize > VERIFY_CHUNK_SIZE)
					readSize = VERIFY_CHUNK_SIZE;

				__FE_COUTT__ << "SPI verify chunk start: roc='" << roc
				             << "' link=" << rocs_.at(roc)->getLinkID() << " offset=" << i
				             << " size=" << readSize << " flashAddr=0x" << std::hex
				             << (startAddress + i) << std::dec
				             << " totalBytes=" << contents.size() << __E__;

				//append to readData
				rocs_.at(roc)->readSPIFlashBlock(readData, startAddress + i, readSize);

				__FE_COUTT__ << "SPI verify chunk done: roc='" << roc
				             << "' link=" << rocs_.at(roc)->getLinkID() << " offset=" << i
				             << " readWords=" << readData.size() << __E__;

				// Log verify progress at INFO level every 10%
				size_t currentPercent = (i + readSize) * 100 / contents.size();
				if(currentPercent / 10 != lastVerifyPercent / 10 ||
				   i + readSize >= contents.size())
				{
					__FE_COUT_INFO__ << "SPI verify progress: " << currentPercent << "% "
					                 << "(" << (i + readSize) << "/" << contents.size()
					                 << " bytes)" << __E__;
					lastVerifyPercent = currentPercent;
				}

				//partial word verify loop
				for(size_t j = i; j < i + readSize; j += 2)
				{
					if(uint8_t(contents[j]) != uint8_t(readData[j / 2]) ||
					   uint8_t(contents[j + 1]) != uint8_t(readData[j / 2] >> 8))
					{
						// Diagnostic: read ROC registers at time of mismatch
						auto doneAtMismatch =
						    rocs_.at(roc)->readRegister(128 /*ROC_ADDRESS_ACTION_DONE*/);
						auto countAtMismatch = rocs_.at(roc)->readRegister(
						    129 /*ROC_ADDRESS_ACTION_READ_SIZE*/);
						auto statusAtMismatch = rocs_.at(roc)->readRegister(
						    132 /*ROC_ADDRESS_ACTION_STATUS*/);

						__FE_SS__
						    << "SPI VERIFY MISMATCH: roc='" << roc
						    << "' link=" << rocs_.at(roc)->getLinkID()
						    << " offset=" << std::dec << j << " flashAddr=0x" << std::hex
						    << (startAddress + j) << " chunkOffset=" << std::dec << i
						    << " chunkSize=" << readSize
						    << " totalBytes=" << contents.size()
						    << " verifyChunkSize=" << VERIFY_CHUNK_SIZE << " expected=0x"
						    << std::hex << std::setw(2) << std::setfill('0')
						    << (uint16_t(contents[j + 1]) & 0xFF)
						    << (uint16_t(contents[j]) & 0xFF) << " got=0x"
						    << (uint16_t(readData[j / 2] >> 8) & 0xFF)
						    << (uint16_t(readData[j / 2]) & 0xFF) << " reg128=0x"
						    << doneAtMismatch << " reg129=0x" << countAtMismatch
						    << " reg132=0x" << statusAtMismatch;

						// Dump surrounding readback words for context
						ss << ". Readback around mismatch (word index, value):";
						size_t dumpStart = (j / 2 >= 4) ? (j / 2 - 4) : 0;
						size_t dumpEnd   = std::min(j / 2 + 5, readData.size());
						for(size_t d = dumpStart; d < dumpEnd; ++d)
							ss << " [" << std::dec << d << "]=0x" << std::hex
							   << std::setw(4) << std::setfill('0') << readData[d];
						ss << __E__;

						__FE_SS_THROW__;
					}
				}  //end partial verify loop

			}  //end read check

			// now verify size
			if(readData.size() * 2 != contents.size())
			{
				__FE_SS__ << "At roc '" << roc << "' link=" << rocs_.at(roc)->getLinkID()
				          << ", SPI VERIFY SIZE MISMATCH: expectedBytes="
				          << contents.size() << " readBytes=" << readData.size() * 2
				          << __E__;
				__FE_SS_THROW__;
			}

			long long verifyMs = std::chrono::duration_cast<std::chrono::milliseconds>(
			                         std::chrono::steady_clock::now() - verifyStartTime)
			                         .count();
			__FE_COUT_INFO__ << "SPI verify done: roc='" << roc
			                 << "' link=" << rocs_.at(roc)->getLinkID()
			                 << " bytes=" << contents.size() << " elapsedMs=" << verifyMs
			                 << __E__;

			resultsSs << "At roc '" << roc << "' link=" << rocs_.at(roc)->getLinkID()
			          << ", SPI data verified." << __E__;
		}  //end launch of ROC erase SPI block loop
	}  //end verify

	if(!program)
	{
		__SET_ARG_OUT__("Result", resultsSs.str());
		return;  //block programming
	}

	// 3) start programming the fpga with action 4 (index)
	//first launch program
	setFEMacroPercentDone(90);
	__FE_COUT__ << "Start programing from SPI..." << __E__;
	for(auto& roc : targetROCs)
	{
		__FE_COUTV__(roc);
		__FE_COUTV__(rocs_.at(roc)->getLinkID());
		rocs_.at(roc)->programFromSPIByAddress(startAddress, false /* waitForDone */);
	}  //end launch of ROC erase SPI block loop

	__FE_COUT__ << "Checking that program done..." << __E__;
	//then check for erase done
	{
		bool                                                 allDone = true;
		DTCLib::roc_data_t                                   readStatus;
		std::map<std::string /* ROC UIC */, bool /* done */> doneMap;
		std::map<std::string /* ROC UIC */, bool /* done */> lostConnectionMap;
		size_t                                               attempt = 0;
		do
		{
			allDone = true;
			for(auto& roc : targetROCs)
			{
				if(doneMap[roc])
					continue;  //skip those done

				try
				{
					doneMap[roc] = rocs_.at(roc)->isActionDone(
					    &readStatus, true /* releaseLockOnDone */);
					if(lostConnectionMap
					       [roc])  //if previously lost connection, consider it back!
					{
						__FE_COUT__ << "At roc '" << roc
						            << "' link=" << rocs_.at(roc)->getLinkID()
						            << ", back after connection lost! Marking done!"
						            << __E__;
						doneMap[roc] = true;
					}
				}
				catch(...)
				{
					__FE_COUT__
					    << "At roc '" << roc << "' link=" << rocs_.at(roc)->getLinkID()
					    << ", Caught exception... ignorning while FPGA down." << __E__;
					sleep(1);
					getDTC()->SoftReset();
					doneMap[roc]           = false;
					lostConnectionMap[roc] = true;  //mark connection lost
				}

				if(!doneMap[roc])
					allDone = false;
				else
				{
					if(readStatus)
					{
						__FE_SS__
						    << "At roc '" << roc
						    << "' link=" << rocs_.at(roc)->getLinkID()
						    << ", Non-zero status received after SPI program action: 0x"
						    << std::hex << readStatus << __E__;
						__FE_SS_THROW__;
					}
					__FE_COUT__ << roc << " link=" << rocs_.at(roc)->getLinkID()
					            << ", done with program from SPI action." << __E__;
					resultsSs << roc << " link=" << rocs_.at(roc)->getLinkID()
					          << ", done with program from SPI action." << __E__;
				}
			}  //end launch of ROC erase SPI block loop

			if(!allDone && ++attempt > 120 * 1 /* 1 mins */)
			{
				__FE_SS__ << "Timeout waiting for SPI flash program action! Check for "
				             "more info with ROC Read to 128."
				          << __E__;
				__FE_SS_THROW__;
			}
			else if(!allDone)
				usleep(1000 * 500 /* 500 ms */);
		} while(!allDone);
	}  //end check for program done

	setFEMacroPercentDone(100);
	__SET_ARG_OUT__("Result", resultsSs.str());
	__FE_COUT__ << "Done with all program actions!" << __E__;
}  //end ProgramROCs()

// DEFINE_OTS_INTERFACE(DTCFrontEndInterface)
