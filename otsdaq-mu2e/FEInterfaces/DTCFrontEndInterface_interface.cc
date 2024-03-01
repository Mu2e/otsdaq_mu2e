#include "otsdaq-mu2e/FEInterfaces/DTCFrontEndInterface.h"
#include "otsdaq/FECore/MakeInterface.h"
#include "otsdaq/Macros/BinaryStringMacros.h"
#include "otsdaq/Macros/InterfacePluginMacros.h"


#include <fstream>


using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "DTCFrontEndInterface"

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
		__FE_COUT_INFO__ << "Hardware Dev Mode identified, so forcing CFO emulation mode for DTC" << __E__;
		emulate_cfo_ = true;
	}
	else
		emulate_cfo_ = getSelfNode().getNode("EmulateCFO").getValue<bool>();
	__FE_COUTV__(emulate_cfo_);

	if(emulatorMode_)
	{
		__FE_COUT__ << "Emulator DTC mode starting up..." << __E__;
		createROCs();
		registerFEMacros();
		return;
	}
	// else not emulator mode

	DTCInstantiate();

}  // end constructor()

//==========================================================================================
DTCFrontEndInterface::~DTCFrontEndInterface(void)
{
	{
		// uint32_t lossOfLockReadData = registerRead(0x93c8);  // read loss-of-lock counter
		__FE_COUTV__(thisDTC_->FormatRXCDRUnlockCountCFOLink());
	}

	// destroy ROCs before DTC destruction
	rocs_.clear();


	if(thisDTC_)
		delete thisDTC_;

	__FE_COUT__ << "Destructed." << __E__;
}  // end destructor()

//==============================================================================
void DTCFrontEndInterface::registerFEMacros(void)
{
	__FE_COUT__ << "Registering DTC FE Macros..." << __E__;

	mapOfFEMacroFunctions_.clear();

	// clang-format off

	// registerFEMacroFunction(
	// 	"Get Firmware Version",  // feMacroName
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&DTCFrontEndInterface::GetFirmwareVersion),  // feMacroFunction
	// 				std::vector<std::string>{},
	// 				std::vector<std::string>{"Firmware Version Date"},  // namesOfOutputArgs
	// 				1,  //"allUsers:0 | TDAQ:255");
	// 				"*",
	// 				"Read the modification date of the DTC firmware using <b>MON/DD/20YY HH:00</b> format."
	// );
					
	// registerFEMacroFunction(
	// 	"Flash_LEDs",  // feMacroName
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&DTCFrontEndInterface::FlashLEDs),  // feMacroFunction
	// 				std::vector<std::string>{},
	// 				std::vector<std::string>{},  // namesOfOutputArgs
	// 				1,  // requiredUserPermissions
	// 				"*", // allowedCallingFEs
	// 				"Flashes the LEDs, for example to help find the target FPGA in a multi-FPGA deployment."
	// );

	// registerFEMacroFunction(
	// 	"Get Status",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&DTCFrontEndInterface::GetStatus),            // feMacroFunction
	// 				std::vector<std::string>{},  // namesOfInputArgs
	// 				std::vector<std::string>{"Status"},
	// 				1,  // requiredUserPermissions
	// 				"*", // allowedCallingFEs
	// 				"Reads and displays all registers in a human-readable format."
	// );

	// registerFEMacroFunction(
	// 	"Get Simple Status",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&DTCFrontEndInterface::GetSimpleStatus),            // feMacroFunction
	// 				std::vector<std::string>{},  // namesOfInputArgs
	// 				std::vector<std::string>{"Status"},
	// 				1,  // requiredUserPermissions
	// 				"*", // allowedCallingFEs
	// 				"Similar to <b>Get Status</b>, this FE Macro fetches data from a select few registers. "
	// 				"Registers <b>DTC Control, ROC Emulation Enable, Link Enable, SERDES Reset, and SERDES Reset Done</b> are displayed in a human-readable format."
	// );



	registerFEMacroFunction(
		"ROC Setup",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::SetupROCs),
						std::vector<std::string>{"Target ROC (Default = -1 := all ROCs)",
				        						"Set Link RX/TX Enable (Default := false)",
				        						"Set ROC Emulation Enable (Default := false)",
				        						"ROC Emulation Type (Default = 0: Internal, 1: Fiber-Loopback, 2: External)",
				        						"ROC generated Data Payload fragment packet count (11-bits, Default := 16)",
											},
						std::vector<std::string>{"Result"},
					1,   // requiredUserPermissions
					"*",
					"Use this to control the parameters of the ROC Emulation of the DTC. "
					"Internal ROC emulation does not require any fibers; the ROC is emulated without using the link SERDES rx/tx. "
					"Fiber-Loopback ROC emulation requires a single fiber connected between the link SERDES rx <==> tx channels. "
					"External ROC emulation uses the link SERDES rx/tx as though it is a ROC; this means the rx/tx channels could be connected to another DTC, and the other DTC would communicate to this emulated ROC following the ROC protocol of docdb-4914."
					"\n\n"
					"Note: Internal and External ROC emulation can co-exist on the same DTC link. While Fiber-Loopback ROC emulation is exclusive to the other two types and will take precedence."
	);

	registerFEMacroFunction(
		"ROC Write",  // feMacroName
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::WriteROC),  // feMacroFunction
					std::vector<std::string>{"rocLinkIndex", "address", "writeData"},
					std::vector<std::string>{},  // namesOfOutput
					1,                           // requiredUserPermissions
					"*",						 // allowedCallingFEs
					"This FE Macro writes data to a specific register on a specified link."
	);

	registerFEMacroFunction(
		"ROC Read",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::ReadROC),                  // feMacroFunction
					std::vector<std::string>{"rocLinkIndex", "address"},  // namesOfInputArgs
					std::vector<std::string>{"readData"},
					1,   // requiredUserPermissions
					"*",
					"This FE Macro reads data from a ROC given a link and address."
	);

	registerFEMacroFunction(
		"ROC Block Read",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::BockReadROC),
					std::vector<std::string>{"rocLinkIndex", "address", 
						"Number Of 16-bit words to Read (Default := 8)", 
						"incrementAddress (Default := false)"},
					std::vector<std::string>{"readData"},
					1,   // requiredUserPermissions 
					"*",
					"This FE Macro is used to read multiple words from a ROC."
	);

	registerFEMacroFunction(
		"ROC Block Write",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::BockWriteROC),
					std::vector<std::string>{"rocLinkIndex", "address",
						"writeData (CSV-literal or CSV-filename of 16-bit words, or keyword + parameter 'AUTOGENERATE count')", 
						"incrementAddress (Default = false)", "requestAck (Default = false)"},
					std::vector<std::string>{"Status"},
					1,   // requiredUserPermissions 
					"*",
					"This FE Macro enables users to write multiple words in the format of a comma separated values or "
					"a CSV file."
	);

	// Until further subsystem ROC development starts up, ignore the external block register access of core ROC firmware template established for the ROC dev cards.
	// registerFEMacroFunction(
	// 	"ROC_Write_ExtRegister",  // feMacroName
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&DTCFrontEndInterface::WriteExternalROCRegister),  // feMacroFunction
	// 				std::vector<std::string>{"rocLinkIndex", "block", "address", "writeData"},
	// 				std::vector<std::string>{},  // namesOfOutputArgs
	// 				1);                          // requiredUserPermissions

	// registerFEMacroFunction(
	// 	"ROC_Read_ExtRegister",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&DTCFrontEndInterface::ReadExternalROCRegister),
	// 				std::vector<std::string>{"rocLinkIndex", "block", "address"},
	// 				std::vector<std::string>{"readData"},
	// 				1);  // requiredUserPermissions			

							

	


	registerFEMacroFunction(
		"Buffer Test",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::BufferTest),                  // feMacroFunction
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
					std::vector<std::string>{"response"},
					1, // requiredUserPermissions
					"*",
					"Read a specified number of events from the Data DMA channel-0, and attempt to validate data."
					// "Send a request for a number of events and waits for the respective responses. "
					// "Currently, the responses are simulated data (a counter)."
	);


	registerFEMacroFunction(
		"Buffer Test Detached",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::BufferTest_detached),                  // feMacroFunction
					std::vector<std::string>{
						"Command to Start, Status (to read counters, etc.), or Halt (Default: Status)",
						"Data are SubEvents (Default: true)",
						// "Number of [Sub]Events (Default: 1)",  // will be continuous!
						"Starting Event Window Tag (Default: 0)",
						"Match Event Tags (Default: false)", 
						// "Display Payload at GUI (Default: true)", // will be summary output
					// "eventDuration (Default := 400)", 
						// "doNotReadBack (bool)", 
						"Save Binary Data to File (Default: false)"
						// "Software Generated Data Requests (bool)",
						// "Do Not Send Heartbeats (bool)"
						}, 
					std::vector<std::string>{"response"},
					1, // requiredUserPermissions
					"*",
					"Read a specified number of events from the Data DMA channel-0, and attempt to validate data."
					// "Send a request for a number of events and waits for the respective responses. "
					// "Currently, the responses are simulated data (a counter)."
	);

	registerFEMacroFunction(
		"Pattern Test",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::PatternTest),                  // feMacroFunction
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
					std::vector<std::string>{"response"},
					1, // requiredUserPermissions
					"*",
					"Read a specified number of events from the Data DMA channel-0, and attempt to validate data."
					// "Send a request for a number of events and waits for the respective responses. "
					// "Currently, the responses are simulated data (a counter)."
	);


	registerFEMacroFunction(
		"DTC Write",  // feMacroName
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::WriteDTC),  // feMacroFunction
					std::vector<std::string>{"address", "writeData"},
					std::vector<std::string>{"Status"},  // namesOfOutput
					1,                           // requiredUserPermissions
					"*",
					"This FE Macro writes to the DTC registers."
	);

	registerFEMacroFunction(
		"DTC Read",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::ReadDTC),                  // feMacroFunction
					std::vector<std::string>{"address"},  // namesOfInputArgs
					std::vector<std::string>{"readData"},
					1,   // requiredUserPermissions
					"*", 
					"Read from the DTC Memory Map."
	);

	// registerFEMacroFunction(
	// 	"DTC_HighRate_DCS_Check",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&DTCFrontEndInterface::DTCHighRateDCSCheck),
	// 				std::vector<std::string>{"rocLinkIndex","loops","baseAddress",
	// 					"correctRegisterValue0","correctRegisterValue1"},
	// 				std::vector<std::string>{},
	// 				1);  // requiredUserPermissions
					
	// registerFEMacroFunction(
	// 	"DTC_HighRate_DCS_Block_Check",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&DTCFrontEndInterface::DTCHighRateBlockCheck),
	// 				std::vector<std::string>{"rocLinkIndex","loops","baseAddress",
	// 					"correctRegisterValue0","correctRegisterValue1"},
	// 				std::vector<std::string>{},
	// 				1);  // requiredUserPermissions

	// registerFEMacroFunction(
	// 	"DTC_SendHeartbeatAndDataRequest",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&DTCFrontEndInterface::DTCSendHeartbeatAndDataRequest),
	// 				std::vector<std::string>{"numberOfRequests","timestampStart","useSWCFOEmulator","rocMask"},
	// 				std::vector<std::string>{"readData"},
	// 				1);  // requiredUserPermissions					
					
			
	registerFEMacroFunction(
		"Loss-of-Lock Counter Read",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::ReadLossOfLockCounter),
					std::vector<std::string>{},
					std::vector<std::string>{						
						"Upstream Rx Lock Loss Count"},
					1, 
					"*", 
					"Displays the number of times the CFO Control Link lost CDR lock since the last reset. "
					"Use the FE Macro <b>Reset Loss-of-Lock Counter</b> to reset the register counter to zero."
	);
	

	registerFEMacroFunction(
		"Loss-of-Lock Counter Reset",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::ResetLossOfLockCounter),
					std::vector<std::string>{},
					std::vector<std::string>{						
						"Upstream Rx Lock Loss Count"},
					1,   // requiredUserPermissions
					"*",
					"Use this FE Macro to reset the Loss-of-Lock register counter to zero. Reseting the DTC will also reset this register."
	);
	
	
	registerFEMacroFunction(
		"Get DTC Counters",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::DTCCounters),
					std::vector<std::string>{},
					std::vector<std::string>{						
						"Link Counters",
						"Performance Counters",
						"Packet Counters"},
					1,  // requiredUserPermissions
					"*",
					"Fetches data from all the counter registers in a human-readable format. "
					"Counters include the number of bytes and packets transmitted and recieved over ROC/CFO links since the last reset. "
					"Also includes Event Builder, Jitter Attenuator, Emulated ROC delay, Heartbeat packet and Data Header packet counters since last reset. "
	);

	registerFEMacroFunction(
		"Get Link Lock Status",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::GetLinkLockStatus),
				        std::vector<std::string>{},
						std::vector<std::string>{"Lock Status"},
					1,   // requiredUserPermissions
					"*", 
					"Read the SERDES CDR Lock bit on all links."
	);

	// registerFEMacroFunction(
	// 	"Select Jitter Attenuator Source",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&DTCFrontEndInterface::SelectJitterAttenuatorSource),
	// 			        std::vector<std::string>{"Source Clock (0 is from CFO, 1 is from RJ45)", 
	// 											"DoNotSet",
	// 											"AlsoResetJA"},
	// 					std::vector<std::string>{"Register Write Results"},
	// 				1,  // requiredUserPermissions 
	// 				"*", 
	// 				"Select the source of the jitter attenuator: a local oscilator on the DTC or the RTF.\n"
	// 				"The RTF (RJ45 Timing Fanout) is a separate board to alleviate jitter accumulation. <b>Not all DTCs are connected to the RTF</b>. "
	// );

	
	registerFEMacroFunction(
		"Configure DTC for HWDevmode",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::configureHardwareDevMode),
				        std::vector<std::string>{},
						std::vector<std::string>{"Setting the CFO emulated, DCS enabled and the retransmission off"},
					1,   // requiredUserPermissions
					"*", 
					"This FE Macro prepares the DTC for HW Dev Mode (emulated CFO)."
	);

	registerFEMacroFunction(
		"Read Rx Diag FIFO",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::readRxDiagFIFO),
				        std::vector<std::string>{"LinkIndex"},
						std::vector<std::string>{"Diagnostic RX FIFO"},
					1,    // requiredUserPermissions
					"*", 
					"This FE Macro reads the ROC link RX diagnostic FIFO buffer from the SERDES. "
					"When empty, the FIFO reports 0XDEADDEAD. <b>Note</b>: the FIFO must be read at least once before valid data appears. "
					"Reading the FIFO pulses the Read Enable input."
	);

	registerFEMacroFunction(
		"Read Tx Diag FIFO",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::readTxDiagFIFO),
				        std::vector<std::string>{"LinkIndex"},
						std::vector<std::string>{"Diagnostic TX FIFO"},
					1,   // requiredUserPermissions
					"*", 
					"This FE Macro reads the ROC link TX diagnostic FIFO buffer from the SERDES. "
					"When empty, the FIFO reports 0XDEADDEAD. <b>Note</b>: the FIFO must be read at least once before valid data appears. "
					"Reading the FIFO pulses the Read Enable input."
	);
					
	registerFEMacroFunction(
		"Get Link Errors",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::GetLinkErrors),
				        std::vector<std::string>{""},
						std::vector<std::string>{"Link Errors"},
					1,   // requiredUserPermissions
					"*", 
					"This FE Macro returns the number of errors on all links since last reset. "
					"Errors include the number of times illegal characters, a disparity, PRBS, and CRC error the SERDES has received. "
					"It also includes the number of EVB RX packet errors, and number of times the Jitter Attenuator lost the RX Recovered clock and "
					"lost the RX External clock since last reset."
	);

	// registerFEMacroFunction(
	// 	"DTC Instantiate",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&DTCFrontEndInterface::DTCInstantiate),
	// 				std::vector<std::string>{},
	// 				std::vector<std::string>{},
	// 				1,    // requiredUserPermissions	
	// 				"*", 
	// 				"This FE Macro reinstantiates the DTC interface class."
	// );
					
	registerFEMacroFunction(
		"Reset ALL (CFO/ROC/EVB) DTC Links",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::ResetDTCLinks),
					std::vector<std::string>{},
					std::vector<std::string>{},
					1,   // requiredUserPermissions 
					"*", 
					"This FE Macro resets the SERDES TX/RX links and then the SERDES."
	);

	// registerFEMacroFunction(
	// 	"Headers Format test",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 			&DTCFrontEndInterface::HeaderFormatTest),
	// 			std::vector<std::string>{},
	// 			std::vector<std::string>{"setRegister"},
	// 			1, 
	// 			"*", 
	// 			"Use this FE Macro to test the header format using emulated CFO Heartbeat packets."
	// );

	std::stringstream feMacroTooltip;
	feMacroTooltip << "There are " << CONFIG_DTC_TIMING_CHAIN_STEPS <<
		" steps. So choose 1 step at a time, 0-" << CONFIG_DTC_TIMING_CHAIN_STEPS-1 << 
		" or use -1 to run all steps sequentially." << __E__;
		
	registerFEMacroFunction(
		"Configure for Timing Chain",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::ConfigureForTimingChain),
					std::vector<std::string>{"StepIndex"},
					std::vector<std::string>{},
					1,"*" /*allowedCallingFEs*/,
					feMacroTooltip.str());  // requiredUserPermissions

	
	registerFEMacroFunction(
		"Loopback Manual Setup",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::ManualLoopbackSetup),
				    std::vector<std::string>{"setAsPassthrough", "ROC_Link"},
					std::vector<std::string>{},
					1,    // requiredUserPermissions
					"*", 
					"Sets the DTC in loopback mode. This is accomplished by disabling all links except for <b>ROC_Link</b>. "
					"If <b>setAsPassThrough</b> is enabled, the DTC Control Register bit 28 is set to low (default). "
					"This means the CFO Link SERDES output is routed back to the source. "
					"CFO packets will be transmitted instead to the next DTC (Normal operation). "
	);
	
	registerFEMacroFunction(
		"Check Firefly Loss-of-Light",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::GetLinkLossOfLight),            // feMacroFunction
					std::vector<std::string>{},  // namesOfInputArgs
					std::vector<std::string>{"Link Status"},
					1,  // requiredUserPermissions
					"*",
					"Checking the Loss-of-Light reads from the I2C bus register on the Firefly for any light source."
	);

	// registerFEMacroFunction(
	// 	"Reset PCIe",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&DTCFrontEndInterface::ResetPCIe),            // feMacroFunction
	// 				std::vector<std::string>{},  // namesOfInputArgs
	// 				std::vector<std::string>{"Status"},
	// 				1,  // requiredUserPermissions 
	// 				"*",
	// 				"Reset the PCIe interface."
	// );

	// registerFEMacroFunction(
	// 	"Check Firefly Temperature",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&DTCFrontEndInterface::GetFireflyTemperature),            // feMacroFunction
	// 				std::vector<std::string>{},  // namesOfInputArgs
	// 				std::vector<std::string>{"Temperature"},
	// 				1,  // requiredUserPermissions
	// 				"*",
	// 				"Check the temperature from the Firefly RX IIC Bus."
	// );

	// registerFEMacroFunction(
	// 	"Check FPGA Temperature",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&DTCFrontEndInterface::GetFPGATemperature),            // feMacroFunction
	// 				std::vector<std::string>{},  // namesOfInputArgs
	// 				std::vector<std::string>{"Temperature"},
	// 				1, 
	// 				"*", 
	// 				"Get the temperature of the FPGA in degrees Celesius. "
	// 				"A -273.0 reading is equivalent to 0x0 from the ADC on-die sensor."
	// );

	registerFEMacroFunction(
		"Reset CFO Link Rx",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::ResetCFOLinkRx),            // feMacroFunction
					std::vector<std::string>{},  // namesOfInputArgs
					std::vector<std::string>{},
					1,  // requiredUserPermissions
					"*",
					"Reset the SERDES RX interface."
	);
	registerFEMacroFunction(
		"Reset CFO Link Tx",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::ResetCFOLinkTx),            // feMacroFunction
					std::vector<std::string>{},  // namesOfInputArgs
					std::vector<std::string>{},
					1,  // requiredUserPermissions
					"*",
					"Reset the CFO SERDES RX interface."
	);
	registerFEMacroFunction(
		"Reset CFO Link Rx PLL",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::ResetCFOLinkRxPLL),            // feMacroFunction
					std::vector<std::string>{},  // namesOfInputArgs
					std::vector<std::string>{},
					1,  // requiredUserPermissions
					"*", 
					"Reset the CFO SERDES RX PLL."
	);
	registerFEMacroFunction(
		"Reset CFO Link Tx PLL",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::ResetCFOLinkTxPLL),            // feMacroFunction
					std::vector<std::string>{},  // namesOfInputArgs
					std::vector<std::string>{},
					1,  // requiredUserPermissions
					"*", 
					"Reset the CFO SERDES TX PLL."
	);

	registerFEMacroFunction(
		"CFO Interface (Emulation) Setup",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::SetupCFOInterface),            // feMacroFunction
					std::vector<std::string>{
						"Put DTC in CFO Emulation Mode (Default := false)",
						"Set Link RX/TX Enable (Default := false)",
						"Enable Auto-generation of Data Request Packets (Default := false)",
					},  // namesOfInputArgs
					std::vector<std::string>{},
					1,  // requiredUserPermissions
					"*", 
					"Select or Deselect the CFO Emulator to take priority over the Link-6 external CFO. When selected, the CFO timing link, Link-6, will be ignored."
	);
	
	registerFEMacroFunction(
		"CFO Emulator On/Off Spill Emulation Setup",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::SetCFOEmulatorOnOffSpillEmulation),                  // feMacroFunction
					std::vector<std::string>{"Enable CFO Emulator (Default := false)",
											"Number of 1.4s super cycle repetitions (0 := infinite)",
											"Starting Event Window Tag",
											"Enable Auto-generation of Data Request Packets (Default := false)",
											"Enable Clock Markers (Default := false)",
											"Use Detached Buffer Test (Default := false)",
											"For Detached Buffer Test, Save Binary Data to File (Default: false)",
											"For Detached Buffer Test, Do NOT Reset Counters (Default: false)"
											},  // namesOfInputArgs
					std::vector<std::string>{"response"},
					1,   // requiredUserPermissions					
					"*",
					"Enable/Disable the CFO Emulator. Disabling turns off output of emulated Event Window Markers, timing markers, and Heartbeat Packets. " /* feMacroTooltip */
					"Enabling turns on emulated Event Window generation and timing markers based on the CFO emulator parameters."
	);
	registerFEMacroFunction(
		"CFO Emulator Fixed-width Event Window Emulation Setup",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&DTCFrontEndInterface::SetCFOEmulatorFixedWidthEmulation),                  // feMacroFunction
					std::vector<std::string>{"Enable CFO Emulator (Default := false)",
											"Fixed-width Event Window Duration (s, ms, us, ns, and clocks allowed) [clocks := 25ns]",
											"Number of Event Window Markers to generate (0 := infinite)",
											"Starting Event Window Tag",
											"Event Window Mode (Default := 1)",
											"Enable Auto-generation of Data Request Packets (Default := false)",
											"Enable Clock Markers (Default := false)",
											"Use Detached Buffer Test (Default := false)",
											"For Detached Buffer Test, Save Binary Data to File (Default: false)",
											"For Detached Buffer Test, Do NOT Reset Counters (Default: false)"
											},  // namesOfInputArgs
					std::vector<std::string>{"response"},
					1,   // requiredUserPermissions					
					"*",
					"Enable/Disable the CFO Emulator. Disabling turns off output of emulated Event Window Markers, timing markers, and Heartbeat Packets. " /* feMacroTooltip */
					"Enabling turns on emulated Event Window generation and timing markers based on the CFO emulator parameters."
	);



	{ //add ROC FE Macros
		__FE_COUT__ << "Getting children ROC FEMacros..." << __E__;
		rocFEMacroMap_.clear();
		for(auto& roc : rocs_)
		{
			auto feMacros = roc.second->getMapOfFEMacroFunctions();
			for(auto& feMacro:feMacros)
			{
				__FE_COUT__ << roc.first << "::" << feMacro.first << __E__;

				//make DTC FEMacro forwarding to ROC FEMacro				
                std::string macroName =
                                "Link" +
                                std::to_string(roc.second->getLinkID()) +
                                "_" + roc.first + "_" +
                                feMacro.first;
                __FE_COUTV__(macroName);
                std::vector<std::string> inputArgs,outputArgs;
				//TODO, take ROC target as parameter for ROC FE Macros				
                //inputParams.push_back("ROC_UID");
                //inputParams.push_back("ROC_FEMacroName");
				for(auto& inArg: feMacro.second.namesOfInputArguments_)
					inputArgs.push_back(inArg);
				for(auto& outArg: feMacro.second.namesOfOutputArguments_)
					outputArgs.push_back(outArg);

				__FE_COUTV__(StringMacros::vectorToString(inputArgs));
				__FE_COUTV__(StringMacros::vectorToString(outputArgs));

				rocFEMacroMap_.emplace(std::make_pair(macroName,
						std::make_pair(roc.first,feMacro.first)));

				registerFEMacroFunction(macroName,
						static_cast<FEVInterface::frontEndMacroFunction_t>(
								&DTCFrontEndInterface::RunROCFEMacro),
								inputArgs,
								outputArgs,
								1);  // requiredUserPermissions
			}
		}
	} //end add ROC FE Macros

	// clang-format on

	CFOandDTCCoreVInterface::registerCFOandDTCFEMacros();

}  // end registerFEMacros()

//==============================================================================
void DTCFrontEndInterface::configureSlowControls(void)
{
	__FE_COUT__ << "Configuring slow controls..." << __E__;

	// parent configure adds DTC slow controls channels
	FEVInterface::configureSlowControls();  // also resets mapOfSlowControlsChannels_

	__FE_COUT__ << "DTC '" << getInterfaceUID()
	            << "' slow controls channel count (BEFORE considering ROCs): "
	            << getSlowControlsChannelCount() << __E__;

	for(auto& roc:rocs_)
	{
		__FE_COUT__ << "Configuring DTC '" << getInterfaceUID()
					<< "' ROC '" << roc.second->getInterfaceUID()
					<< "' slow controls..." << __E__;
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
	for(auto& roc:rocs_)
		roc.second->resetSlowControlsChannelIterator();
}  // end resetSlowControlsChannelIterator()

//==============================================================================
// virtual in case channels are handled in multiple maps, for example
FESlowControlsChannel* DTCFrontEndInterface::getNextSlowControlsChannel(void)
{
	// if not finished with DTC slow controls channels, return next DTC channel
	FESlowControlsChannel* nextSlowControlsChannel = FEVInterface::getNextSlowControlsChannel();
	if(nextSlowControlsChannel != nullptr) return nextSlowControlsChannel;

	// else if finished with DTC slow controls channels, move on to ROC list
	
	for(auto& roc:rocs_)
	{
		nextSlowControlsChannel = roc.second->getNextSlowControlsChannel();
		if(nextSlowControlsChannel != nullptr) return nextSlowControlsChannel;		
	}

	// else no more channels
	__FE_COUT__ << "Done with DTC+ROC slow control channel handling." << __E__;
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

	std::vector<std::pair<std::string, ConfigurationTree>> rocChildren =
	    Configurable::getSelfNode().getNode("LinkToROCGroupTable").getChildren();

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
				__COUTV__(theXDAQContextConfigTree_.getValueAsString());
				__COUTV__(
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

				// setup parent supervisor of FEVinterface (for backwards compatibility,
				// left out of constructor)
				tmpVFE->parentSupervisor_ = parentSupervisor_;

				ROCCoreVInterface& tmpRoc = dynamic_cast<ROCCoreVInterface&>(
				    *tmpVFE);  // dynamic_cast<ROCCoreVInterface*>(tmpRoc.get());

				// setup other members of ROCCore (for interface plug-in compatibility,
				// left out of constructor)

				__COUTV__(tmpRoc.emulatorMode_);
				tmpRoc.emulatorMode_ = emulatorMode_;
				__COUTV__(tmpRoc.emulatorMode_);

				if(emulatorMode_)
				{
					__FE_COUT__ << "Creating ROC in emulator mode..." << __E__;

					// try
					{
						// all ROCs support emulator mode

						//						// verify ROCCoreVEmulator class
						// functionality  with  dynamic_cast
						// ROCCoreVEmulator&  tmpEmulator =
						// dynamic_cast<ROCCoreVEmulator&>(
						//						    tmpRoc);  //
						// dynamic_cast<ROCCoreVInterface*>(tmpRoc.get());

						// start emulator thread
						std::thread(
						    [](ROCCoreVInterface* rocEmulator) {
							    __COUT__ << "Starting ROC emulator thread..." << __E__;
							    ROCCoreVInterface::emulatorThread(rocEmulator);
						    },
						    &tmpRoc)
						    .detach();
					}
					//					catch(const std::bad_cast& e)
					//					{
					//						__SS__ << "Cast to ROCCoreVEmulator failed!
					// Verify  the  emulator " 						          "plugin
					// inherits
					// from  ROCCoreVEmulator."
					//						       << __E__;
					//						ss << "Failed to instantiate plugin named '"
					//<<  roc.first
					//						   << "' of type '"
					//						   <<
					// roc.second.getNode("ROCInterfacePluginName")
					//						          .getValue<std::string>()
					//						   << "' due to the following error: \n"
					//						   << e.what() << __E__;
					//
					//						__SS_THROW__;
					//					}
				}
				else
				{
					tmpRoc.thisDTC_ = thisDTC_;
				}

				rocs_.emplace(std::pair<std::string, std::unique_ptr<ROCCoreVInterface>>(
				    roc.first, &tmpRoc));
				tmpVFE.release();  // release the FEVInterface unique_ptr, so we are left
				                   // with just one

				__COUTV__(rocs_[roc.first]->emulatorMode_);
			}
			catch(const cet::exception& e)
			{
				__SS__ << "Failed to instantiate plugin named '" << roc.first
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

	if(skipInit_) return;

	if(operatingMode_ == "HardwareDevMode")
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
	// 	__FE_COUT_INFO__ << "Configuring for Loopback mode!" << __E__;
	// 	configureLoopbackMode();
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

	// // if(regWriteMonitorStream_.is_open())
	// // {
	// // 	regWriteMonitorStream_ << "Timestamp: " << std::dec << time(0)
	// // 	                       << ", \t ---------- Configure step " << getIterationIndex()
	// // 	                       << ":" << getSubIterationIndex() << "\n";
	// // 	regWriteMonitorStream_.flush();
	// // }

	// if(getConfigurationManager()
	//        ->getNode("/Mu2eGlobalsTable/SyncDemoConfig/SkipCFOandDTCConfigureSteps")
	//        .getValue<bool>())
	// {
	// 	__FE_COUT_INFO__ << "Skipping configure steps!" << __E__;
	// 	return;
	// }

	// if(emulatorMode_)
	// {
	// 	__FE_COUT__ << "Emulator DTC configuring... # of ROCs = " << rocs_.size()
	// 	            << __E__;
	// 	for(auto& roc : rocs_)
	// 		roc.second->configure();
	// 	return;
	// }

	// uint32_t dtcEventBuilderReg_DTCID = 0;
	// uint32_t dtcEventBuilderReg_Mode = 0;
	// uint32_t dtcEventBuilderReg_PartitionID = 0;
	// uint32_t dtcEventBuilderReg_MACIndex = 0;
	// // uint32_t dtcEventBuilderReg_DTCInfo = 0;

	// uint32_t dtcEventBuilderReg_NumBuff       = 0;
	// uint32_t dtcEventBuilderReg_StartNode     = 0;
	// uint32_t dtcEventBuilderReg_NumNodes      = 0;
	// // uint32_t dtcEventBuilderReg_Configuration = 0;

	// try
	// {
	// 	dtcEventBuilderReg_DTCID =
	// 	    getSelfNode().getNode("EventBuilderDTCID").getValue<uint32_t>();
	// 	dtcEventBuilderReg_Mode =
	// 	    getSelfNode().getNode("EventBuilderMode").getValue<uint32_t>();
	// 	dtcEventBuilderReg_PartitionID =
	// 	    getSelfNode().getNode("EventBuilderPartitionID").getValue<uint32_t>();
	// 	dtcEventBuilderReg_MACIndex =
	// 	    getSelfNode().getNode("EventBuilderMACIndex").getValue<uint32_t>();

	// 	dtcEventBuilderReg_NumBuff =
	// 	    getSelfNode().getNode("EventBuilderNumBuff").getValue<uint32_t>();
	// 	dtcEventBuilderReg_StartNode =
	// 	    getSelfNode().getNode("EventBuilderStartNode").getValue<uint32_t>();
	// 	dtcEventBuilderReg_NumNodes =
	// 	    getSelfNode().getNode("EventBuilderNumNodes").getValue<uint32_t>();

	// 	__FE_COUTV__(dtcEventBuilderReg_DTCID);
	// 	__FE_COUTV__(dtcEventBuilderReg_Mode);
	// 	__FE_COUTV__(dtcEventBuilderReg_PartitionID);
	// 	__FE_COUTV__(dtcEventBuilderReg_MACIndex);
	// 	__FE_COUTV__(dtcEventBuilderReg_NumBuff);
	// 	__FE_COUTV__(dtcEventBuilderReg_StartNode);
	// 	__FE_COUTV__(dtcEventBuilderReg_NumNodes);

	// 	// Register x9154 is #DTC ID [31-24] / EVB Mode [23-16]/ EVB Partition ID [15-8]/
	// 	// EVB Local MAC Index [7-0]
	// 	thisDTC_->SetEVBInfo(dtcEventBuilderReg_DTCID,
	// 		dtcEventBuilderReg_Mode,
	// 		dtcEventBuilderReg_PartitionID,
	// 		dtcEventBuilderReg_MACIndex);
	// 	// dtcEventBuilderReg_DTCInfo =
	// 	//     dtcEventBuilderReg_DTCID << 24 | dtcEventBuilderReg_Mode << 16 |
	// 	//     dtcEventBuilderReg_PartitionID << 8 | dtcEventBuilderReg_MACIndex;
	// 	// __FE_COUTV__(dtcEventBuilderReg_DTCInfo);
	// 	// registerWrite(0x9154, dtcEventBuilderReg_DTCInfo);

	// 	// Register x9158 is #Num EVB Buffers[22-16], EVB Start Node [14-8], Num Nodes
	// 	// [6-0]
	// 	thisDTC_->SetEVBBufferInfo(dtcEventBuilderReg_NumBuff,
	// 		dtcEventBuilderReg_StartNode,
	// 		dtcEventBuilderReg_NumNodes);
	// 	// dtcEventBuilderReg_Configuration = dtcEventBuilderReg_NumBuff << 16 |
	// 	//                                    dtcEventBuilderReg_StartNode << 8 |
	// 	//                                    dtcEventBuilderReg_NumNodes;
	// 	// __FE_COUTV__(dtcEventBuilderReg_Configuration);
	// 	// registerWrite(0x9158, dtcEventBuilderReg_Configuration);
	// }
	// catch(...)
	// {
	// 	__FE_COUT_INFO__ << "Ignoring missing event building configuration values."
	// 	                 << __E__;
	// }
	// // end of the new code

	// __FE_COUT__ << "DTC configuring... # of ROCs = " << rocs_.size() << __E__;

	// // NOTE: otsdaq/xdaq has a soap reply timeout for state transitions.
	// // Therefore, break up configuration into several steps so as to reply before
	// // the time out As well, there is a specific order in which to configure the
	// // links in the chain of CFO->DTC0->DTC1->...DTCN

	// const int number_of_system_configs =
	//     -1;                    // if < 0, keep trying until links are OK.
	//                            // If > 0, go through configuration steps this many times

	// const int reset_fpga = 1;  // 1 = yes, 0 = no
	// // const bool config_clock             = configure_clock_;  // 1 = yes, 0 = no
	// const bool config_jitter_attenuator = configure_clock_;  // 1 = yes, 0 = no
	// // const int  reset_rx                 = 0;                 // 1 = yes, 0 = no

	// const int number_of_dtc_config_steps = 7;

	// const int max_number_of_tries = 3;  // max number to wait for links OK

	// int number_of_total_config_steps =
	//     number_of_system_configs * number_of_dtc_config_steps;

	// int config_step    = getIterationIndex();
	// int config_substep = getSubIterationIndex();

	// bool isLastTimeThroughConfigure = false;

	// if(number_of_system_configs > 0)
	// {
	// 	if(config_step >= number_of_total_config_steps)  // done, exit system config
	// 		return;
	// }

	// // waiting for link loop
	// if(config_substep > 0 && config_substep < max_number_of_tries)
	// {  // wait a maximum of 30 seconds

	// 	const int number_of_link_checks = 10;

	// 	// int link_ok = 0;

	// 	for(int i = 0; i < number_of_link_checks; i++)
	// 	{
	// 		if(checkLinkStatus() == 1)
	// 		{
	// 			// links OK,  continue with the rest of the configuration
	// 			__FE_COUT__ << device_name_ << " Link Status is OK = 0x" << std::hex
	// 			            << registerRead(0x9140) << std::dec << __E__;

	// 			indicateIterationWork();
	// 			turnOffLED();
	// 			return;
	// 		}
	// 		else if(getCFOLinkStatus() == 0)
	// 		{
	// 			// in this case, links will never get to be OK, stop waiting for them

	// 			__FE_COUT__ << device_name_ << " CFO Link Status is bad = 0x" << std::hex
	// 			            << registerRead(0x9140) << std::dec << __E__;

	// 			// usleep(500000 /*500ms*/);
	// 			sleep(1);

	// 			indicateIterationWork();
	// 			turnOffLED();
	// 			return;
	// 		}
	// 		else
	// 		{
	// 			// links still not OK, keep checking...

	// 			__FE_COUT__ << "Waiting for DTC Link Status = 0x" << std::hex
	// 			            << registerRead(0x9140) << std::dec << __E__;
	// 			// usleep(500000 /*500ms*/);
	// 			sleep(1);
	// 		}
	// 	}

	// 	indicateSubIterationWork();
	// 	return;
	// }
	// else if(config_substep > max_number_of_tries)
	// {
	// 	// wait a maximum of 30 seconds, then stop waiting for them

	// 	__FE_COUT__ << "Links still bad = 0x" << std::hex << registerRead(0x9140)
	// 	            << std::dec << "... continue" << __E__;
	// 	indicateIterationWork();
	// 	turnOffLED();
	// 	return;
	// }

	// turnOnLED();


	// if((config_step % number_of_dtc_config_steps) == 0)
	// {
	// 	__FE_COUTV__(GetFirmwareVersion());
	// 	if(reset_fpga == 1 && config_step < number_of_dtc_config_steps)
	// 	{
	// 		// only reset the FPGA the first time through
	// 		__FE_COUT_INFO__ << "Step " << config_step << ": " << device_name_
	// 		                 << " Reset DTC " << __E__;

	// 		DTCSoftReset();
	// 	}

	// 	// From Rick new code
	// 	uint32_t dtcEventBuilderReg_DTCID       = 0;
	// 	uint32_t dtcEventBuilderReg_Mode        = 0;
	// 	uint32_t dtcEventBuilderReg_PartitionID = 0;
	// 	uint32_t dtcEventBuilderReg_MACIndex    = 0;
	// 	uint32_t dtcEventBuilderReg_DTCInfo     = 0;

	// 	uint32_t dtcEventBuilderReg_NumBuff       = 0;
	// 	uint32_t dtcEventBuilderReg_StartNode     = 0;
	// 	uint32_t dtcEventBuilderReg_NumNodes      = 0;
	// 	uint32_t dtcEventBuilderReg_Configuration = 0;

	// 	try
	// 	{
	// 		__FE_COUT__ << "Configuring DTC registers for the EVB" << rocs_.size()
	// 		            << __E__;

	// 		dtcEventBuilderReg_DTCID =
	// 		    getSelfNode().getNode("EventBuilderDTCID").getValue<uint32_t>();
	// 		dtcEventBuilderReg_Mode =
	// 		    getSelfNode().getNode("EventBuilderMode").getValue<uint32_t>();
	// 		dtcEventBuilderReg_PartitionID =
	// 		    getSelfNode().getNode("EventBuilderPartitionID").getValue<uint32_t>();
	// 		dtcEventBuilderReg_MACIndex =
	// 		    getSelfNode().getNode("EventBuilderMACIndex").getValue<uint32_t>();

	// 		dtcEventBuilderReg_NumBuff =
	// 		    getSelfNode().getNode("EventBuilderNumBuff").getValue<uint32_t>();
	// 		dtcEventBuilderReg_StartNode =
	// 		    getSelfNode().getNode("EventBuilderStartNode").getValue<uint32_t>();
	// 		dtcEventBuilderReg_NumNodes =
	// 		    getSelfNode().getNode("EventBuilderNumNodes").getValue<uint32_t>();

	// 		__FE_COUTV__(dtcEventBuilderReg_DTCID);  // Doesn't work if I use uint8_t
	// 		__FE_COUTV__(dtcEventBuilderReg_Mode);
	// 		__FE_COUTV__(dtcEventBuilderReg_PartitionID);
	// 		__FE_COUTV__(dtcEventBuilderReg_MACIndex);
	// 		__FE_COUTV__(dtcEventBuilderReg_NumBuff);
	// 		__FE_COUTV__(dtcEventBuilderReg_StartNode);
	// 		__FE_COUTV__(dtcEventBuilderReg_NumNodes);

	// 		// Register x9154 is #DTC ID [31-24] / EVB Mode [23-16]/ EVB Partition ID
	// 		// [15-8]/ EVB Local MAC Index [7-0]
	// 		dtcEventBuilderReg_DTCInfo =
	// 		    dtcEventBuilderReg_DTCID << 24 | dtcEventBuilderReg_Mode << 16 |
	// 		    dtcEventBuilderReg_PartitionID << 8 | dtcEventBuilderReg_MACIndex;
	// 		__FE_COUTV__(dtcEventBuilderReg_DTCInfo);
	// 		registerWrite(0x9154, dtcEventBuilderReg_DTCInfo);

	// 		// Register x9158 is #Num EVB Buffers[22-16], EVB Start Node [14-8], Num Nodes
	// 		// [6-0]
	// 		dtcEventBuilderReg_Configuration = dtcEventBuilderReg_NumBuff << 16 |
	// 		                                   dtcEventBuilderReg_StartNode << 8 |
	// 		                                   dtcEventBuilderReg_NumNodes;
	// 		__FE_COUTV__(dtcEventBuilderReg_Configuration);
	// 		registerWrite(0x9158, dtcEventBuilderReg_Configuration);
	// 	}
	// 	catch(...)
	// 	{
	// 		__FE_COUT_INFO__ << "Ignoring missing event building configuration values."
	// 		                 << __E__;
	// 	}
	// 	// end of the new code
	// }
	// else if((config_step % number_of_dtc_config_steps) == 1)
	// {
	// 	__FE_COUT_INFO__ << "Step" << config_step << ": " << device_name_
	// 	                 << "select/setup clock..." << __E__;

	// 	// choose jitter attenuator input select (reg 0x9308, bits 5:4)
	// 	//  0 is Upstream Control Link Rx Recovered Clock
	// 	//  1 is RJ45 Upstream Clock
	// 	//  2 is Timing Card Selectable (SFP+ or FPGA) Input Clock
	// 	{
	// 		uint32_t readData = registerRead(0x9308);
	// 		uint32_t val      = 0;
	// 		try
	// 		{
	// 			val = getSelfNode()
	// 			          .getNode("JitterAttenuatorInputSource")
	// 			          .getValue<uint32_t>();
	// 		}
	// 		catch(...)
	// 		{
	// 			__FE_COUT__ << "Defaulting Jitter Attenuator Input Source to val = "
	// 			            << val << __E__;
	// 		}
	// 		readData &= ~(3 << 4);       // clear the two bits
	// 		readData &= ~(1);            // ensure unreset of jitter attenuator
	// 		readData |= (val & 3) << 4;  // set the two bits to selected value

	// 		registerWrite(0x9308, readData);
	// 		__FE_COUT__
	// 		    << "Jitter Attenuator Input Select: " << val << " ==> "
	// 		    << (val == 0
	// 		            ? "Upstream Control Link Rx Recovered Clock"
	// 		            : (val == 1
	// 		                   ? "RJ45 Upstream Clock"
	// 		                   : "Timing Card Selectable (SFP+ or FPGA) Input Clock"))
	// 		    << __E__;
	// 	}
	// }
	// else if((config_step % number_of_dtc_config_steps) == 2)
	// {
	// 	// configure Jitter Attenuator to recover clock
	// 	if((config_jitter_attenuator == 1 || emulate_cfo_ == 1) &&
	// 	   config_step < number_of_dtc_config_steps)
	// 	{
	// 		__MCOUT_INFO__("Step " << config_step << ": " << device_name_
	// 		                       << " configure Jitter Attenuator..." << __E__);

	// 		// It's needed only after a powercycle
	// 		configureJitterAttenuator();

	// 		usleep(500000 /*500ms*/);
	// 		sleep(1);
	// 	}
	// 	else
	// 	{
	// 		__MCOUT_INFO__("Step " << config_step << ": " << device_name_
	// 		                       << " do NOT configure Jitter Attenuator..." << __E__);
	// 	}
	// }
	// else if((config_step % number_of_dtc_config_steps) == 3)
	// {
	// 	if(emulate_cfo_ == 1)
	// 	{
	// 		__MCOUT_INFO__("Step " << config_step << ": " << device_name_
	// 		                       << " enable CFO emulation and internal clock");

	// 		int dataToWrite = 0x40808404;
	// 		registerWrite(0x9100, dataToWrite);  // This disable retransmission + set the
	// 		                                     // CFO in emulation mode

	// 		// Micol thinks this two lines below are not needed
	// 		//__FE_COUT__ << ".......CFO emulation: time between data requests" << __E__;
	// 		// registerWrite(0x91a8, 0x4e20);
	// 	}
	// 	else
	// 	{
	// 		int dataInReg = registerRead(0x9100);
	// 		int dataToWrite =
	// 		    dataInReg &
	// 		    0xbfff7fff;  // bit 30 = CFO emulation enable; bit 15 CFO emulation mode
	// 		registerWrite(0x9100, dataToWrite);
	// 	}
	// }
	// else if((config_step % number_of_dtc_config_steps) == 4) {}
	// else if((config_step % number_of_dtc_config_steps) == 5)
	// {
	// 	__MCOUT_INFO__("Step " << config_step << ": " << device_name_
	// 	                       << " enable markers, Tx, Rx" << __E__);

	// 	// enable markers, tx and rx

	// 	int data_to_write = (roc_mask_ << 8) | roc_mask_;
	// 	__FE_COUT__ << "CFO enable markers - enabled ROC links 0x" << std::hex
	// 	            << data_to_write << std::dec << __E__;
	// 	registerWrite(0x91f8, data_to_write);

	// 	data_to_write = 0x4040 | (roc_mask_ << 8) | roc_mask_;
	// 	__FE_COUT__ << "DTC enable tx and rx - CFO and enabled ROC links 0x" << std::hex
	// 	            << data_to_write << std::dec << __E__;
	// 	registerWrite(0x9114, data_to_write);

	// 	data_to_write = 0x00014141;  // DMA timeout from chants.
	// 	__FE_COUT__ << "set DMA timeout" << std::hex << data_to_write << std::dec
	// 	            << __E__;
	// 	registerWrite(0x9144, data_to_write);

	// 	// put DTC CFO link output into loopback mode
	// 	__FE_COUT__ << "DTC set CFO link output loopback mode ENABLE" << __E__;

	// 	__MCOUT_INFO__("Step " << config_step << ": " << device_name_ << " configure ROCs"
	// 	                       << __E__);

	// 	bool doConfigureROCs = false;
	// 	try
	// 	{
	// 		doConfigureROCs = Configurable::getSelfNode()
	// 		                      .getNode("EnableROCConfigureStep")
	// 		                      .getValue<bool>();
	// 	}
	// 	catch(...)
	// 	{
	// 	}  // ignore missing field
	// 	if(doConfigureROCs)
	// 		for(auto& roc : rocs_)
	// 			roc.second->configure();

	// 	// usleep(500000 /*500ms*/);
	// 	sleep(1);
	// }
	// else if((config_step % number_of_dtc_config_steps) == 6)
	// {
	// 	if(emulate_cfo_ == 1)
	// 	{
	// 		__MCOUT_INFO__("Step " << config_step
	// 		                       << ": CFO emulation enable Event start characters "
	// 		                          "and event window interval"
	// 		                       << __E__);

	// 		__FE_COUT__ << "CFO emulation:  set Event Window interval" << __E__;
	// 		registerWrite(0x91f0, 0x00000000);  // for NO markers, write these

	// 		__FE_COUT__ << "CFO emulation:  set 40MHz marker interval" << __E__;
	// 		registerWrite(0x91f4, 0x00000000);  // for NO markers, write these

	// 		__FE_COUT__ << "CFO emulation:  set heartbeat interval " << __E__;
	// 	}
	// 	__MCOUT_INFO__("Step " << config_step << ": " << device_name_ << " configured"
	// 	                       << __E__);

	// 	__FE_COUTV__(getIterationIndex());
	// 	__FE_COUTV__(getSubIterationIndex());

	// 	if(checkLinkStatus() == 1)
	// 	{
	// 		__MCOUT_INFO__(device_name_ << " links OK 0x" << std::hex
	// 		                            << registerRead(0x9140) << std::dec << __E__);

	// 		// usleep(500000 ); //500ms/
	// 		sleep(1);
	// 		turnOffLED();

	// 		if(number_of_system_configs < 0)
	// 		{
	// 			isLastTimeThroughConfigure = true;
	// 			// do a final DTC Reset
	// 			//__MCOUT_INFO__("Last step in configuration; doing DTCSoftReset" << __E__);
	// 			// DTCSoftReset();
	// 		}
	// 	}
	// 	else if(config_step > max_number_of_tries)
	// 	{
	// 		isLastTimeThroughConfigure = true;
	// 		__MCOUT_INFO__(device_name_ << " after " << max_number_of_tries
	// 		                            << " tries, links not OK 0x" << std::hex
	// 		                            << registerRead(0x9140) << std::dec << __E__);
	// 	}
	// 	else
	// 	{
	// 		__MCOUT_INFO__(device_name_ << " links not OK 0x" << std::hex
	// 		                            << registerRead(0x9140) << std::dec << __E__);
	// 	}

	// 	__FE_COUTV__(isLastTimeThroughConfigure);
	// 	if(isLastTimeThroughConfigure)
	// 	{
	// 		sleep(2);
	// 		// write anything to reset
	// 		// 0x93c8 is RX CDR Unlock counter (32-bit)
	// 		__MCOUT_INFO__("LAST STEP!! Reset Loss-of-Lock Counter() on DTC");

	// 		registerWrite(0x93c8, 0);

	// 		// Registers to set the EVB
	// 		try
	// 		{
	// 			__FE_COUT__ << "Configuring DTC registers for the EVB" << rocs_.size()
	// 			            << __E__;
	// 			// These registers are needed for the EVB, but I need to check their
	// 			// meaning
	// 			registerWrite(0x9100, 0x800404);
	// 			registerWrite(0x92c0, 0x0);
	// 			registerWrite(0x9114, 0xc1c1);
	// 			registerWrite(0x96C8, 0x555555D5);
	// 			registerWrite(0x96CC, 0x78555555);
	// 		}
	// 		catch(...)
	// 		{
	// 			__FE_COUT_INFO__
	// 			    << "Ignoring missing event building DTC registers values." << __E__;
	// 		}
	// 		// end of the new code

	// 		return;  // links OK, kick out of configure OR link tries complete
	// 	}
	// }

	// readStatus();             // spit out link status at every step

	// indicateIterationWork();  // otherwise, tell state machine to stay in configure
	//                           // state ("come back to me")

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
	try	{ throw; } //one more try to printout extra info
	catch(const std::exception &e)
	{
		ss << "Exception message: " << e.what();
	}
	catch(...){}
	__FE_SS_THROW__;
}

//==============================================================================
void DTCFrontEndInterface::configureHardwareDevMode(void)
{
	__FE_COUT_INFO__ << "configureHardwareDevMode()" << __E__;

	// 0x9100 is the DTC Control Register
	// registerWrite(0x9100, 0x40008404);  // bit 30 = CFO emulation enable, bit 15 = CFO emulation mode
	                  // (set both to 1 to be use the CFO emulated)
	                  // bit 2 = DCS enable
	                  // bit 10 = Sequence Number Disable
	                  // (set to 1 to turns off retrasmission DTC-ROC which isn't working
	                  // right now)

	//put DTC in known state with DTC reset and control clear
	thisDTC_->SoftReset(); 
	thisDTC_->ClearControlRegister(); 
	
	//During debug session on 14-Nov-2023, realized JA config breaks ROC link CDR lock
	//	So solution:
	//		- only configure JA one time ever after cold start
	//		- from then on, do not touch JA
	if(0 && configure_clock_)
	{
		uint32_t select      = 0;
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
		thisDTC_->SetJitterAttenuatorSelect(select, true /* alsoResetJA */); // this call should first check if JA is already locked, JA only needs to be set after a cold start or if input clock changes
	}
	else
		__FE_COUT_INFO__ << "Skipping configure clock." << __E__;



	// thisDTC_->EnableCFOEmulation(); // this will enable sending requests, so do it later after configuring event 
	thisDTC_->SetCFOEmulationMode(); //turn on DTC emulation (ignores any real CFO) 
	thisDTC_->SetSequenceNumberDisable();
	thisDTC_->EnableDCSReception();


	// CFO Event Mode bytes (for the CFO Emulator): 0x91c0 and 0x91c4
	// Data in these registers populates the Event Mode field in the Heartbeat packets the
	// CFO Emulator will send. They are used to identify a Null vs. Non-null heartbeat: If
	// the Event Mode bits are all zero, the DTC interprets it a null heartbeats. The DTC
	// only generates Data Request Packets for non-null heartbeats.  So when these
	// registers are set to zeros, the DTC generates only null heartbeats, and no Data
	// Requests. For testing purposes, it's needed to set the Event Mode Bytes to some
	// non-zero value.
	thisDTC_->SetCFOEmulationEventMode(-1);
	__FE_COUTV__(thisDTC_->ReadCFOEmulationEventMode());

	// The DTC's CFO Emulator will generate a number of null packets after the number
	// of requested non-null heartbeats has been output.
	// The number of requests (non-nulls) is set in 0x91AC, and the number of nulls is set
	// in 0x91BC.
	// registerWrite(0x91BC, 0x10);  // Set number of null heartbeats packets
	thisDTC_->SetCFOEmulationNumNullHeartbeats(0x10);
	// registerWrite(0x91AC, 0); // Set number of heartbeats packets


	// check if any ROCs should be DTC-hardware emulated ROCs
	{
		std::vector<std::pair<std::string, ConfigurationTree>> rocChildren =
		    Configurable::getSelfNode().getNode("LinkToROCGroupTable").getChildren();

		int dtcHwEmulateROCmask = 0;
		for(auto& roc : rocChildren)
		{
			bool enabled = roc.second.getNode("EmulateInDTCHardware").getValue<bool>();

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
		thisDTC_->SetROCEmulatorMask(dtcHwEmulateROCmask);
		// registerWrite(0x9110, dtcHwEmulateROCmask);
		__FE_COUT__ << "End check for DTC-hardware emulated ROCs." << __E__;
	}  // end check if any ROCs should be DTC-hardware emulated ROCs

	//enable ROC links (do not forget CFO link is off in HW dev mode)
	__FE_COUT__ << "Enabling/Disabling DTC links with ROC mask = " << roc_mask_ << __E__;
	thisDTC_->DisableLink(DTCLib::DTC_Link_CFO);
	thisDTC_->DisableLink(DTCLib::DTC_Link_EVB);
	for(size_t i=0;i<DTCLib::DTC_ROC_Links.size();++i)
		if((roc_mask_ >> i) & 1)
			thisDTC_->EnableLink(DTCLib::DTC_ROC_Links[i]);
		else
			thisDTC_->DisableLink(DTCLib::DTC_ROC_Links[i]);
	// thisDTC_->SetROCDCSResponseTimer(1000); //Register removed as of Dec 2023 //set ROC DCS timeout (if 0, the DTC will hang forever when a ROC does not respond)
	thisDTC_->SetDMATimeoutPreset(0x00014141);  // DMA timeout from chants (default is 0x800)

	//in 13-Oct-2023 tests with Rick, reseting the serdes PLL brought the ROC tx back up
	// for(size_t i=0;i<DTCLib::DTC_PLLs.size();++i)
	// 	thisDTC_->ResetSERDESPLL(DTCLib::DTC_PLLs[i]);
	
	thisDTC_->ResetSERDESRX(DTCLib::DTC_Link_ID::DTC_Link_ALL);
	thisDTC_->ResetSERDESTX(DTCLib::DTC_Link_ID::DTC_Link_ALL);
}  // end configureHardwareDevMode()

//==============================================================================
void DTCFrontEndInterface::configureEventBuildingMode(int step)
{
	if(step == -1)
		step = getIterationIndex();

	__FE_COUT_INFO__ << "configureEventBuildingMode() " << step << __E__;
	
	if(emulate_cfo_) //when no CFO, what is event building mode?
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
		configureForTimingChain(getIterationIndex() - 
			CFOandDTCCoreVInterface::CONFIG_DTC_TIMING_CHAIN_START_INDEX /* start case index!! */);
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
		__FE_COUT__ << "Setup EVB parameters..." << __E__;
		uint32_t dtcEventBuilderReg_DTCID = 0;
		uint32_t dtcEventBuilderReg_Mode = 0;
		uint32_t dtcEventBuilderReg_PartitionID = 0;
		uint32_t dtcEventBuilderReg_MACIndex = 0;
		// uint32_t dtcEventBuilderReg_DTCInfo = 0;

		uint32_t dtcEventBuilderReg_NumBuff       = 0;
		uint32_t dtcEventBuilderReg_StartNode     = 0;
		uint32_t dtcEventBuilderReg_NumNodes      = 0;
		// uint32_t dtcEventBuilderReg_Configuration = 0;

		try
		{
			dtcEventBuilderReg_DTCID =
				getSelfNode().getNode("EventBuilderDTCID").getValue<uint32_t>();
			dtcEventBuilderReg_Mode =
				getSelfNode().getNode("EventBuilderMode").getValue<uint32_t>();
			dtcEventBuilderReg_PartitionID =
				getSelfNode().getNode("EventBuilderPartitionID").getValue<uint32_t>();
			dtcEventBuilderReg_MACIndex =
				getSelfNode().getNode("EventBuilderMACIndex").getValue<uint32_t>();

			dtcEventBuilderReg_NumBuff =
				getSelfNode().getNode("EventBuilderNumBuff").getValue<uint32_t>();
			dtcEventBuilderReg_StartNode =
				getSelfNode().getNode("EventBuilderStartNode").getValue<uint32_t>();
			dtcEventBuilderReg_NumNodes =
				getSelfNode().getNode("EventBuilderNumNodes").getValue<uint32_t>();

			__FE_COUTV__(dtcEventBuilderReg_DTCID);
			__FE_COUTV__(dtcEventBuilderReg_Mode);
			__FE_COUTV__(dtcEventBuilderReg_PartitionID);
			__FE_COUTV__(dtcEventBuilderReg_MACIndex);
			__FE_COUTV__(dtcEventBuilderReg_NumBuff);
			__FE_COUTV__(dtcEventBuilderReg_StartNode);
			__FE_COUTV__(dtcEventBuilderReg_NumNodes);

			// Register x9154 is #DTC ID [31-24] / EVB Mode [23-16]/ EVB Partition ID [15-8]/
			// EVB Local MAC Index [7-0]
			thisDTC_->SetEVBInfo(dtcEventBuilderReg_DTCID,
				dtcEventBuilderReg_Mode,
				dtcEventBuilderReg_PartitionID,
				dtcEventBuilderReg_MACIndex);
			// dtcEventBuilderReg_DTCInfo =
			//     dtcEventBuilderReg_DTCID << 24 | dtcEventBuilderReg_Mode << 16 |
			//     dtcEventBuilderReg_PartitionID << 8 | dtcEventBuilderReg_MACIndex;
			// __FE_COUTV__(dtcEventBuilderReg_DTCInfo);
			// registerWrite(0x9154, dtcEventBuilderReg_DTCInfo);

			// Register x9158 is #Num EVB Buffers[22-16], EVB Start Node [14-8], Num Nodes
			// [6-0]
			thisDTC_->SetEVBBufferInfo(dtcEventBuilderReg_NumBuff,
				dtcEventBuilderReg_StartNode,
				dtcEventBuilderReg_NumNodes);
			// dtcEventBuilderReg_Configuration = dtcEventBuilderReg_NumBuff << 16 |
			//                                    dtcEventBuilderReg_StartNode << 8 |
			//                                    dtcEventBuilderReg_NumNodes;
			// __FE_COUTV__(dtcEventBuilderReg_Configuration);
			// registerWrite(0x9158, dtcEventBuilderReg_Configuration);
		}
		catch(...)
		{
			__FE_COUT_INFO__ << "Ignoring missing event building configuration values."
								<< __E__;
		}
		
		// These registers are needed for the EVB, but I need to check their meaning
		// registerWrite(0x9100, 0x800404);
		thisDTC_->EnableAutogenDRP(); //bit 23
		thisDTC_->SetSequenceNumberDisable(); //bit 10

		// registerWrite(0x92c0, 0x0);
		thisDTC_->ClearEventModeTableEnable();
		thisDTC_->SetEventModeLookupByteSelect(0);

		// registerWrite(0x9114, 0xc1c1);
		thisDTC_->EnableLink(DTCLib::DTC_Link_EVB);

		// registerWrite(0x96C8, 0x555555D5); 	//10G configurable preamble world 
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

	if(emulate_cfo_) //when no CFO, what is loopback mode?
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
			thisDTC_->SoftReset();
			thisDTC_->ClearControlRegister();
			
			indicateIterationWork();
			break;
		case 1:
			{
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
					thisDTC_->SetJitterAttenuatorSelect(select, true /* alsoResetJA */);
				}
				else
					__FE_COUT_INFO__ << "Skipping configure clock." << __E__;
			}
			indicateIterationWork();
			break;
		case 2:

			// check if any ROCs should be DTC-hardware emulated ROCs
			{
				std::vector<std::pair<std::string, ConfigurationTree>> rocChildren =
					Configurable::getSelfNode().getNode("LinkToROCGroupTable").getChildren();

				int dtcHwEmulateROCmask = 0;
				for(auto& roc : rocChildren)
				{
					bool enabled = roc.second.getNode("EmulateInDTCHardware").getValue<bool>();

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
				thisDTC_->SetROCEmulatorMask(dtcHwEmulateROCmask);
				// registerWrite(0x9110, dtcHwEmulateROCmask);
				__FE_COUT__ << "End check for DTC-hardware emulated ROCs." << __E__;
			}  // end check if any ROCs should be DTC-hardware emulated ROCs
			
			//enable ROC links w/CFO link
			__FE_COUT__ << "Enabling/Disabling DTC links with ROC mask = " << roc_mask_ << __E__;
			thisDTC_->EnableLink(DTCLib::DTC_Link_CFO);
			thisDTC_->DisableLink(DTCLib::DTC_Link_EVB);
			for(size_t i = 0; i < DTCLib::DTC_ROC_Links.size(); ++i)
			{
				if((roc_mask_ >> i) & 1)
					thisDTC_->EnableLink(DTCLib::DTC_ROC_Links[i]);
				else
					thisDTC_->DisableLink(DTCLib::DTC_ROC_Links[i]);
			}
				
			// thisDTC_->SetROCDCSResponseTimer(1000); //Register removed as of Dec 2023 //set ROC DCS timeout (if 0, the DTC will hang forever when a ROC does not respond)
			thisDTC_->EnableDCSReception();

			__FE_COUT__ << "DTC reset links" << __E__;		
			// thisDTC_->ResetSERDESPLL(DTCLib::DTC_PLL_ID::DTC_PLL_CFO_RX);
			thisDTC_->ResetSERDESRX(DTCLib::DTC_Link_ID::DTC_Link_ALL);
			thisDTC_->ResetSERDESTX(DTCLib::DTC_Link_ID::DTC_Link_ALL);	
			thisDTC_->ResetSERDES(DTCLib::DTC_Link_ID::DTC_Link_ALL);
			break;
		default:
			__FE_COUT__ << "Do nothing while other configurable entities finish..." << __E__;
	}	


}  // end configureForTimingChain()

//==============================================================================
void DTCFrontEndInterface::halt(void)
{
	// if(regWriteMonitorStream_.is_open())
	// {
	// 	regWriteMonitorStream_ << "Timestamp: " << std::dec << time(0)
	// 	                       << ", \t ---------- Halting..."
	// 	                       << "\n";
	// 	regWriteMonitorStream_.flush();
	// }

	__FE_COUT__ << "Halting..." << __E__;

	for(auto& roc : rocs_)  // halt "as usual"
	{
		roc.second->halt();
	}

	rocs_.clear();

	__FE_COUT__ << "Halted." << __E__;

	// if(device_name_ == "DTC8")
	// {
	// 	__FE_COUT__ << "Forcing abort" << __E__;
	// 	abort();
	// }

	//	__FE_COUT__ << "HALT: DTC status" << __E__;
	//	readStatus();

	// if(runDataFile_.is_open())
	// 	runDataFile_.close();
}  // end halt()

//==============================================================================
void DTCFrontEndInterface::pause(void)
{
	// if(regWriteMonitorStream_.is_open())
	// {
	// 	regWriteMonitorStream_ << "Timestamp: " << std::dec << time(0)
	// 	                       << ", \t ---------- Pausing..."
	// 	                       << "\n";
	// 	regWriteMonitorStream_.flush();
	// }

	__FE_COUT__ << "Pausing..." << __E__;
	for(auto& roc : rocs_)  // pause "as usual"
	{
		roc.second->pause();
	}

	//	__FE_COUT__ << "PAUSE: DTC status" << __E__;
	//	readStatus();

	__FE_COUT__ << "Paused." << __E__;
}

//==============================================================================
void DTCFrontEndInterface::resume(void)
{
	// if(regWriteMonitorStream_.is_open())
	// {
	// 	regWriteMonitorStream_ << "Timestamp: " << std::dec << time(0)
	// 	                       << ", \t ---------- Resuming..."
	// 	                       << "\n";
	// 	regWriteMonitorStream_.flush();
	// }

	__FE_COUT__ << "Resuming..." << __E__;
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
	__FE_COUTV__(operatingMode_);
	__FE_COUTV__(emulatorMode_);

	if(operatingMode_ == "HardwareDevMode")
	{
		__FE_COUT_INFO__ << "Starting for hardware development mode!" << __E__;
	}
	else if(operatingMode_ == "EventBuildingMode")
	{
		__FE_COUT_INFO__ << "Starting for Event Building mode!" << __E__;
	}
	else if(operatingMode_ == "LoopbackMode")
	{
		__FE_COUT_INFO__ << "Starting for Loopback mode!" << __E__;
		loopbackTest();
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

	// // if(regWriteMonitorStream_.is_open())
	// // {
	// // 	regWriteMonitorStream_ << "Timestamp: " << std::dec << time(0)
	// // 	                       << ", \t ---------- Starting..."
	// // 	                       << "\n";
	// // 	regWriteMonitorStream_.flush();
	// // }

	// // open a file for this run number to write data to, if it hasn't been opened yet
	// // define a data file
	// if(!artdaqMode_)
	// {
	// 	std::string runDataFilename = std::string(__ENV__("OTSDAQ_DATA")) + "/RunData_" +
	// 	                              runNumber + "_" + device_name_ + ".dat";

	// 	__FE_COUTV__(runDataFilename);
	// 	if(runDataFile_.is_open())
	// 	{
	// 		__SS__
	// 		    << "File was left open! How was this possible -  open data file RunData: "
	// 		    << runDataFilename << __E__;
	// 		__SS_THROW__;
	// 	}

	// 	runDataFile_.open(runDataFilename, std::ios::out | std::ios::app);

	// 	if(runDataFile_.fail())
	// 	{
	// 		__SS__ << "FAILED to open data file RunData: " << runDataFilename << __E__;
	// 		__SS_THROW__;
	// 	}

	// }  // end local run file creation

	// if(emulatorMode_)
	// {
	// 	__FE_COUT__ << "Emulator DTC starting... # of ROCs = " << rocs_.size() << __E__;
	// 	for(auto& roc : rocs_)
	// 	{
	// 		__FE_COUT__ << "Starting ROC ";
	// 		roc.second->start(runNumber);
	// 		__FE_COUT__ << "Done starting ROC";
	// 	}
	// 	return;
	// }

	// int numberOfLoopbacks =
	//     getConfigurationManager()
	//         ->getNode("/Mu2eGlobalsTable/SyncDemoConfig/NumberOfLoopbacks")
	//         .getValue<unsigned int>();

	// __FE_COUTV__(numberOfLoopbacks);

	// // int stopIndex = getIterationIndex();

	// if(numberOfLoopbacks == 0)
	// {
	// 	for(auto& roc : rocs_)  // start "as usual"
	// 	{
	// 		roc.second->start(runNumber);
	// 	}
	// 	return;
	// }

	// __MCOUT_INFO__(device_name_ << " Ignoring loopback for now..." << __E__);
	// return;  // for now ignore loopback mode

	// const int numberOfChains = 1;
	// // int       link[numberOfChains] = {0};

	// const int numberOfDTCsPerChain = 1;

	// const int numberOfROCsPerDTC = 1;  // assume these are ROC0 and ROC1

	// // To do loopbacks on all CFOs, first have to setup all DTCs, then the CFO
	// // (this method) work per iteration.  Loop back done on all chains (in this
	// // method), assuming the following order: i  DTC0  DTC1  ...  DTCN 0  ROC0
	// // none  ...  none 1  ROC1  none  ...  none 2  none  ROC0  ...  none 3  none
	// // ROC1  ...  none
	// // ...
	// // N-1  none  none  ...  ROC0
	// // N  none  none  ...  ROC1

	// int totalNumberOfMeasurements =
	//     numberOfChains * numberOfDTCsPerChain * numberOfROCsPerDTC;

	// int loopbackIndex = getIterationIndex();

	// if(loopbackIndex == 0)  // start
	// {
	// 	initial_9100_ = registerRead(0x9100);
	// 	initial_9114_ = registerRead(0x9114);
	// 	indicateIterationWork();
	// 	return;
	// }

	// if(loopbackIndex > totalNumberOfMeasurements)  // finish
	// {
	// 	__MCOUT_INFO__(device_name_ << " loopback DONE" << __E__);

	// 	if(checkLinkStatus() == 1)
	// 	{
	// 		//      __MCOUT_INFO__(device_name_ << " links OK 0x" << std::hex <<
	// 		//      registerRead(0x9140) << std::dec << __E__);
	// 	}
	// 	else
	// 	{
	// 		//      __MCOUT_INFO__(device_name_ << " links not OK 0x" << std::hex <<
	// 		//      registerRead(0x9140) << std::dec << __E__);
	// 	}

	// 	if(0)
	// 		for(auto& roc : rocs_)
	// 		{
	// 			__MCOUT_INFO__(".... ROC" << roc.second->getLinkID() << "-DTC link lost "
	// 			                          << roc.second->readDTCLinkLossCounter()
	// 			                          << " times");
	// 		}

	// 	registerWrite(0x9100, initial_9100_);
	// 	registerWrite(0x9114, initial_9114_);

	// 	return;
	// }

	// //=========== Perform loopback=============

	// // where are we in the procedure?
	// unsigned int activeROC = (loopbackIndex - 1) % numberOfROCsPerDTC;

	// int activeDTC = -1;

	// for(int nDTC = 0; nDTC < numberOfDTCsPerChain; nDTC++)
	// {
	// 	if((loopbackIndex - 1) >= (nDTC * numberOfROCsPerDTC) &&
	// 	   (loopbackIndex - 1) < ((nDTC + 1) * numberOfROCsPerDTC))
	// 	{
	// 		activeDTC = nDTC;
	// 	}
	// }

	// // __FE_COUT__ << "loopback index = " << loopbackIndex
	// // 	<< " activeDTC = " << activeDTC
	// //	 	<< " activeROC = " << activeROC
	// //	 	<< __E__;

	// if(activeDTC == dtc_location_in_chain_)
	// {
	// 	__FE_COUT__ << "DTC" << activeDTC << "loopback mode ENABLE" << __E__;
	// 	int dataInReg   = registerRead(0x9100);
	// 	int dataToWrite = dataInReg & 0xefffffff;  // bit 28 = 0
	// 	registerWrite(0x9100, dataToWrite);
	// }
	// else
	// {
	// 	// this DTC is lower in chain than the one being looped.  Pass the loopback
	// 	// signal through
	// 	__FE_COUT__ << "active DTC = " << activeDTC
	// 	            << " is NOT this DTC = " << dtc_location_in_chain_
	// 	            << "... pass signal through" << __E__;

	// 	int dataInReg   = registerRead(0x9100);
	// 	int dataToWrite = dataInReg | 0x10000000;  // bit 28 = 1
	// 	registerWrite(0x9100, dataToWrite);
	// }

	// int ROCToEnable =
	//     0x00004040 |
	//     (0x101 << activeROC);  // enables TX and Rx to CFO (bit 6) and appropriate ROC
	// __FE_COUT__ << "enable ROC " << activeROC << " --> 0x" << std::hex << ROCToEnable
	//             << std::dec << __E__;

	// registerWrite(0x9114, ROCToEnable);

	// indicateIterationWork();  // FIXME -- go back to including the ROC (could not 'read'
	//                           // for some reason)
	// return;
	// // Re-align the links for the activeROC
	// for(auto& roc : rocs_)
	// {
	// 	if(roc.second->getLinkID() == activeROC)
	// 	{
	// 		__FE_COUT__ << "... ROC realign link... " << __E__;
	// 		roc.second->writeRegister(22, 0);
	// 		roc.second->writeRegister(22, 1);
	// 	}
	// }

	// indicateIterationWork();
	// return;
}  // end start()

//==============================================================================
void DTCFrontEndInterface::stop(void)
{
	// if(regWriteMonitorStream_.is_open())
	// {
	// 	regWriteMonitorStream_ << "---------- Stopping..."
	// 	                       << "\n";
	// 	regWriteMonitorStream_.flush();
	// }

	// must close data file on each possible return with call 'if(runDataFile_.is_open())
	// runDataFile_.close();'

	if(emulatorMode_)
	{
		__FE_COUT__ << "Emulator DTC stopping... # of ROCs = " << rocs_.size() << __E__;
		for(auto& roc : rocs_)
			roc.second->stop();

		// if(runDataFile_.is_open())
		// 	runDataFile_.close();
		// return;
	}

	int numberOfCAPTANPulses =
	    getConfigurationManager()
	        ->getNode("/Mu2eGlobalsTable/SyncDemoConfig/NumberOfCAPTANPulses")
	        .getValue<unsigned int>();

	__FE_COUTV__(numberOfCAPTANPulses);

	// int stopIndex = getIterationIndex();

	if(numberOfCAPTANPulses == 0)
	{
		for(auto& roc : rocs_)  // stop "as usual"
		{
			roc.second->stop();
		}
		// if(runDataFile_.is_open())
		// 	runDataFile_.close();
		// return;
	}

	// if(stopIndex == 0)
	// {
	// 	//		int i = 0;
	// 	for(auto& roc : rocs_)
	// 	{
	// 		// re-align link
	// 		roc.second->writeRegister(22, 0);
	// 		roc.second->writeRegister(22, 1);

	// 		// std::stringstream filename;
	// 		// filename << "/home/mu2edaq/sync_demo/ots/" << device_name_ << "_ROC"
	// 		//          << roc.second->getLinkID() << "data.txt";
	// 		// std::string filenamestring = filename.str();
	// 		// datafile_[i].open(filenamestring);
	// 		//	i++;
	// 	}
	// }

	// if(stopIndex > numberOfCAPTANPulses)
	// {
	// 	int i = 0;
	// 	for(auto& roc : rocs_)
	// 	{
	// 		__MCOUT_INFO__(".... ROC" << roc.second->getLinkID() << "-DTC link lost "
	// 		                          << roc.second->readDTCLinkLossCounter()
	// 		                          << " times");
	// 		datafile_[i].close();
	// 		i++;
	// 	}
	// 	if(runDataFile_.is_open())
	// 		runDataFile_.close();
	// 	return;
	// }

	// int i = 0;
	// __FE_COUT__ << "Entering read timestamp loop..." << __E__;
	// for(auto& roc : rocs_)
	// {
	// 	int timestamp_data = roc.second->readInjectedPulseTimestamp();

	// 	__FE_COUT__ << "Read " << stopIndex << " -> " << device_name_ << " timestamp "
	// 	            << timestamp_data << __E__;

	// 	datafile_[i] << stopIndex << " " << timestamp_data << std::endl;
	// 	i++;
	// }

	// indicateIterationWork();
	// return;
}  // end stop()

//==============================================================================
// return true to keep running
bool DTCFrontEndInterface::running(void)
{
	__FE_COUTV__(operatingMode_);
	__FE_COUTV__(emulatorMode_);

	if(operatingMode_ == "HardwareDevMode")
	{
		__FE_COUT_INFO__ << "Running for hardware development mode!" << __E__;
	}
	else if(operatingMode_ == "EventBuildingMode")
	{
		__FE_COUT_INFO__ << "Running for Event Building mode!" << __E__;
	}
	else if(operatingMode_ == "LoopbackMode")
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

	return false;

	// /////////////////////////////
	// /////////////////////////////
	// ///////////////////////////// old running
	// ///////////////////////////// old running
	// ///////////////////////////// old running
	// ///////////////////////////// old running
	// /////////////////////////////
	// /////////////////////////////

	// //   if(artdaqMode_) {
	// //     __FE_COUT__ << "Running in artdaqmode" << __E__;
	// return true;
	// //   }
	// if(emulatorMode_)
	// {
	// 	__FE_COUT__ << "Emulator DTC running... # of ROCs = " << rocs_.size() << __E__;
	// 	bool stillRunning = false;
	// 	for(auto& roc : rocs_)
	// 		stillRunning = stillRunning || roc.second->running();

	// 	return stillRunning;
	// }

	// // first setup DTC and CFO.  This is stolen from "getheartbeatanddatarequest"

	// //	auto start = DTCLib::DTC_EventWindowTag(static_cast<uint64_t>(timestampStart));

	// //    std::time_t current_time;

	// bool incrementTimestamp = true;

	// uint32_t cfodelay = 10000;  // have no idea what this is, but 1000 didn't work (don't
	//                             // know if 10000 works, either)
	// int          requestsAhead  = 0;
	// unsigned int number         = -1;  // largest number of events?
	// unsigned int timestampStart = 0;

	// auto device = thisDTC_->GetDevice();
	// // auto initTime = device->GetDeviceTime();
	// device->ResetDeviceTime();
	// // auto afterInit = std::chrono::steady_clock::now();

	// if(emulate_cfo_ == 1)
	// {
	// 	registerWrite(
	// 	    0x9100, 0x40008404);  // bit 30 = CFO emulation enable, bit 15 = CFO
	// 	                          // emulation mode, bit 2 = DCS enable
	// 	                          // bit 10 turns off retry which isn't working right now
	// 	sleep(1);

	// 	// set number of null heartbeats
	// 	// registerWrite(0x91BC, 0x0);
	// 	registerWrite(0x91BC, 0x10);  // new incantaton from Rick K. 12/18/2019
	// 	//	  sleep(1);

	// 	// # Send data
	// 	// #disable 40mhz marker
	// 	registerWrite(0x91f4, 0x0);
	// 	//	  sleep(1);

	// 	// #set num dtcs
	// 	registerWrite(0x9158, 0x1);
	// 	//	  sleep(1);

	// 	bool     useSWCFOEmulator = true;
	// 	uint16_t debugPacketCount = 0;
	// 	auto     debugType        = DTCLib::DTC_DebugType_SpecialSequence;
	// 	bool     stickyDebugType  = true;
	// 	bool     quiet            = false;
	// 	bool     asyncRR          = false;
	// 	bool     forceNoDebugMode = true;

	// 	DTCLib::DTCSoftwareCFO* EmulatedCFO_ =
	// 	    new DTCLib::DTCSoftwareCFO(thisDTC_,
	// 	                               useSWCFOEmulator,
	// 	                               debugPacketCount,
	// 	                               debugType,
	// 	                               stickyDebugType,
	// 	                               quiet,
	// 	                               asyncRR,
	// 	                               forceNoDebugMode);

	// 	EmulatedCFO_->SendRequestsForRange(
	// 	    number,
	// 	    DTCLib::DTC_EventWindowTag(static_cast<uint64_t>(timestampStart)),
	// 	    incrementTimestamp,
	// 	    cfodelay,
	// 	    requestsAhead);

	// 	// auto readoutRequestTime = device->GetDeviceTime();
	// 	device->ResetDeviceTime();
	// 	// auto afterRequests = std::chrono::steady_clock::now();
	// }

	// while(WorkLoop::continueWorkLoop_)
	// {
	// 	registerWrite(
	// 	    0x9100, 0x40008404);  // bit 30 = CFO emulation enable, bit 15 = CFO
	// 	                          // emulation mode, bit 2 = DCS enable
	// 	                          // bit 10 turns off retry which isn't working right now
	// 	sleep(1);

	// 	// set number of null heartbeats
	// 	// registerWrite(0x91BC, 0x0);
	// 	registerWrite(0x91BC, 0x10);  // new incantaton from Rick K. 12/18/2019
	// 	//	  sleep(1);

	// 	// # Send data
	// 	// #disable 40mhz marker
	// 	registerWrite(0x91f4, 0x0);
	// 	//	  sleep(1);

	// 	// #set num dtcs
	// 	registerWrite(0x9158, 0x1);
	// 	//	  sleep(1);

	// 	bool     useSWCFOEmulator = true;
	// 	uint16_t debugPacketCount = 0;
	// 	auto     debugType        = DTCLib::DTC_DebugType_SpecialSequence;
	// 	bool     stickyDebugType  = true;
	// 	bool     quiet            = false;
	// 	bool     asyncRR          = false;
	// 	bool     forceNoDebugMode = true;

	// 	DTCLib::DTCSoftwareCFO* EmulatedCFO_ =
	// 	    new DTCLib::DTCSoftwareCFO(thisDTC_,
	// 	                               useSWCFOEmulator,
	// 	                               debugPacketCount,
	// 	                               debugType,
	// 	                               stickyDebugType,
	// 	                               quiet,
	// 	                               asyncRR,
	// 	                               forceNoDebugMode);

	// 	EmulatedCFO_->SendRequestsForRange(
	// 	    number,
	// 	    DTCLib::DTC_EventWindowTag(static_cast<uint64_t>(timestampStart)),
	// 	    incrementTimestamp,
	// 	    cfodelay,
	// 	    requestsAhead);

	// 	// auto readoutRequestTime = device->GetDeviceTime();
	// 	device->ResetDeviceTime();
	// 	// auto afterRequests = std::chrono::steady_clock::now();
	// }

	// while(WorkLoop::continueWorkLoop_)
	// {
	// 	for(auto& roc : rocs_)
	// 	{
	// 		roc.second->running();
	// 	}

	// 	// print out stuff
	// 	unsigned quietCount = 20;
	// 	bool     quiet      = false;

	// 	std::stringstream ostr;
	// 	ostr << std::endl;

	// 	//		std::cout << "Buffer Read " << std::dec << ii << std::endl;
	// 	mu2e_databuff_t* buffer;
	// 	auto             tmo_ms = 1500;
	// 	__FE_COUT__ << "util - before read for DAQ in running";
	// 	auto sts = device->read_data(
	// 	    DTC_DMA_Engine_DAQ, reinterpret_cast<void**>(&buffer), tmo_ms);
	// 	__FE_COUT__ << "util - after read for DAQ in running "
	// 	            << " sts=" << sts << ", buffer=" << (void*)buffer;

	// 	if(sts > 0)
	// 	{
	// 		void* readPtr = &buffer[0];
	// 		auto  bufSize = static_cast<uint16_t>(*static_cast<uint64_t*>(readPtr));
	// 		readPtr       = static_cast<uint8_t*>(readPtr) + 8;

	// 		__FE_COUT__ << "Buffer reports DMA size of " << std::dec << bufSize
	// 		            << " bytes. Device driver reports read of " << sts << " bytes,"
	// 		            << std::endl;

	// 		__FE_COUT__ << "util - bufSize is " << bufSize;
	// 		outputStream.write(static_cast<char*>(readPtr), sts - 8);
	// 		auto maxLine = static_cast<unsigned>(ceil((sts - 8) / 16.0));
	// 		__FE_COUT__ << "maxLine " << maxLine;
	// 		for(unsigned line = 0; line < maxLine; ++line)
	// 		{
	// 			ostr << "0x" << std::hex << std::setw(5) << std::setfill('0') << line
	// 			     << "0: ";
	// 			for(unsigned byte = 0; byte < 8; ++byte)
	// 			{
	// 				if(line * 16 + 2 * byte < sts - 8u)
	// 				{
	// 					auto thisWord =
	// 					    reinterpret_cast<uint16_t*>(buffer)[4 + line * 8 + byte];
	// 					ostr << std::setw(4) << static_cast<int>(thisWord) << " ";
	// 				}
	// 			}

	// 			ostr << std::endl;
	// 			//	std::cout << ostr.str();

	// 			//     __SET_ARG_OUT__("readData", ostr.str());  // write to data file

	// 			// don't write data to the log file, only the data file
	// 			// __FE_COUT__ << ostr.str();

	// 			if(maxLine > quietCount * 2 && quiet && line == (quietCount - 1))
	// 			{
	// 				line =
	// 				    static_cast<unsigned>(ceil((sts - 8) / 16.0)) - (1 + quietCount);
	// 			}
	// 		}
	// 	}
	// 	device->read_release(DTC_DMA_Engine_DAQ, 1);

	// 	ostr << std::endl;

	// 	if(runDataFile_.is_open())
	// 	{
	// 		runDataFile_ << ostr.str();
	// 		runDataFile_.flush();  // flush to disk
	// 	}
	// 	//__FE_COUT__ << ostr.str();

	// 	delete EmulatedCFO_;

	// 	break;
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

	DTCLib::DTC_Link_ID rocLinkIndex =
	    DTCLib::DTC_Link_ID(__GET_ARG_IN__("rocLinkIndex", uint8_t));
	DTCLib::roc_address_t address = __GET_ARG_IN__("address", DTCLib::roc_address_t);
	__FE_COUTV__(rocLinkIndex);
	__FE_COUTV__((unsigned int)address);

	DTCLib::roc_data_t readData = -999;

	for(auto& roc : rocs_)
	{
		__FE_COUT__ << "Found link ID " << roc.second->getLinkID() << " looking for "
		            << rocLinkIndex << __E__;

		if(rocLinkIndex == roc.second->getLinkID())
		{
			// readData = roc.second->readRegister(address);

			readData = thisDTC_->ReadROCRegister(rocLinkIndex, address, 300);

			char readDataStr[100];
			sprintf(readDataStr, "0x%X", readData);
			__SET_ARG_OUT__("readData", readDataStr);
			//__SET_ARG_OUT__("readData", readData);

			// for(auto &argOut:argsOut)
			__FE_COUT__ << "readData"
			            << ": " << std::hex << readData << std::dec << __E__;
			__FE_COUT__ << "End of Data";
			return;
		}
	}

	__FE_SS__ << "ROC link ID " << rocLinkIndex << 
		" not found! Check the configuration of the DTC to make sure a child ROC is enabled at link " << 
		rocLinkIndex << "." << __E__;
	__FE_SS_THROW__;

}  // end ReadROC()

//==============================================================================
// DTCStatus
//	FEMacro 'DTCStatus' generated, Oct-22-2018 03:16:46, by 'admin' using
// MacroMaker. 	Macro Notes:
void DTCFrontEndInterface::WriteROC(__ARGS__)
{
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	DTCLib::DTC_Link_ID rocLinkIndex =
	    DTCLib::DTC_Link_ID(__GET_ARG_IN__("rocLinkIndex", uint8_t));
	DTCLib::roc_address_t  address   = __GET_ARG_IN__("address", DTCLib::roc_address_t);
	DTCLib::roc_data_t writeData = __GET_ARG_IN__("writeData", DTCLib::roc_data_t);

	__FE_COUTV__(rocLinkIndex);
	__FE_COUTV__((unsigned int)address);
	__FE_COUTV__(writeData);

	__FE_COUT__ << "ROCs size = " << rocs_.size() << __E__;

	for(auto& roc : rocs_)
	{
		__FE_COUT__ << "Found link ID " << roc.second->getLinkID() << " looking for "
		            << rocLinkIndex << __E__;

		if(rocLinkIndex == roc.second->getLinkID())
		{
			roc.second->writeRegister(address, writeData);

			for(auto& argOut : argsOut)
				__FE_COUT__ << argOut.first << ": " << argOut.second << __E__;

			return;
		}
	}

	__FE_SS__ << "ROC link ID " << rocLinkIndex << " not found!" << __E__;
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

	DTCLib::DTC_Link_ID rocLinkIndex =
	    DTCLib::DTC_Link_ID(__GET_ARG_IN__("rocLinkIndex", uint8_t));
	DTCLib::roc_address_t  address   = __GET_ARG_IN__("address", DTCLib::roc_address_t);
	DTCLib::roc_data_t writeData = __GET_ARG_IN__("writeData", DTCLib::roc_data_t);
	DTCLib::roc_address_t   block     = __GET_ARG_IN__("block", DTCLib::roc_address_t );
	__FE_COUTV__(rocLinkIndex);
	__FE_COUT__ << "block = " << std::dec << (unsigned int)block << __E__;
	__FE_COUT__ << "address = 0x" << std::hex << (unsigned int)address << std::dec
	            << __E__;
	__FE_COUT__ << "writeData = 0x" << std::hex << writeData << std::dec << __E__;

	bool acknowledge_request = false;

	thisDTC_->WriteExtROCRegister(
	    rocLinkIndex, block, address, writeData, acknowledge_request, 0);

	for(auto& argOut : argsOut)
		__FE_COUT__ << argOut.first << ": " << argOut.second << __E__;
}  // end WriteExternalROCRegister()

//==============================================================================
void DTCFrontEndInterface::ReadExternalROCRegister(__ARGS__)
{
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	DTCLib::DTC_Link_ID rocLinkIndex =
	    DTCLib::DTC_Link_ID(__GET_ARG_IN__("rocLinkIndex", uint8_t));
	DTCLib::roc_address_t  address = __GET_ARG_IN__("address", DTCLib::roc_address_t );
	DTCLib::roc_address_t  block   = __GET_ARG_IN__("block", DTCLib::roc_address_t );
	__FE_COUTV__(rocLinkIndex);
	__FE_COUT__ << "block = " << std::dec << (unsigned int)block << __E__;
	__FE_COUT__ << "address = 0x" << std::hex << (unsigned int)address << std::dec
	            << __E__;

	// bool acknowledge_request = false;

	for(auto& roc : rocs_)
	{
		__FE_COUT__ << "At ROC link ID " << roc.second->getLinkID() << ", looking for "
		            << rocLinkIndex << __E__;

		if(rocLinkIndex == roc.second->getLinkID())
		{
			DTCLib::roc_data_t readData;

			readData = thisDTC_->ReadExtROCRegister(rocLinkIndex, block, address);

			std::string readDataString = "";
			readDataString = BinaryStringMacros::binaryNumberToHexString(readData);

			// StringMacros::vectorToString(readData);

			__SET_ARG_OUT__("readData", readDataString);

			// for(auto &argOut:argsOut)
			__FE_COUT__ << "readData"
			            << ": " << readDataString << __E__;
			return;
		}
	}

	__FE_SS__ << "ROC link ID " << rocLinkIndex << " not found!" << __E__;
	__FE_SS_THROW__;
}  // end ReadExternalROCRegister()

//========================================================================
void DTCFrontEndInterface::BockReadROC(__ARGS__)
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

	DTCLib::DTC_Link_ID rocLinkIndex =
	    DTCLib::DTC_Link_ID(__GET_ARG_IN__("rocLinkIndex", uint8_t));
	DTCLib::roc_address_t address          = __GET_ARG_IN__("address", DTCLib::roc_address_t);
	uint16_t wordCount        = __GET_ARG_IN__("Number Of 16-bit words to Read (Default := 8)", uint16_t, 8);
	bool     incrementAddress = __GET_ARG_IN__("incrementAddress (Default := false)", bool);

	__FE_COUTV__(rocLinkIndex);
	__FE_COUT__ << "address = 0x" << std::hex << (unsigned int)address << std::dec
	            << __E__;
	__FE_COUT__ << "numberOfWords = " << std::dec << (unsigned int)wordCount << __E__;
	__FE_COUTV__(incrementAddress);

	for(auto& roc : rocs_)
	{
		__FE_COUT__ << "At ROC link ID " << roc.second->getLinkID() << ", looking for "
		            << rocLinkIndex << __E__;

		if(rocLinkIndex == roc.second->getLinkID())
		{
			std::vector<DTCLib::roc_data_t> readData;

			roc.second->readBlock(readData, address, wordCount, incrementAddress);

			std::string readDataString = "";
			{
				for(size_t i=0; i<readData.size(); ++i)
				{
					if(i) readDataString += ", ";
					if(i && i%8 == 0) readDataString += "\n\t";
					readDataString += BinaryStringMacros::binaryNumberToHexString(readData[i]);					
				}
			}
			// StringMacros::vectorToString(readData);

			__SET_ARG_OUT__("readData", readDataString);

			// for(auto &argOut:argsOut)
			__FE_COUT__ << "readData"
			            << ": " << readDataString << __E__;
			return;
		}
	}

	__FE_SS__ << "ROC link ID " << rocLinkIndex << " not found!" << __E__;
	__FE_SS_THROW__;

}  // end BockReadROC()

//========================================================================
void DTCFrontEndInterface::BockWriteROC(__ARGS__)
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

	DTCLib::DTC_Link_ID rocLinkIndex =
	    DTCLib::DTC_Link_ID(__GET_ARG_IN__("rocLinkIndex", uint8_t));
	DTCLib::roc_address_t address          = __GET_ARG_IN__("address", DTCLib::roc_address_t);
	std::string writeDataIn   = __GET_ARG_IN__("writeData (CSV-literal or CSV-filename of 16-bit words, or keyword + parameter 'AUTOGENERATE count')", std::string);
	bool     incrementAddress = __GET_ARG_IN__("incrementAddress (Default = false)", bool);
	bool     requestAck = __GET_ARG_IN__("requestAck (Default = false)", bool);
	std::vector<DTCLib::roc_data_t> writeData;

	__FE_COUTV__(rocLinkIndex);
	__FE_COUT__ << "address = 0x" << std::hex << (unsigned int)address << std::dec
	            << __E__;
	__FE_COUTV__(incrementAddress);
	__FE_COUTV__(requestAck);

	if(writeDataIn.find("AUTOGENERATE") != std::string::npos)
	{
		__FE_COUT__ << "Auto-generating ROC write block data... 2nd argument is count." << __E__;
		std::vector<std::string> split = StringMacros::getVectorFromString(writeDataIn,
			{',', '|', '&', ' '} /* delimeters */);
		__FE_COUTV__(StringMacros::vectorToString(split));
		if(split.size() != 2)
		{
			__FE_SS__ << "Illegal ROC write block auto-generate parameters found: " << writeDataIn << __E__
				<< "Must be 'AUTOGENERATE <count>' where count is a number." << __E__;
			__FE_SS_THROW__;			
		}
		uint16_t count;
		StringMacros::getNumber(split[1], count);
		__FE_COUTV__(count);
		//generate incrementing counter data
		for(uint16_t i=0; i<count; ++i)
			writeData.push_back(i);
	}
	else if(writeDataIn.find(',') == std::string::npos)
	{
		__FE_COUT__ << "Assuming write data is a CSV filename (because no comma found): " << writeDataIn << __E__;
		FILE* fp = fopen(writeDataIn.c_str(), "r");
		if(fp)
		{
			fseek(fp, 0, SEEK_END);
			const unsigned long fileSize = ftell(fp);
			size_t readSize;
			writeDataIn.resize(fileSize);
			rewind(fp);
			if((readSize = fread(&writeDataIn[0], 1, fileSize, fp)) != fileSize)
			{
				__FE_SS__ << "CSV filename (because no comma found) could not be read! Wrong byte count returned: readSize=" << 
					readSize << " vs fileSize=" << fileSize << __E__;
				__FE_SS_THROW__;
			}

			fclose(fp);
		}
		else
		{
				__FE_SS__ << "CSV filename (because no comma found) was not founnd: " << writeDataIn << __E__;
				__FE_SS_THROW__;
		}
	} //end CSV file handling
	else //literal
	{	
		__FE_COUT__ << "CSV literal: " << writeDataIn << __E__;
		std::vector<std::string> writeDataStrings = StringMacros::getVectorFromString(writeDataIn);
		for(auto& writeDataString : writeDataStrings)
		{
			DTCLib::roc_data_t number;
			StringMacros::getNumber(writeDataString, number);
			writeData.push_back(number);
		}		
	}

	__FE_COUTV__(StringMacros::vectorToString(writeData));	
	__FE_COUT__ << "numberOfWords = " << std::dec << (unsigned int)writeData.size() << __E__;


	for(auto& roc : rocs_)
	{
		__FE_COUT__ << "At ROC link ID " << roc.second->getLinkID() << ", looking for "
		            << rocLinkIndex << __E__;

		if(rocLinkIndex == roc.second->getLinkID())
		{
			roc.second->writeBlock(writeData, address, incrementAddress, requestAck);

			// for(auto &argOut:argsOut)
			std::stringstream ss; ss << "Wrote " << writeData.size() << " words to address 0x" << std::hex << address <<
				", incrementingAddress=" << (incrementAddress?"TRUE":"FALSE") <<
				", requestAck=" << (requestAck?"TRUE":"FALSE") << __E__;
			__FE_COUT__ << ss.str();
			__SET_ARG_OUT__("Status", ss.str());

			return;
		}
	}

	__FE_SS__ << "ROC link ID " << rocLinkIndex << " not found!" << __E__;
	__FE_SS_THROW__;

}  // end BockWriteROC()

// //========================================================================
// void DTCFrontEndInterface::DTCHighRateBlockCheck(__ARGS__)
// {
// 	unsigned int linkIndex   = __GET_ARG_IN__("rocLinkIndex", unsigned int);
// 	unsigned int loops       = __GET_ARG_IN__("loops", unsigned int);
// 	unsigned int baseAddress = __GET_ARG_IN__("baseAddress", unsigned int);
// 	unsigned int correctRegisterValue0 =
// 	    __GET_ARG_IN__("correctRegisterValue0", unsigned int);
// 	unsigned int correctRegisterValue1 =
// 	    __GET_ARG_IN__("correctRegisterValue1", unsigned int);

// 	__FE_COUTV__(linkIndex);
// 	__FE_COUTV__(loops);
// 	__FE_COUTV__(baseAddress);
// 	__FE_COUTV__(correctRegisterValue0);
// 	__FE_COUTV__(correctRegisterValue1);

// 	for(auto& roc : rocs_)
// 		if(roc.second->getLinkID() == linkIndex)
// 		{
// 			roc.second->highRateBlockCheck(
// 			    loops, baseAddress, correctRegisterValue0, correctRegisterValue1);
// 			return;
// 		}

// 	__FE_SS__ << "Error! Could not find ROC at link index " << linkIndex << __E__;
// 	__FE_SS_THROW__;

// }  // end DTCHighRateBlockCheck()

// //========================================================================
// void DTCFrontEndInterface::DTCHighRateDCSCheck(__ARGS__)
// {
// 	unsigned int linkIndex   = __GET_ARG_IN__("rocLinkIndex", unsigned int);
// 	unsigned int loops       = __GET_ARG_IN__("loops", unsigned int);
// 	unsigned int baseAddress = __GET_ARG_IN__("baseAddress", unsigned int);
// 	unsigned int correctRegisterValue0 =
// 	    __GET_ARG_IN__("correctRegisterValue0", unsigned int);
// 	unsigned int correctRegisterValue1 =
// 	    __GET_ARG_IN__("correctRegisterValue1", unsigned int);

// 	__FE_COUTV__(linkIndex);
// 	__FE_COUTV__(loops);
// 	__FE_COUTV__(baseAddress);
// 	__FE_COUTV__(correctRegisterValue0);
// 	__FE_COUTV__(correctRegisterValue1);

// 	for(auto& roc : rocs_)
// 		if(roc.second->getLinkID() == linkIndex)
// 		{
// 			roc.second->highRateCheck(
// 			    loops, baseAddress, correctRegisterValue0, correctRegisterValue1);
// 			return;
// 		}

// 	__FE_SS__ << "Error! Could not find ROC at link index " << linkIndex << __E__;
// 	__FE_SS_THROW__;

// }  // end DTCHighRateDCSCheck()

// //========================================================================
// void DTCFrontEndInterface::DTCSendHeartbeatAndDataRequest(__ARGS__)
// {
// 	unsigned int number           = __GET_ARG_IN__("numberOfRequests", unsigned int);
// 	unsigned int timestampStart   = __GET_ARG_IN__("timestampStart", unsigned int);
// 	bool         useSWCFOEmulator = __GET_ARG_IN__("useSWCFOEmulator", bool);
// 	unsigned int rocMask          = __GET_ARG_IN__("rocMask", unsigned int);

// 	//	auto start = DTCLib::DTC_EventWindowTag(static_cast<uint64_t>(timestampStart));

// 	bool     incrementTimestamp = true;
// 	uint32_t cfodelay = 20000;  // have no idea what this is, but 1000 didn't work (don't
// 	                            // know if 10000 works, either)
// 	int requestsAhead    = 0;
// 	int heartbeatPackets = 32;

// 	__FE_COUTV__(number);
// 	__FE_COUTV__(timestampStart);
// 	__FE_COUTV__(useSWCFOEmulator);
// 	__FE_COUTV__(rocMask);

// 	if(thisDTC_)
// 		delete thisDTC_;
// 	thisDTC_    = new DTCLib::DTC(DTCLib::DTC_SimMode_NoCFO, deviceIndex_, rocMask, "");
// 	auto device = thisDTC_->GetDevice();

// 	// auto initTime = device->GetDeviceTime();
// 	device->ResetDeviceTime();
// 	// auto afterInit = std::chrono::steady_clock::now();

// 	if(emulate_cfo_ == 1)
// 	{
// 		thisDTC_->SetSequenceNumberDisable();  // Set 9100 bit 10
// 		// registerWrite(0x9100, 0x40808404);  // bit 30 = CFO emulation enable, bit 15 =
// 		// CFO
// 		//  emulation mode, bit 2 = DCS enable
// 		//  bit 10 turns off retry which isn't working right now
// 		//	sleep(1);

// 		// set number of null heartbeats
// 		// registerWrite(0x91BC, 0x0);
// 		//	registerWrite(0x91BC, 0x10);  // new incantaton from Rick K. 12/18/2019
// 		//	  sleep(1);

// 		// # Send data
// 		// #disable 40mhz marker
// 		thisDTC_->SetCFOEmulation40MHzMarkerInterval(0);
// 		// registerWrite(0x91f4, 0x0);
// 		//	  sleep(1);

// 		// #set num dtcs
// 		thisDTC_->SetEVBNumberOfDestinationNodes(1);
// 		// registerWrite(0x9158, 0x1);
// 		//	  sleep(1);

// 		uint16_t debugPacketCount = 0;
// 		auto     debugType        = DTCLib::DTC_DebugType_SpecialSequence;
// 		bool     stickyDebugType  = true;
// 		bool     quiet            = false;
// 		bool     asyncRR          = false;
// 		bool     forceNoDebugMode = true;

// 		DTCLib::DTCSoftwareCFO* EmulatedCFO_ =
// 		    new DTCLib::DTCSoftwareCFO(thisDTC_,
// 		                               useSWCFOEmulator,
// 		                               debugPacketCount,
// 		                               debugType,
// 		                               stickyDebugType,
// 		                               quiet,
// 		                               asyncRR,
// 		                               forceNoDebugMode);

// 		EmulatedCFO_->SendRequestsForRange(
// 		    number,
// 		    DTCLib::DTC_EventWindowTag(static_cast<uint64_t>(timestampStart)),
// 		    incrementTimestamp,
// 		    cfodelay,
// 		    requestsAhead,
// 		    heartbeatPackets);

// 		// auto readoutRequestTime = device->GetDeviceTime();
// 		device->ResetDeviceTime();
// 		// auto afterRequests = std::chrono::steady_clock::now();

// 		// print out stuff
// 		unsigned quietCount = 20;
// 		quiet               = false;

// 		std::stringstream ostr;
// 		ostr << std::endl;

// 		for(unsigned ii = 0; ii < number; ++ii)
// 		{
// 			__FE_COUT__ << "Buffer Read " << std::dec << ii << std::endl;
// 			mu2e_databuff_t* buffer;
// 			auto             tmo_ms = 1500;
// 			__FE_COUT__ << "util - before read for DAQ - ii=" << ii;
// 			auto sts = device->read_data(
// 			    DTC_DMA_Engine_DAQ, reinterpret_cast<void**>(&buffer), tmo_ms);
// 			__FE_COUT__ << "util - after read for DAQ - ii=" << ii << ", sts=" << sts
// 			            << ", buffer=" << (void*)buffer;

// 			if(sts > 0)
// 			{
// 				void* readPtr = &buffer[0];
// 				auto  bufSize = static_cast<uint16_t>(*static_cast<uint64_t*>(readPtr));
// 				readPtr       = static_cast<uint8_t*>(readPtr) + 8;

// 				std::cout << "Buffer reports DMA size of " << std::dec << bufSize
// 				          << " bytes. Device driver reports read of " << sts << " bytes,"
// 				          << std::endl;

// 				std::cout << "util - bufSize is " << bufSize;
// 				//	__SET_ARG_OUT__("bufSize", bufSize);

// 				//	__FE_COUT__ << "bufSize" << bufSize;
// 				outputStream.write(static_cast<char*>(readPtr), sts - 8);

// 				auto maxLine = static_cast<unsigned>(ceil((sts - 8) / 16.0));
// 				std::cout << "maxLine " << maxLine;
// 				for(unsigned line = 0; line < maxLine; ++line)
// 				{
// 					ostr << "0x" << std::hex << std::setw(5) << std::setfill('0') << line
// 					     << "0: ";
// 					for(unsigned byte = 0; byte < 8; ++byte)
// 					{
// 						if(line * 16 + 2 * byte < sts - 8u)
// 						{
// 							auto thisWord =
// 							    reinterpret_cast<uint16_t*>(buffer)[4 + line * 8 + byte];
// 							ostr << std::setw(4) << static_cast<int>(thisWord) << " ";
// 						}
// 					}

// 					ostr << std::endl;
// 					//	std::cout << ostr.str();

// 					//__SET_ARG_OUT__("readData", ostr.str());  // write to data file

// 					__FE_COUT__ << ostr.str();  // write to log file

// 					if(maxLine > quietCount * 2 && quiet && line == (quietCount - 1))
// 					{
// 						line = static_cast<unsigned>(ceil((sts - 8) / 16.0)) -
// 						       (1 + quietCount);
// 					}
// 				}
// 			}
// 			device->read_release(DTC_DMA_Engine_DAQ, 1);
// 		}
// 		ostr << std::endl;

// 		__SET_ARG_OUT__("readData", ostr.str());  // write to data file

// 		__FE_COUT__ << ostr.str();                // write to log file

// 		delete EmulatedCFO_;
// 	}
// 	else
// 	{
// 		__FE_SS__ << "Error! DTC must be in CFOEmulate mode" << __E__;
// 		__FE_SS_THROW__;
// 	}

// }  // end DTCSendHeartbeatAndDataRequest()

//========================================================================
void DTCFrontEndInterface::ResetLossOfLockCounter(__ARGS__)
{
	// write anything to reset
	// 0x93c8 is RX CDR Unlock counter (32-bit)
	thisDTC_->ClearRXCDRUnlockCount(DTCLib::DTC_Link_ID::DTC_Link_CFO);
	// registerWrite(0x93c8, 0);

	// now check
	// uint32_t readData = registerRead(0x93c8);

	// char readDataStr[100];
	// sprintf(readDataStr, "%d", readData);
	// __SET_ARG_OUT__("Upstream Rx Lock Loss Count", readDataStr);
	__SET_ARG_OUT__("Upstream Rx Lock Loss Count", 
		thisDTC_->FormatRXCDRUnlockCountCFOLink());
}  // end ResetLossOfLockCounter()

//========================================================================
void DTCFrontEndInterface::ReadLossOfLockCounter(__ARGS__)
{
	// 0x93c8 is RX CDR Unlock counter (32-bit)
	uint32_t readData = //registerRead(0x93c8);
		thisDTC_->ReadRXCDRUnlockCount(DTCLib::DTC_Link_ID::DTC_Link_CFO);

	char readDataStr[100];
	sprintf(readDataStr, "%d", readData);

	// 0x9140 bit-6 is RX CDR is locked

	bool isUpstreamLocked = 1;
	for(int i = 0; i < 5; ++i) //read 5x for multiple samples in case of instability
	{
		isUpstreamLocked &= thisDTC_->ReadSERDESRXCDRLock(DTCLib::DTC_Link_ID::DTC_Link_CFO);
		// readData = registerRead(0x9140);
		// isUpstreamLocked &=
		//     (readData >> 6) & 1;  //& to force unlocked for any unlocked reading
	}
	//__SET_ARG_OUT__("Upstream Rx CDR Lock Status",isUpstreamLocked?"LOCKED":"Not
	//Locked");

	// 0x9128 bit-6 is RX PLL
	// readData                 = registerRead(0x9128);
	bool isUpstreamPLLLocked = //(readData >> 6) & 1;
		thisDTC_->ReadSERDESPLLLocked(DTCLib::DTC_Link_ID::DTC_Link_CFO);

	// Jitter attenuator has configurable "Free Running" mode
	// LOL == Loss of Lock, LOS == Loss of Signal (4-inputs to jitter attenuator)
	// 0x9308 bit-0 is reset, input select bit-5:4, bit-8 is LOL, bit-11:9 (input LOS)
	// readData          = //registerRead(0x9308);
		
	uint32_t    val   = thisDTC_->ReadJitterAttenuatorSelect().to_ulong();//(readData >> 4) & 3;
	std::string JAsrc = val == 0 ? "from CFO" : (val == 1 ? "from RJ45" : "from FMC/SFP+");

	__SET_ARG_OUT__(
	    "Upstream Rx Lock Loss Count",
	    std::string(readDataStr) +
	        "... CDR = " + std::string(isUpstreamLocked ? " LOCKED" : " Not Locked") +
	        "... PLL = " + std::string(isUpstreamPLLLocked ? " LOCKED" : " Not Locked") +
	        "... JA = " + JAsrc);

}  // end ReadLossOfLockCounter()

//========================================================================
void DTCFrontEndInterface::GetLinkLockStatus(__ARGS__) 
{ 
	std::stringstream outss;
	outss << thisDTC_->FormatRXCDRLockStatus() << "\n\n" << thisDTC_->FormatLinkEnable();
	__SET_ARG_OUT__("Lock Status", "\n" + outss.str()); 
}  // end GetLinkLockStatus()

// //========================================================================
// void DTCFrontEndInterface::SelectJitterAttenuatorSource(__ARGS__)
// {
// 	uint32_t select = __GET_ARG_IN__(
// 	    "Source Clock (0 is from CFO, 1 is from RJ45)", uint32_t);
// 	select %= 4;
// 	__FE_COUTV__((unsigned int)select);

// 	// choose jitter attenuator input select (reg 0x9308, bits 5:4)
// 	//  0 is Upstream Control Link Rx Recovered Clock
// 	//  1 is RJ45 Upstream Clock
// 	//  2 is Timing Card Selectable (SFP+ or FPGA) Input Clock

// 	// char              reg_0x9308[100];
// 	// uint32_t          val = registerRead(0x9308);
// 	// std::stringstream results;

// 	// sprintf(reg_0x9308, "0x%8.8X", val);
// 	// __FE_COUTV__(reg_0x9308);
// 	// results << "reg_0x9118 Starting Value: " << reg_0x9308 << __E__;

// 	// val &= ~(3 << 4);          // clear the two bits
// 	// val &= ~(1);               // ensure unreset of jitter attenuator
// 	// val |= (select & 3) << 4;  // set the two bits to selected value

// 	// sprintf(reg_0x9308, "0x%8.8X", val);
// 	// __FE_COUTV__(reg_0x9308);
// 	// results << "reg_0x9118 Select Value: " << reg_0x9308 << __E__;

// 	// registerWrite(0x9308, val);  // write select value

// 	if(!__GET_ARG_IN__(
// 	    "DoNotSet", bool))
// 	{
// 		bool alsoResetJA = __GET_ARG_IN__(
// 	    		"AlsoResetJA", bool);
// 		__FE_COUTV__(alsoResetJA);
// 		thisDTC_->SetJitterAttenuatorSelect(select, alsoResetJA);
// 		sleep(1);
// 		for(int i=0;i<10;++i) //wait for JA to lock before reading
// 		{
// 			if(thisDTC_->ReadJitterAttenuatorLocked())
// 				break;
// 			sleep(1);
// 		}
// 	}
// 	__FE_COUT__ << "Done with jitter attenuator source select: " << select << __E__;

// 	__SET_ARG_OUT__("Register Write Results", thisDTC_->FormatJitterAttenuatorCSR());	

// }  // end SelectJitterAttenuatorSource()

//========================================================================
void DTCFrontEndInterface::WriteDTC(__ARGS__)
{
	uint32_t address   = __GET_ARG_IN__("address", uint32_t);
	uint32_t writeData = __GET_ARG_IN__("writeData", uint32_t);
	__FE_COUTV__((unsigned int)address);
	__FE_COUTV__((unsigned int)writeData);

	int errorCode = getDevice()->write_register( address, 100, writeData);
	if (errorCode != 0)
	{
		__FE_SS__ << "Error writing register 0x" << std::hex << std::setfill('0') << std::setw(4) << address << ". Error code = " << errorCode;
		__SS_THROW__;
	}

	std::stringstream ss;
	ss << "Wrote " << std::dec << writeData << " 0x" << std::hex << std::setfill('0') << std::setw(8) << writeData << 
		" to address 0x" << std::setw(4) << address << ".";
	__SET_ARG_OUT__("Status", ss.str());  // readDataStr);

}  // end WriteDTC()

//========================================================================
void DTCFrontEndInterface::ReadDTC(__ARGS__)
{
	uint32_t address = __GET_ARG_IN__("address", uint32_t);
	__FE_COUTV__((unsigned int)address);
	uint32_t readData;// = registerRead(address);

	int errorCode = getDevice()->read_register(address, 100, &readData);
	if (errorCode != 0)
	{
		__FE_SS__ << "Error reading register 0x" << std::hex << address << " " << errorCode;
		__SS_THROW__;
	}	

	// char readDataStr[100];
	// sprintf(readDataStr,"0x%X",readData);
	// converted to dec and hex display in FEVInterfacesManager handling of FE Macros
	std::stringstream ss;
	ss << "Read " << std::dec << readData << " 0x" << std::hex << std::setfill('0') << std::setw(8) << readData << 
		" from address 0x" << std::setw(4) << address << ".";
	__SET_ARG_OUT__("readData", ss.str());  // readDataStr);
}  // end ReadDTC()

// //========================================================================
// void DTCFrontEndInterface::DTCSoftReset()
// {
// 	__FE_COUT__ << "Starting DTC Reset: reset the DTC FPGA and SERDES" << __E__;

// 	// 0x9100 is the DTC Control Register
// 	thisDTC_->SoftReset();
// 	// registerWrite(0x9100, //registerRead(0x9100) |
// 	//  	(1 << 31));  // bit 31 = DTC Reset FPGA

// 	// usleep(500000 /*500ms*/);
// 	// sleep(3);

// 	// reset DTC serdes osc
// 	// thisDTC_->SetCFOEmulationMode(); //not needed after configureHardwareDevMode 



// 	// registerWrite(0x9100, //registerRead(0x9100) | 
// 	// 	(1 << 15));  // Turn on CFO Emulation Mode for Serdes Reset
// 	// registerWrite(0x9118, 0xffff00ff);      // SERDES resets
// 	// registerWrite(0x9118, 0x00000000);      // clear SERDES resets
	
// 	//Rick says he never resets links unless there is a problem:
// 	// thisDTC_->ResetSERDESTX(DTCLib::DTC_Link_ID::DTC_Link_ALL);
// 	// thisDTC_->ResetSERDESRX(DTCLib::DTC_Link_ID::DTC_Link_ALL);
// 	// thisDTC_->ResetSERDES(DTCLib::DTC_Link_ID::DTC_Link_ALL);
// 	//there is also bit-8 of the control register, that resets all the links


// 	// if(emulate_cfo_ == 1)
// 	// {
// 	// 	// registerWrite(0x9100, registerRead(0x9100) & ~(1 << 15)); // Turn off CFO Emulation Mode 
// 	// 	thisDTC_->ClearCFOEmulationMode();
// 	// }

// 	// usleep(500000 /*500ms*/);
// 	// sleep(2);

// 	__FE_COUT__ << "Done with DTC Reset." << __E__;
// }  // end DTCSoftReset()

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

	__FE_COUTV__(rocUID);
	__FE_COUTV__(rocFEMacroName);

	auto rocIt = rocs_.find(rocUID);
	if(rocIt == rocs_.end())
	{
		__FE_SS__ << "Fatal error - ROC name '" << rocUID << "' not found in DTC's map!"
		          << __E__;
		__FE_SS_THROW__;
	}

	rocIt->second->runSelfFrontEndMacro(rocFEMacroName, argsIn, argsOut);

}  // end RunROCFEMacro()

//========================================================================
void DTCFrontEndInterface::SetupROCs(__ARGS__)
{
	DTCLib::DTC_Link_ID rocLinkIndex =
	    DTCLib::DTC_Link_ID(__GET_ARG_IN__("Target ROC (Default = -1 := all ROCs)", uint8_t, -1 /* ALL */));
	__FE_COUTV__(rocLinkIndex);

	bool rocRxTxEnable = __GET_ARG_IN__("Set Link RX/TX Enable (Default := false)", bool, false);
	__FE_COUTV__(rocRxTxEnable);
	bool rocEmulationEnable = __GET_ARG_IN__("Set ROC Emulation Enable (Default := false)", bool, false);
	
	__FE_COUTV__(rocEmulationEnable);


	if(rocRxTxEnable)
		thisDTC_->EnableLink(rocLinkIndex);
	else
		thisDTC_->DisableLink(rocLinkIndex);

	DTCLib::DTC_ROC_Emulation_Type rocEmulationType =
	    DTCLib::DTC_ROC_Emulation_Type(__GET_ARG_IN__("ROC Emulation Type (Default = 0: Internal, 1: Fiber-Loopback, 2: External)", uint8_t, 0 /* internal */));
	__FE_COUTV__(rocEmulationType);

	if(rocEmulationEnable)
		thisDTC_->EnableROCEmulator(rocLinkIndex, rocEmulationType);
	else
		thisDTC_->DisableROCEmulator(rocLinkIndex, rocEmulationType);

	// To change the size of the event, need to write to each ROC emulator
	// 0x91B0 (b0-10 roc0, b16-26 roc1), 0x91B4 (b0-10 roc2, b16-26 roc3), 0x91B8 (b0-10
	// roc4, b16-26 roc5)

	uint32_t size  = __GET_ARG_IN__("ROC generated Data Payload fragment packet count (11-bits, Default := 16)", uint32_t, 16);
	uint32_t wsize = size & 0x03FF;  // only allow 11 bits

	__FE_COUTV__((unsigned int)size);
	__FE_COUTV__((unsigned int)wsize);
	if(size != wsize)
	{
		__FE_SS__ << "Out-of-ranage write emulated ROC Data Payload packet count specified: " << size << ". The field size is 11-bits." << __E__;
		__FE_SS_THROW__;
	}

	thisDTC_->SetCFOEmulationNumPackets(rocLinkIndex,wsize);
	
	__SET_ARG_OUT__("Result", thisDTC_->FormattedRegDump(20,thisDTC_->formattedROCEmulationFunctions_));

}  // end SetEmulatedROCEventFragmentSize()

//========================================================================
void DTCFrontEndInterface::configureHardwareDevMode(__ARGS__)
{
	configureHardwareDevMode();
} // end configureHardwareDevMode()

// //========================================================================
// void DTCFrontEndInterface::HeaderFormatTest(__ARGS__) 
// {
// 	__FE_COUT__ << "Operation \"heder_format\"" << std::endl;

// 	std::stringstream ostr;
// 	ostr << std::endl;

// 	auto device = thisDTC_->GetDevice();

// 	device->write_register(37316, 100, 170);	// 0x91c4 <- 0xAA
// 	TLOG(TLVL_ERROR) << "Write " << std::hex << 170 << " in 0x91C4" << std::endl;
// 	ostr << "Write " << std::hex << 170 << " in 0x91C4" << std::endl;

// 	device->write_register(37312, 100, 47802);	// 0x91c0 <- 0xBABA
// 	TLOG(TLVL_ERROR) << "Write " << std::hex << 47802 << " in 0x91C0" << std::endl;
// 	ostr << "Write " << std::hex << 47802 << " in 0x91C0" << std::endl;

// 	device->write_register(37204, 100, 205);	// 0x9154 <- 0xCD
// 	TLOG(TLVL_ERROR) << "Write " << std::hex << 205 << " in 0x9154" << std::endl;
// 	ostr << "Write " << std::hex << 205 << " in 0x9154" << std::endl;

// 	std::string outStr = "Test packet headers \n" + ostr.str();
// 	__SET_ARG_OUT__("setRegister", outStr);
// }

//========================================================================
void DTCFrontEndInterface::DTCCounters(__ARGS__)
{	
	__SET_ARG_OUT__("Link Counters", thisDTC_->FormattedRegDump(20, thisDTC_->formattedSERDESCounterFunctions_));
	__SET_ARG_OUT__("Performance Counters", thisDTC_->FormattedRegDump(20, thisDTC_->formattedPerformanceCounterFunctions_));
	__SET_ARG_OUT__("Packet Counters", thisDTC_->FormattedRegDump(20, thisDTC_->formattedPacketCounterFunctions_));
}  // end DTCCounters()

// //========================================================================
// void DTCFrontEndInterface::FlashLEDs(__ARGS__)
// {	
// 	thisDTC_->FlashLEDs();
// } //end FlashLEDs()

// //==============================================================================
// // GetFirmwareVersion
//  void DTCFrontEndInterface::GetFirmwareVersion(__ARGS__)
// {	
// 	__SET_ARG_OUT__("Firmware Version Date", thisDTC_->ReadDesignDate());
// }  // end GetFirmwareVersion()

// //========================================================================
// void DTCFrontEndInterface::GetStatus(__ARGS__)
// {	
// 	__SET_ARG_OUT__("Status", thisDTC_->FormattedRegDump(20, thisDTC_->formattedDumpFunctions_));
// } //end GetStatus()

// //========================================================================
// void DTCFrontEndInterface::GetSimpleStatus(__ARGS__)
// {	
// 	__SET_ARG_OUT__("Status", thisDTC_->FormattedRegDump(20, thisDTC_->formattedSimpleDumpFunctions_));
// } //end GetSimpleStatus()

//========================================================================
void DTCFrontEndInterface::readRxDiagFIFO(__ARGS__)
{	
	DTCLib::DTC_Link_ID LinkIndex =
	    DTCLib::DTC_Link_ID(__GET_ARG_IN__("LinkIndex", uint8_t));

	__SET_ARG_OUT__("Diagnostic RX FIFO", thisDTC_->FormatRXDiagFifo(DTCLib::DTC_ROC_Links[LinkIndex]));

	
} //end readRxDiagFIFO()

//========================================================================
void DTCFrontEndInterface::readTxDiagFIFO(__ARGS__)
{	
	DTCLib::DTC_Link_ID LinkIndex =
	    DTCLib::DTC_Link_ID(__GET_ARG_IN__("LinkIndex", uint8_t));

	__SET_ARG_OUT__("Diagnostic TX FIFO", thisDTC_->FormatTXDiagFifo(DTCLib::DTC_ROC_Links[LinkIndex]));

	
} //end readTxDiagFIFO()

//========================================================================
void DTCFrontEndInterface::GetLinkErrors(__ARGS__)
{	
	__SET_ARG_OUT__("Link Errors", thisDTC_->FormattedRegDump(20, thisDTC_->formattedSERDESErrorFunctions_));
} //end GetLinkErrors()

// //========================================================================
// void DTCFrontEndInterface::ROCDestroy(__ARGS__)
// {	
	
// 	rocs_.clear();

// } //end ROCDestroy()
// //========================================================================
// void DTCFrontEndInterface::ROCInstantiate(__ARGS__)
// {	
// 	createROCs();
	
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
	
	unsigned dtc_class_roc_mask = 0;
	// create roc mask for DTC
	{
		std::vector<std::pair<std::string, ConfigurationTree>> rocChildren =
		    Configurable::getSelfNode().getNode("LinkToROCGroupTable").getChildren();

		__FE_COUTV__(rocChildren.size());
		roc_mask_ = 0;

		for(auto& roc : rocChildren)
		{
			__FE_COUT__ << "roc uid " << roc.first << __E__;
			bool enabled = roc.second.getNode("Status").getValue<bool>();
			__FE_COUT__ << "roc enable " << enabled << __E__;

			if(enabled)
			{
				int linkID = roc.second.getNode("linkID").getValue<int>();
				roc_mask_ |= (0x1 << linkID);
				dtc_class_roc_mask |=
				    (0x1 << (linkID * 4));  // the DTC class instantiation expects each
				                            // ROC has its own hex nibble
			}
		}

		__FE_COUT__ << "DTC roc_mask_ = 0x" << std::hex << roc_mask_ << std::dec << __E__;
		__FE_COUT__ << "roc_mask to instantiate DTC class = 0x" << std::hex
		            << dtc_class_roc_mask << std::dec << __E__;

	}  // end create roc mask

	// instantiate DTC with the appropriate ROCs enabled
	// DTC firmware design version must match ReadDesignDate()_ReadVivadoVersion() [ ignoring ReadDesignVersionNumber() for now ]
	// for example the string might be "Jun/13/2023 16:00   raw-data: 0x23061316" + "_22.1"
	std::string         expectedDesignVersion = "";
	try
	{
		expectedDesignVersion = getSelfNode().getNode("ExpectedFirmwareVersion").getValue(); 
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

	thisDTC_ = new DTCLib::DTC(
	    mode, deviceIndex_, dtc_class_roc_mask, expectedDesignVersion, 
		true /* skipInit */, //always skip init and lots ots configure setup
		"" /* simMemoryFile */,
		getInterfaceUID()); 
	
	try //attempt to print out firmware version to the log
	{
		std::string designVersion = thisDTC_->ReadDesignVersion();
		__FE_COUTV__(designVersion);
	} catch (...) {} //hide exception to finish instantiation (likely exception is from a need to reset PCIe)
	__FE_COUT__ << "Linux Kernel Driver Version: " << thisDTC_->GetDevice()->get_driver_version() << __E__;

	createROCs();
	registerFEMacros();

	// DTC-specific info
	dtc_location_in_chain_ =
	    getSelfNode().getNode("LocationInChain").getValue<unsigned int>();
	
	__FE_COUT_INFO__ << "DTC instantiated with name: " << getInterfaceUID()
	                 << " dtc_location_in_chain_ = " << dtc_location_in_chain_
	                 << " talking to /dev/mu2e" << deviceIndex_ << __E__;

} //end DTCInstantiate()

//========================================================================
void DTCFrontEndInterface::ResetDTCLinks(__ARGS__)
{	
	thisDTC_->ResetSERDESTX(DTCLib::DTC_Link_ID::DTC_Link_ALL);
	thisDTC_->ResetSERDESRX(DTCLib::DTC_Link_ID::DTC_Link_ALL);
	thisDTC_->ResetSERDES(DTCLib::DTC_Link_ID::DTC_Link_ALL);
} //end ResetDTCLinks()

//========================================================================
void DTCFrontEndInterface::ConfigureForTimingChain(__ARGS__)
{	
	//call virtual readStatus

	int stepIndex = __GET_ARG_IN__("StepIndex", int);

	// do 0, then 1
	if(stepIndex == -1)
	{
		for(int i=0;i<CONFIG_DTC_TIMING_CHAIN_STEPS;++i)
		{
			configureForTimingChain(i);
			usleep(1000);
		}
	}
	else
		configureForTimingChain(stepIndex);
	

} //end ConfigureForTimingChain()

// //========================================================================
// void DTCFrontEndInterface::ResetPCIe(__ARGS__)
// {	
// 	thisDTC_->ResetPCIe();

// 	__SET_ARG_OUT__("Status", "Done");
// } //end ResetPCIe()

// //========================================================================
// void DTCFrontEndInterface::GetFPGATemperature(__ARGS__)
// {	
// 	// rd << "Celsius: " << val << ", Fahrenheit: " << val*9/5 + 32 << ", " << (val < 65?"GOOD":"BAD");
// 	std::stringstream ss;
// 	ss << "Note: temperatures of 65C or higher should be addressed by DAQ experts.\n\n";
// 	ss << thisDTC_->FormatFPGATemperature() << "\n\n" << thisDTC_->FormatFPGAAlarms();
// 	__SET_ARG_OUT__("Temperature", ss.str());
// } //end GetFPGATemperature()

// //========================================================================
// void DTCFrontEndInterface::GetFireflyTemperature(__ARGS__)
// {	
// 	std::stringstream rd;

// 	// // #Read Firefly RX temp registers
// 	// // #enable IIC on Firefly
// 	// // my_cntl write 0x93a0 0x00000200
// 	// registerWrite(0x93a0,0x00000200);
// 	// // #Device address, register address, null, null
// 	// // my_cntl write 0x9298 0x54160000
// 	// registerWrite(0x9298,0x54160000);
// 	// // #read enable
// 	// // my_cntl write 0x929c 0x00000002
// 	// registerWrite(0x929c,0x00000002);
// 	// // #disable IIC on Firefly
// 	// // my_cntl write 0x93a0 0x00000000
// 	// registerWrite(0x93a0,0x00000000);
// 	// // #read data: Device address, register address, null, temp in 2's compl.
// 	// // my_cntl read 0x9298

// 	rd << "Note: temperatures of 65C or higher should be addressed by DAQ experts.\n\n";

// 	thisDTC_->SetTXRXFireflySelect(true);
// 	auto val = thisDTC_->ReadFireflyRXIICInterface(0x54 /*device*/, 0x16 /*address*/);
// 	thisDTC_->SetTXRXFireflySelect(false);	


// 	// dtc_data_t val = registerRead(0x9298) & 0x0FF;
// 	rd << std::fixed << std::setprecision(1) << uint16_t(val) << ".0 C, " << double(val)*9/5 + 32 << " F, <65C=" << (val < 65?"GOOD":"BAD") <<
// 		"\n\n" << thisDTC_->FormatFPGAAlarms();

// 	__SET_ARG_OUT__("Temperature",rd.str());
// } //end GetFireflyTemperature()

//========================================================================
void DTCFrontEndInterface::ResetCFOLinkRx(__ARGS__)
{	
	thisDTC_->ResetSERDESRX(DTCLib::DTC_Link_ID::DTC_Link_CFO);
} //end ResetCFOLinkRx()
//========================================================================
void DTCFrontEndInterface::ResetCFOLinkTx(__ARGS__)
{	
	thisDTC_->ResetSERDESTX(DTCLib::DTC_Link_ID::DTC_Link_CFO);
} //end ReseResetCFOLinkTxtCFORx()
//========================================================================
void DTCFrontEndInterface::ResetCFOLinkRxPLL(__ARGS__)
{	
	thisDTC_->ResetSERDESPLL(DTCLib::DTC_PLL_ID::DTC_PLL_CFO_RX);
} //end ResetCFOLinkRxPLL()
//========================================================================
void DTCFrontEndInterface::ResetCFOLinkTxPLL(__ARGS__)
{	
	thisDTC_->ResetSERDESPLL(DTCLib::DTC_PLL_ID::DTC_PLL_CFO_TX);
} //end ResetCFOLinkTxPLL()

// //========================================================================
// void DTCFrontEndInterface::GetLinkLossOfLight(__ARGS__)
// {	
// 	std::stringstream rd;



// 	//do initial set of writes to get the live read of loss-of-light status (because it is latched value from last read)
// /*
// 	// #Read Firefly RX LOS registers
// 	// #enable IIC on Firefly
// 	// my_cntl write 0x93a0 0x00000200
// 	registerWrite(0x93a0,0x00000200);
// 	// #Device address, register address, null, null
// 	// my_cntl write 0x9298 0x54080000
// 	registerWrite(0x9298,0x54080000);
// 	// #read enable
// 	// my_cntl write 0x929c 0x00000002
// 	registerWrite(0x929c,0x00000002);
// 	// #disable IIC on Firefly
// 	// my_cntl write 0x93a0 0x00000000
// 	registerWrite(0x93a0,0x00000000);
// 	// #read data: Device address, register address, null, value
// 	// my_cntl read 0x9298
// */
// 	thisDTC_->SetTXRXFireflySelect(true);
// 	thisDTC_->WriteFireflyRXIICInterface(0x54 /*device*/, 0x08 /*address*/, 0 /*data*/);
// 	thisDTC_->SetTXRXFireflySelect(false);

// 	// #{EVB, ROC4, ROC1, CFO, unused, ROC5, unused, unused}
// 	usleep(1000*100);

// /*
// 	// #Read Firefly RX LOS registers
// 	// my_cntl write 0x93a0 0x00000200
// 	registerWrite(0x93a0,0x00000200);
// 	// my_cntl write 0x9298 0x54070000
// 	registerWrite(0x9298,0x54070000);
// 	// my_cntl write 0x929c 0x00000002
// 	registerWrite(0x929c,0x00000002);
// 	// my_cntl write 0x93a0 0x00000000
// 	registerWrite(0x93a0,0x00000000);
// 	// my_cntl read 0x9298
// */

// 	thisDTC_->SetTXRXFireflySelect(true);
// 	thisDTC_->WriteFireflyRXIICInterface(0x54 /*device*/, 0x07 /*address*/, 0 /*data*/);
// 	thisDTC_->SetTXRXFireflySelect(false);

// 	//END do initial set of writes to get the live read of loss-of-light status (because it is latched value from last read)

// 	dtc_data_t val=0, val2=0, tmpVal;
// 	for(int i=0;i<5;++i)
// 	{
// 		usleep(1000*100 /* 100 ms */);
// 		thisDTC_->SetTXRXFireflySelect(true);
// 		//OR := if ever 1, mark dead
// 		tmpVal = thisDTC_->ReadFireflyRXIICInterface(0x54 /*device*/, 0x08 /*address*/); 
// 		val |= tmpVal;//thisDTC_->ReadFireflyRXIICInterface(0x54 /*device*/, 0x08 /*address*/);
// 		__COUT__ << "0x08 ==> " << std::hex << " OrVal = 0x" << val << " readval = 0x" << tmpVal << __E__;
// 		thisDTC_->SetTXRXFireflySelect(false);
			
		
// 		usleep(1000*100 /* 100 ms */);
// 		thisDTC_->SetTXRXFireflySelect(true);
// 		//OR := if ever 1, mark dead
// 		tmpVal = thisDTC_->ReadFireflyRXIICInterface(0x54 /*device*/, 0x07 /*address*/);
// 		val2 |= tmpVal;
// 		__COUT__ << "0x07 ==> " << std::hex << " OrVal = 0x" << val2 << " readval = 0x" << tmpVal << __E__;
// 		thisDTC_->SetTXRXFireflySelect(false);
// 	} //end multi-read to check for strange value changing

// 	// #ROC0 bit 3
// 	rd << "{0:" << (((val2>>(0+3))&1)?"DEAD":"OK");
// 	// #ROC1 bit 5
// 	rd << ", 1: " << (((val>>(0+5))&1)?"DEAD":"OK");
// 	// #ROC2 bit 2
// 	rd << ", 2:" << (((val2>>(0+2))&1)?"DEAD":"OK");
// 	// #ROC3 bit 0
// 	rd << ", 3:" << (((val2>>(0+0))&1)?"DEAD":"OK");
// 	// #ROC4 bit 6
// 	rd << ", 4: " << (((val>>(0+6))&1)?"DEAD":"OK");
// 	// #ROC5 bit 1
// 	rd << ", 5: " << (((val>>(0+1))&1)?"DEAD":"OK");
// 	// #CFO bit 4
// 	rd << ", 6/CFO: " << (((val>>(0+4))&1)?"DEAD":"OK");
// 	// #EVB bit 7  Are EVB and CFO reversed?
// 	rd << ", 7/EVB: " << (((val>>(0+7))&1)?"DEAD":"OK") << "}";



// 	__SET_ARG_OUT__("Link Status",rd.str());
// } //end GetLinkLossOfLight()

//========================================================================
void DTCFrontEndInterface::SetupCFOInterface(__ARGS__)
{	
	if(__GET_ARG_IN__("Put DTC in CFO Emulation Mode (Default := false)",bool,false))
		thisDTC_->SetCFOEmulationMode();
	else 
		thisDTC_->ClearCFOEmulationMode();


	bool cfoRxTxEnable = __GET_ARG_IN__("Set Link RX/TX Enable (Default := false)", bool, false);
	__FE_COUTV__(cfoRxTxEnable);

	if(cfoRxTxEnable)
	{
		thisDTC_->EnableReceiveCFOLink();
		thisDTC_->EnableTransmitCFOLink();
	}
	else
	{
		thisDTC_->DisableReceiveCFOLink();
		thisDTC_->DisableTransmitCFOLink();
	}

	if(__GET_ARG_IN__("Enable Auto-generation of Data Request Packets (Default := false)",bool,false))
		thisDTC_->EnableAutogenDRP();
	else
		thisDTC_->DisableAutogenDRP();

} //end SetupCFOInterface()

//========================================================================
void DTCFrontEndInterface::SetCFOEmulatorOnOffSpillEmulation(__ARGS__)
{	
	bool enable = (__GET_ARG_IN__("Enable CFO Emulator (Default := false)",bool,false));
	__FE_COUTV__(enable);

	std::stringstream outSs;

	thisDTC_->DisableCFOEmulation();
	if(!enable) //do not need to apply parameters if disabling		
	{
		outSs << "Halted CFO Emulator!" << __E__;
		__SET_ARG_OUT__("response", outSs.str());
		return;
	}
	//else enabling, so apply parameters, then enable

	thisDTC_->SoftReset(); //to reset event window tag starting point handling

	bool useDetachedBufferTest = __GET_ARG_IN__("Use Detached Buffer Test (Default := false)",uint32_t);

	// release of all the buffers
	if(!useDetachedBufferTest)
		getDevice()->read_release(DTC_DMA_Engine_DAQ, 100);

	//If Event Window duration = 0, this specifies to execute the On/Off Spill emulation of Event Window intervals.
	thisDTC_->SetCFOEmulationHeartbeatInterval(0);  

	uint32_t numberOfSuperCycles = __GET_ARG_IN__("Number of 1.4s super cycle repetitions (0 := infinite)",uint32_t);
	__FE_COUTV__(numberOfSuperCycles);
	thisDTC_->SetCFOEmulationNumHeartbeats(numberOfSuperCycles);
	
	uint64_t initialEventWindowTag = __GET_ARG_IN__("Starting Event Window Tag",uint64_t);
	__FE_COUTV__(initialEventWindowTag);
	thisDTC_->SetCFOEmulationTimestamp(DTCLib::DTC_EventWindowTag(initialEventWindowTag));

	bool enableClockMarkers = __GET_ARG_IN__("Enable Clock Markers (Default := false)",bool,false);
	__FE_COUTV__(enableClockMarkers);
	thisDTC_->SetCFOEmulation40MHzClockMarkerEnable(DTCLib::DTC_Link_ID::DTC_Link_ALL,enableClockMarkers);

	if(__GET_ARG_IN__("Enable Auto-generation of Data Request Packets (Default := false)",bool,false))
		thisDTC_->EnableAutogenDRP();
	else
		thisDTC_->DisableAutogenDRP();

	thisDTC_->EnableReceiveCFOLink(); //enable forwarding if CFO timing link to ROCs


	if(useDetachedBufferTest)
	{
		bool saveBinaryDataToFile = __GET_ARG_IN__("For Detached Buffer Test, Save Binary Data to File (Default: false)", bool);
		__FE_COUTV__(saveBinaryDataToFile);
		bool doNotResetCounters = __GET_ARG_IN__("For Detached Buffer Test, Do NOT Reset Counters (Default: false)", bool);
		__FE_COUTV__(doNotResetCounters);
		__FE_COUT__ << "Initializing detached buffer test!" << __E__;

		if(!bufferTestThreadStruct_) //initialize shared pointer for first time
			bufferTestThreadStruct_ = std::make_shared<DTCFrontEndInterface::DetachedBufferTestThreadStruct>();

		if(bufferTestThreadStruct_->running_)
		{
			__FE_COUT__ << "Found buffer test thread already running... so re-initializing" << __E__;

			// start mutex scope
			{
				std::lock_guard<std::mutex> lock(bufferTestThreadStruct_->lock_);
				bufferTestThreadStruct_->inSubeventMode_ 		= true;
				bufferTestThreadStruct_->activeMatch_ 			= false;
				bufferTestThreadStruct_->expectedEventTag_ 		= initialEventWindowTag;
				bufferTestThreadStruct_->saveBinaryData_ 		= saveBinaryDataToFile;
				bufferTestThreadStruct_->exitThread_ 			= false;
				bufferTestThreadStruct_->resetStartEventTag_ 	= true;
				bufferTestThreadStruct_->doNotResetCounters_ 	= doNotResetCounters;
			}
			outSs << "Found buffer test thread already running... so re-initializing and reading data starting at event tag " << initialEventWindowTag << 
				" (0x" << std::hex << initialEventWindowTag << ")" << __E__;
		}
		else
		{
			__FE_COUT__  << "Launching detached Buffer Test thread..." << __E__;
			// start mutex scope
			{
				std::lock_guard<std::mutex> lock(bufferTestThreadStruct_->lock_);
				bufferTestThreadStruct_->inSubeventMode_ 		= true;
				bufferTestThreadStruct_->activeMatch_ 			= false;
				bufferTestThreadStruct_->expectedEventTag_ 		= initialEventWindowTag;
				bufferTestThreadStruct_->saveBinaryData_ 		= saveBinaryDataToFile;
				bufferTestThreadStruct_->exitThread_ 			= false;
				bufferTestThreadStruct_->resetStartEventTag_ 	= false;
				bufferTestThreadStruct_->thisDTC_				= thisDTC_;
				bufferTestThreadStruct_->running_ 				= true;	
				bufferTestThreadStruct_->doNotResetCounters_ 	= false;
			}
			std::thread([](std::shared_ptr<DTCFrontEndInterface::DetachedBufferTestThreadStruct> threadStruct) { 
						DTCFrontEndInterface::detechedBufferTestThread(threadStruct); },
					bufferTestThreadStruct_)
			.detach();
			outSs << "Launched detached Buffer Test thread and reading data DMA-0 starting at event tag " << initialEventWindowTag << 
				" (0x" << std::hex << initialEventWindowTag << ")" << __E__;
		}
		usleep(100); //give time for buffer reading to be ready		
	}

	thisDTC_->EnableCFOEmulation();

	outSs << "Launched CFO Emulator!" << __E__;
	__SET_ARG_OUT__("response", outSs.str());
	
} //end SetCFOEmulatorOnOffSpillEmulation()

//========================================================================
void DTCFrontEndInterface::SetCFOEmulatorFixedWidthEmulation(__ARGS__)
{	
	bool enable = __GET_ARG_IN__("Enable CFO Emulator (Default := false)",bool,false);
	__FE_COUTV__(enable);

	std::stringstream outSs;

	thisDTC_->DisableCFOEmulation();
	if(!enable) //do not need to apply parameters if disabling		
	{
		outSs << "Halted CFO Emulator!" << __E__;
		__SET_ARG_OUT__("response", outSs.str());
		return;
	}
	//else enabling, so apply parameters, then enable

	thisDTC_->SoftReset(); //to reset event window tag starting point handling

	bool useDetachedBufferTest = __GET_ARG_IN__("Use Detached Buffer Test (Default := false)",uint32_t);

	// release of all the buffers
	if(!useDetachedBufferTest)
		getDevice()->read_release(DTC_DMA_Engine_DAQ, 100);

	std::string eventDuration = __GET_ARG_IN__("Fixed-width Event Window Duration (s, ms, us, ns, and clocks allowed) [clocks := 25ns]",std::string);
	__FE_COUTV__(eventDuration);
	bool foundUnits = false;
	size_t i;
	for(i=0;i<eventDuration.size();++i)
		if(eventDuration[i] == 's' 
			|| eventDuration[i] == 'm'
			|| eventDuration[i] == 'u'
			|| eventDuration[i] == 'n'
			|| eventDuration[i] == 'c')
		{ foundUnits = true; break;}

	if(!foundUnits)
	{
		__SS__ << "No units were found in the input parameters 'Fixed-width Event Window Duration' value: " <<
			eventDuration <<
			". Please use units when specifying event window duration (s, ms, us, ns, and clocks are allowed). For example '1.7us' or '1675ns' would be valid." << __E__;
		__SS_THROW__;
	}
	std::string eventDurationSplitNumber = eventDuration.substr(0,i);
	std::string eventDurationSplitUnits = eventDuration.substr(i);
	__FE_COUTV__(eventDurationSplitNumber);
	__FE_COUTV__(eventDurationSplitUnits);

	//copied from CFO_Compiler.cpp::transcribeInstructions() [L494]
	uint64_t value;
	if(!StringMacros::getNumber(eventDurationSplitNumber,value))
	{
		__FE_SS__<< "The duration parameter value '" << eventDurationSplitNumber << " " << eventDurationSplitUnits << "' is not a valid number. " <<
			"Use 0x### to indicate hex and b### to indicate binary; otherwise, decimal is inferred." << __E__;
		__FE_SS_THROW__;
	}
	//test floating point in case integer conversion dropped something
	double timeValue = strtod(eventDurationSplitNumber.c_str(), 0);
	__FE_COUTV__(timeValue);
	if(timeValue < value)
		timeValue = value;

	const uint64_t 							FPGAClock_ = (1e9/(40e6) /* 40MHz FPGAClock for calculating delays */); //period of FPGA clock in ns	
	__FE_COUTV__(FPGAClock_);
	__FE_COUTV__(value);
	__FE_COUTV__(timeValue);

	uint32_t eventDurationInClocks;
	
	if (eventDurationSplitUnits == "s")  // Wait wanted in seconds
		eventDurationInClocks = timeValue * 1e9 / FPGAClock_;
	else if (eventDurationSplitUnits == "ms")  // Wait wanted in milliseconds
		eventDurationInClocks = timeValue * 1e6 / FPGAClock_;
	else if (eventDurationSplitUnits == "us")  // Wait wanted in microseconds
		eventDurationInClocks = timeValue * 1e3 / FPGAClock_;
	else if (eventDurationSplitUnits == "ns")  // Wait wanted in nanoseconds
	{
		if ((value % FPGAClock_) != 0)
		{
			__FE_SS__ << "FPGA can only wait in multiples of " <<
				FPGAClock_ << " ns: the input event duration value '" << value << "' yields a remainder of " <<
				(value % FPGAClock_) << __E__;
			__FE_SS_THROW__;
		}
		eventDurationInClocks = value / FPGAClock_;
	}
	else if (eventDurationSplitUnits == "clocks")  // Wait wanted in FPGA clocks
		eventDurationInClocks = value;
	else //impossible
	{
		__FE_SS__ << "The event duration input parameter is missing a valid unit type after parameter: " << eventDurationSplitUnits <<
			". Accepted unit types are clocks, ns, us, ms, and s." << __E__; 
		__FE_SS_THROW__;
	}			
	if(eventDurationInClocks < 40)
	{
		__FE_SS__ << "The event duration input parameter can not evaluate to less than 40 clocks (1000ns). The input value '" << 
			eventDurationSplitNumber << " " << eventDurationSplitUnits << "' evaluates to " << eventDurationInClocks
			<< "clocks < 40." << __E__; 
		__FE_SS_THROW__;
	}			

	__FE_COUTV__(eventDurationInClocks);
	thisDTC_->SetCFOEmulationHeartbeatInterval(eventDurationInClocks);  

	uint32_t numberOfEventWindows = __GET_ARG_IN__("Number of Event Window Markers to generate (0 := infinite)",uint32_t);
	__FE_COUTV__(numberOfEventWindows);
	thisDTC_->SetCFOEmulationNumHeartbeats(numberOfEventWindows);
	
	uint64_t initialEventWindowTag = __GET_ARG_IN__("Starting Event Window Tag",uint64_t);
	__FE_COUTV__(initialEventWindowTag);
	thisDTC_->SetCFOEmulationTimestamp(DTCLib::DTC_EventWindowTag(initialEventWindowTag));

	uint64_t eventWindowMode = __GET_ARG_IN__("Event Window Mode (Default := 1)", uint64_t, 1);
	__FE_COUTV__(eventWindowMode);
	thisDTC_->SetCFOEmulationEventMode(eventWindowMode);

	bool enableClockMarkers = __GET_ARG_IN__("Enable Clock Markers (Default := false)",bool,false);
	__FE_COUTV__(enableClockMarkers);
	thisDTC_->SetCFOEmulation40MHzClockMarkerEnable(DTCLib::DTC_Link_ID::DTC_Link_ALL,enableClockMarkers);

	if(__GET_ARG_IN__("Enable Auto-generation of Data Request Packets (Default := false)",bool,false))
		thisDTC_->EnableAutogenDRP();
	else
		thisDTC_->DisableAutogenDRP();

	thisDTC_->EnableReceiveCFOLink();  //enable forwarding if CFO timing link to ROCs

	
	if(useDetachedBufferTest)
	{
		bool saveBinaryDataToFile = __GET_ARG_IN__("For Detached Buffer Test, Save Binary Data to File (Default: false)", bool);
		__FE_COUTV__(saveBinaryDataToFile);
		bool doNotResetCounters = __GET_ARG_IN__("For Detached Buffer Test, Do NOT Reset Counters (Default: false)", bool);
		__FE_COUTV__(doNotResetCounters);
		__FE_COUT__ << "Initializing detached buffer test!" << __E__;

		if(!bufferTestThreadStruct_) //initialize shared pointer for first time
			bufferTestThreadStruct_ = std::make_shared<DTCFrontEndInterface::DetachedBufferTestThreadStruct>();
		
		if(bufferTestThreadStruct_->running_)
		{
			__FE_COUT__ << "Found buffer test thread already running... so re-initializing" << __E__;
			
			// start mutex scope
			{
				std::lock_guard<std::mutex> lock(bufferTestThreadStruct_->lock_);
				bufferTestThreadStruct_->inSubeventMode_ 		= true;
				bufferTestThreadStruct_->activeMatch_ 			= false;
				bufferTestThreadStruct_->expectedEventTag_ 		= initialEventWindowTag;
				bufferTestThreadStruct_->saveBinaryData_ 		= saveBinaryDataToFile;
				bufferTestThreadStruct_->exitThread_ 			= false;
				bufferTestThreadStruct_->resetStartEventTag_ 	= true;
				bufferTestThreadStruct_->doNotResetCounters_ 	= doNotResetCounters;
			}
			outSs << "Found buffer test thread already running... so re-initializing and reading data starting at event tag " << initialEventWindowTag << 
				" (0x" << std::hex << initialEventWindowTag << ")" << __E__;
		}
		else
		{
			__FE_COUT__  << "Launching detached Buffer Test thread..." << __E__;
			// start mutex scope
			{
				std::lock_guard<std::mutex> lock(bufferTestThreadStruct_->lock_);
				bufferTestThreadStruct_->inSubeventMode_ 		= true;
				bufferTestThreadStruct_->activeMatch_ 			= false;
				bufferTestThreadStruct_->expectedEventTag_ 		= initialEventWindowTag;
				bufferTestThreadStruct_->saveBinaryData_ 		= saveBinaryDataToFile;
				bufferTestThreadStruct_->exitThread_ 			= false;
				bufferTestThreadStruct_->resetStartEventTag_ 	= false;
				bufferTestThreadStruct_->thisDTC_				= thisDTC_;
				bufferTestThreadStruct_->running_ 				= true;	
				bufferTestThreadStruct_->doNotResetCounters_ 	= false;
			}
			std::thread([](std::shared_ptr<DTCFrontEndInterface::DetachedBufferTestThreadStruct> threadStruct) { 
						DTCFrontEndInterface::detechedBufferTestThread(threadStruct); },
					bufferTestThreadStruct_)
			.detach();
			outSs << "Launched detached Buffer Test thread and reading data DMA-0 starting at event tag " << initialEventWindowTag << 
				" (0x" << std::hex << initialEventWindowTag << ")" << __E__;
		}

		sleep(1); //give time for buffer reading to be ready
	}

	thisDTC_->EnableCFOEmulation();

	outSs << "Launched CFO Emulator!" << __E__;
	__SET_ARG_OUT__("response", outSs.str());
	
} //end SetCFOEmulatorFixedWidthEmulation()


//==============================================================================
std::string DTCFrontEndInterface::getDetachedBufferTestStatus(std::shared_ptr<DTCFrontEndInterface::DetachedBufferTestThreadStruct> threadStruct)
{
	__COUT__ << "Get detached buffer test status..." << __E__;

	std::stringstream statusSs;

	// start mutex scope
	{
		std::lock_guard<std::mutex> lock(threadStruct->lock_);
		__COUT__ << "Have lock to read..." << __E__;

		statusSs << "Detached thread running:" << (threadStruct->running_ ? "true" : "false") << __E__;
		statusSs << "Events count:" << threadStruct->eventsCount_ << __E__;
		statusSs << "Subevents count:" << threadStruct->subeventsCount_ << __E__;
		statusSs << "Next Expected Event Window Tag:" << threadStruct->nextEventWindowTag_ << __E__;
		statusSs << "Mismatched Event Tags count:" << threadStruct->mismatchedEventTagsCount_ << __E__;

		if(threadStruct->mismatchedEventTagJumps_.size() < 20)
		{
			statusSs << "\t Mismatched Tag Jumps..." << __E__;
			for(size_t i=0;i<threadStruct->mismatchedEventTagJumps_.size();++i)
				statusSs << "\t\t Mismatch Jump-" << i <<
					" Expected:" << threadStruct->mismatchedEventTagJumps_[i].first <<
					" Received:" << threadStruct->mismatchedEventTagJumps_[i].second << __E__;
		}

		
		statusSs << "ROC Fragments..." << __E__;
		for(size_t i=0;i<threadStruct->rocFragmentsCount_.size();++i)
			statusSs << "\t Roc-" << i << " Fragments count:" << threadStruct->rocFragmentsCount_[i] << __E__;

		statusSs << "ROC Payload Empty count..." << __E__;
		for(size_t i=0;i<threadStruct->rocPayloadEmptyCount_.size();++i)
			statusSs << "\t Roc-" << i << " Payload Empty count:" << threadStruct->rocPayloadEmptyCount_[i] << __E__;

		statusSs << "ROC Payload Byte count..." << __E__;
		for(size_t i=0;i<threadStruct->rocPayloadByteCount_.size();++i)
			statusSs << "\t Roc-" << i << " Payload bytes:" << threadStruct->rocPayloadByteCount_[i] << __E__;

		statusSs << "ROC Subevent Header Timeouts..." << __E__;
		for(size_t i=0;i<threadStruct->rocFragmentTimeoutsCount_.size();++i)
			statusSs << "\t Roc-" << i << " Subevent Timeouts count:" << threadStruct->rocFragmentTimeoutsCount_[i] << __E__;

		statusSs << "ROC Fragment Header Timeouts..." << __E__;
		for(size_t i=0;i<threadStruct->rocHeaderTimeoutsCount_.size();++i)
			statusSs << "\t Roc-" << i << " Fragment Header Timeouts count:" << threadStruct->rocHeaderTimeoutsCount_[i] << __E__;

		statusSs << "ROC Errors (Timeouts + others)..." << __E__;
		for(size_t i=0;i<threadStruct->rocFragmentErrorsCount_.size();++i)
			statusSs << "\t Roc-" << i << " Fragment Errors count:" << threadStruct->rocFragmentErrorsCount_[i] << __E__;
	}
	__COUT__ << "Done getting detached buffer test status..." << __E__;

	return statusSs.str();
} //end getDetachedBufferTestStatus()

//==============================================================================
void DTCFrontEndInterface::handleDetachedSubevent(const DTCLib::DTC_SubEvent& subeventIn,
	std::shared_ptr<DTCFrontEndInterface::DetachedBufferTestThreadStruct> threadStruct)
{
	const DTCLib::DTC_SubEvent* subevent = &subeventIn;

	++(threadStruct->subeventsCount_);

	// check the subevent tag window
	if(threadStruct->nextEventWindowTag_ != subevent->GetEventWindowTag().GetEventWindowTag(true))
	{
		++(threadStruct->mismatchedEventTagsCount_);		
		std::stringstream ostr;
		ostr << "Mismatched event tag. Expected = 0x" << std::hex << std::setw(4) << std::setfill('0') <<
			threadStruct->nextEventWindowTag_ << ", Received = 0x" << std::hex << std::setw(4) << std::setfill('0') <<
			subevent->GetEventWindowTag().GetEventWindowTag(true);
		__COUT__ << ostr.str();
		threadStruct->mismatchedEventTagJumps_.push_back(std::make_pair<uint64_t, uint64_t>(
			threadStruct->nextEventWindowTag_,
			subevent->GetEventWindowTag().GetEventWindowTag(true)
		));
	}
	
	if(threadStruct->inSubeventMode_)
		threadStruct->nextEventWindowTag_ = subevent->GetEventWindowTag().GetEventWindowTag(true) + 1; //increment for next

	// print the subevent header
	// ostr << subevent->GetHeader()->toJson() << std::endl;


	//start mutex scope to change non-atomic status counters
	std::lock_guard<std::mutex> lock(threadStruct->lock_);
	
	// check if there is an error on the link in the subevent header
	if (subevent->GetHeader()->link0_status > 0)
	{
		// ostr << "Error: " << std::endl;
		++(threadStruct->rocFragmentErrorsCount_[0]);

		std::bitset<8> link_status(subevent->GetHeader()->link0_status);
		if (link_status.test(0)) 
		{
			++(threadStruct->rocFragmentTimeoutsCount_[0]);
			// ostr << "ROC Timeout Error!" << std::endl;
		}
		// if (link0_status.test(2)) 
		// {
		// 	// ostr << "Packet sequence number Error!" << std::endl;
		// }
		// if (link0_status.test(3)) 
		// {
		// 	// ostr << "CRC Error!" << std::endl;
		// }
		// if (link0_status.test(6)) 
		// {
		// 	// ostr << "Fatal Error!" << std::endl;
		// }
		
	} //end link0 errors
	if (subevent->GetHeader()->link1_status > 0)
	{
		++(threadStruct->rocFragmentErrorsCount_[1]);
		std::bitset<8> link_status(subevent->GetHeader()->link1_status);
		if (link_status.test(0)) 
			++(threadStruct->rocFragmentTimeoutsCount_[1]);							
	} //end link1 errors
	if (subevent->GetHeader()->link2_status > 0)
	{
		++(threadStruct->rocFragmentErrorsCount_[2]);
		std::bitset<8> link_status(subevent->GetHeader()->link2_status);
		if (link_status.test(0)) 
			++(threadStruct->rocFragmentTimeoutsCount_[2]);							
	} //end link2 errors
	if (subevent->GetHeader()->link3_status > 0)
	{
		++(threadStruct->rocFragmentErrorsCount_[3]);
		std::bitset<8> link_status(subevent->GetHeader()->link3_status);
		if (link_status.test(0)) 
			++(threadStruct->rocFragmentTimeoutsCount_[3]);							
	} //end link3 errors
	if (subevent->GetHeader()->link4_status > 0)
	{
		++(threadStruct->rocFragmentErrorsCount_[4]);
		std::bitset<8> link_status(subevent->GetHeader()->link4_status);
		if (link_status.test(0)) 
			++(threadStruct->rocFragmentTimeoutsCount_[4]);							
	} //end link4 errors
	if (subevent->GetHeader()->link5_status > 0)
	{
		++(threadStruct->rocFragmentErrorsCount_[5]);
		std::bitset<8> link_status(subevent->GetHeader()->link5_status);
		if (link_status.test(0)) 
			++(threadStruct->rocFragmentTimeoutsCount_[5]);							
	} //end link5 errors


	// print the number of data blocks
	// ostr << "Number of Data Block (ROC Fragments): " << subevent->GetDataBlockCount() << std::endl;
	
	// iterate over the data blocks
	std::vector<DTCLib::DTC_DataBlock> dataBlocks = subevent->GetDataBlocks();
	for (unsigned int j = 0; j < dataBlocks.size(); ++j)
	{
		// print the data block header
		DTCLib::DTC_DataHeaderPacket *dataHeader = dataBlocks[j].GetHeader().get();			
		__COUT__ << dataHeader->toJSON() << __E__;

		++(threadStruct->rocFragmentsCount_[dataHeader->GetLinkID()]);		

		// ~~~	The Data Header Packet Status 8-bit field is defined as follows ~~~
		// Bit Position	Definition
		// 0	“Event Window has Data” flag indicates detector data present, else No Data for Event Window.
		// 1	“Invalid Event Window Request” flag indicates the ROC did not receive a Heartbeat packet corresponding to this Data Request.
		// 2	“I am corrupt” flag indicates the ROC has lost data or the ability to conduct detector readout has been compromised.
		// 3	“Timeout” flag indicates ROC retrieval of data did not respond before timeout occurred.
		// 4	“Overflow” flag indicates data is good, but not all data could be sent.
		// 7:5	Reserved

		if((dataHeader->GetStatus() >> 3) & 1)
			++(threadStruct->rocHeaderTimeoutsCount_[dataHeader->GetLinkID()]);		
		
		// print the data block ROC fragment header raw data
		{
			auto dataPtr = reinterpret_cast<const uint8_t*>(dataBlocks[j].GetRawBufferPointer());
			// ostr << "Data header raw:" << std::endl;
			for (int l = 0; l < 16; l+=4)
			{
				if(threadStruct->fp_) fwrite(&dataPtr[l],sizeof(uint32_t), 1, threadStruct->fp_);
				// ostr << "\t0x" << std::hex << std::setw(8) << std::setfill('0') << *((uint32_t *)(&(dataPtr[l]))) << std::endl;
			}
		}

		// print the data block payload raw data
		{
			auto dataPtr = reinterpret_cast<const uint8_t*>(dataBlocks[j].GetData());

			threadStruct->rocPayloadByteCount_[dataHeader->GetLinkID()] += 
				dataHeader->GetByteCount() - 16;
			if(dataHeader->GetByteCount() - 16 == 0)	//count empty payloads	
				++(threadStruct->rocPayloadEmptyCount_[dataHeader->GetLinkID()]);		

			// if(displayPayloadAtGUI) ostr << "Data payload:" << std::endl;
			for (int l = 0; l < dataHeader->GetByteCount() - 16; l+=4)
			{
				if(threadStruct->fp_) fwrite(&dataPtr[l],sizeof(uint32_t), 1, threadStruct->fp_);
				// if(displayPayloadAtGUI) ostr << "\t0x" << std::hex << std::setw(8) << std::setfill('0') << *((uint32_t *)(&(dataPtr[l]))) << std::endl;
			}
		}

	} //end Data Block ROC fragment loop
	
	// ostr << std::endl << std::endl;

} //end handleDetachedSubevent()

//==============================================================================
// detechedBufferTestThread
void DTCFrontEndInterface::detechedBufferTestThread(std::shared_ptr<DTCFrontEndInterface::DetachedBufferTestThreadStruct> threadStruct)
try
{
	__COUT__ << "Buffer test thread established..." << __E__;

	if(threadStruct->fp_)
	{
		__SS__ << "Impossible?! File pointer already initialized?" << __E__;
		__SS_THROW__;
	}

	if(threadStruct->saveBinaryData_)
	{
		std::string filename = "/macroOutput_" + std::to_string(time(0)) + "_" +
									std::to_string(clock()) + ".bin";
		filename = std::string(__ENV__("OTSDAQ_DATA")) + "/" + filename;
		__COUTV__(filename);
		threadStruct->fp_ = fopen(filename.c_str(), "wb");
		if(!threadStruct->fp_)
		{
			__SS__ << "Failed to open file to save macro output '"
						<< filename << "'..." << __E__;
			__SS_THROW__;
		}		
	}
	//start with clean release
	// getDevice()->read_release(DTC_DMA_Engine_DAQ, 100);
	threadStruct->thisDTC_->ReleaseAllBuffers(DTC_DMA_Engine_DAQ);

	std::vector<std::unique_ptr<DTCLib::DTC_Event>> events;
	std::vector<std::unique_ptr<DTCLib::DTC_SubEvent>> subevents;
	uint64_t ii = 0;

	// start mutex scope
	{
		std::lock_guard<std::mutex> lock(threadStruct->lock_);
		threadStruct->nextEventWindowTag_.store(threadStruct->expectedEventTag_.load(std::memory_order_relaxed), std::memory_order_relaxed);
		__COUT_INFO__ << "Starting detached buffer test thread looking for Event Window Tag = " << 
			threadStruct->nextEventWindowTag_ << std::endl;

		threadStruct->eventsCount_ = 0;
		threadStruct->subeventsCount_ = 0;
		threadStruct->mismatchedEventTagsCount_ = 0;
		threadStruct->mismatchedEventTagJumps_.clear();
		threadStruct->rocFragmentsCount_ = {0,0,0,0,0,0};
		threadStruct->rocPayloadEmptyCount_ = {0,0,0,0,0,0};	
		threadStruct->rocFragmentTimeoutsCount_ = {0,0,0,0,0,0};
		threadStruct->rocFragmentErrorsCount_ = {0,0,0,0,0,0};
		threadStruct->rocHeaderTimeoutsCount_ = {0,0,0,0,0,0};
		threadStruct->rocPayloadByteCount_ = {0,0,0,0,0,0};
	}

	uint64_t lastCount = 0;

	std::stringstream ostr;
	//------------------------
	while(!threadStruct->exitThread_)
	{
		//check for new starting event tag
		{
			if(threadStruct->resetStartEventTag_)
			{ 
				if(threadStruct->doNotResetCounters_)
					__COUT_INFO__ << "NOT Resetting counters; previous status was as follows: \n" << 
						getDetachedBufferTestStatus(threadStruct) << __E__;
				else
					__COUT_INFO__ << "Resetting counters; previous status was as follows: \n" << 
						getDetachedBufferTestStatus(threadStruct) << __E__;

				// start mutex scope
				{
					std::lock_guard<std::mutex> lock(threadStruct->lock_);
					threadStruct->nextEventWindowTag_.store(threadStruct->expectedEventTag_.load(std::memory_order_relaxed), std::memory_order_relaxed);
					__COUT_INFO__ << "Restarting detached buffer test thread looking for Event Window Tag = " << 
						threadStruct->nextEventWindowTag_ << std::endl;

					//reset counts
					if(!threadStruct->doNotResetCounters_)
					{
						threadStruct->eventsCount_ = 0;
						threadStruct->subeventsCount_ = 0;
						threadStruct->mismatchedEventTagsCount_ = 0;
						threadStruct->mismatchedEventTagJumps_.clear();
						threadStruct->rocFragmentsCount_ = {0,0,0,0,0,0};
						threadStruct->rocPayloadEmptyCount_ = {0,0,0,0,0,0};	
						threadStruct->rocFragmentTimeoutsCount_ = {0,0,0,0,0,0};
						threadStruct->rocFragmentErrorsCount_ = {0,0,0,0,0,0};
						threadStruct->rocHeaderTimeoutsCount_ = {0,0,0,0,0,0};
						threadStruct->rocPayloadByteCount_ = {0,0,0,0,0,0};
					}
					
					threadStruct->resetStartEventTag_ = false; //clear mailbox
				}

				//release buffers for restart				
				threadStruct->thisDTC_->ReleaseAllBuffers(DTC_DMA_Engine_DAQ);
			}
			
		} //done with check for starting event window tag

		if(!threadStruct->inSubeventMode_) //treat as an Event
		{			
		  TLOG_DEBUG() << "get the data requested as events via ->GetData(...)";
			while((events = threadStruct->thisDTC_->GetData(DTCLib::DTC_EventWindowTag(threadStruct->nextEventWindowTag_), 
				threadStruct->activeMatch_)).size())
			{ 
				__COUT__ << "Read iteration #" << ii++ << ": Events returned by the DTC: " << events.size() << std::endl;
				if (events.empty()) break; //impossible!
				
				for(auto& eventPtr : events) 
				{
					if (eventPtr == nullptr) 
					{
						__COUT_ERR__ << "Error: Null pointer!" << std::endl;
						continue;
					}

					// get the event
					auto event = eventPtr.get();
					
					++(threadStruct->eventsCount_);

					// check the event tag window
					if(threadStruct->nextEventWindowTag_ != event->GetEventWindowTag().GetEventWindowTag(true))
					{
						++(threadStruct->mismatchedEventTagsCount_);
						ostr.str(""); //clear
						ostr << "Mismatched event tag. Expected = 0x" << std::hex << std::setw(4) << std::setfill('0') <<
							threadStruct->nextEventWindowTag_ << ", Received = 0x" << std::hex << std::setw(4) << std::setfill('0') <<
							event->GetEventWindowTag().GetEventWindowTag(true);
						__COUT__ << ostr.str();

						threadStruct->mismatchedEventTagJumps_.push_back(std::make_pair<uint64_t, uint64_t>(
							threadStruct->nextEventWindowTag_,
							event->GetEventWindowTag().GetEventWindowTag(true)
						));
					}
					threadStruct->nextEventWindowTag_ = event->GetEventWindowTag().GetEventWindowTag(true) + 1; //increment for next

					// ostr << "Request event tag:\t" << "0x" << std::hex << std::setw(4) << std::setfill('0') << eventTag.GetEventWindowTag(true) + ii << 
					// 	" (" << std::dec << eventTag.GetEventWindowTag(true) + ii << ")" << std::endl;
					// ostr << "Response event tag:\t" << "0x" << std::hex << std::setw(4) << std::setfill('0') << event->GetEventWindowTag().GetEventWindowTag(true) << 
					// 	" (" << std::dec << event->GetEventWindowTag().GetEventWindowTag(true) << ")" << std::endl;

					// get the event and the relative sub events
					// DTCLib::DTC_EventHeader *eventHeader = event->GetHeader();
					std::vector<DTCLib::DTC_SubEvent> subevents = event->GetSubEvents();

					for(auto& subevent : subevents) 					
						handleDetachedSubevent(subevent, threadStruct);				
				}			
			
			} //end primary event retrieval loop
			//if here, no more data in DMA buffer
			if(lastCount != threadStruct->eventsCount_ || ii%100)
			{
				__COUT__ << "No more events found in DMA bufferr... waiting... iteration #" << ii << 
					", Events received so far = " << threadStruct->eventsCount_ << __E__;
				lastCount = threadStruct->eventsCount_;
			}
		}
		else //Treat as Subevent
		{
		  TLOG_DEBUG() << "get the data requested as events via ->GetSubEventData(...)";
			while((subevents = threadStruct->thisDTC_->GetSubEventData(DTCLib::DTC_EventWindowTag(threadStruct->nextEventWindowTag_), 
				threadStruct->activeMatch_)).size())
			{ 
				__COUT__ << "Read iteration #" << ii++ << ": SubEvents returned by the DTC: " << subevents.size() << std::endl;
				
				if (subevents.empty()) continue; //impossible!
				
				for(auto& subeventPtr : subevents) 	
				{
					if (subeventPtr == nullptr) 
					{
						__COUT_ERR__ << "Error: Subevent Null pointer!" << std::endl;
						continue;
					}
					handleDetachedSubevent(*(subeventPtr.get()), threadStruct);
				}
				
			} //end primary Sub Event loop
			//if here, no more data in DMA buffer
			if(lastCount != threadStruct->subeventsCount_ || ii%100)
			{
				__COUT__ << "No more subevents found in DMA bufferr... waiting... iteration #" << ii << 
					", SubEvents received so far = " << threadStruct->subeventsCount_ << __E__;
				lastCount = threadStruct->subeventsCount_;
			}
		} // end Sub Event handling

		if(threadStruct->fp_) fflush(threadStruct->fp_);
		// usleep(100); //100 us sleep
		std::this_thread::yield(); //try to be nice to other threads
		++ii;
	} //end primary loop -------

	if(threadStruct->fp_) fclose(threadStruct->fp_);

	__COUT__ << "Buffer test thread exited. " <<
		" Events received = " << threadStruct->eventsCount_ << ", SubEvents received = " << threadStruct->subeventsCount_ << __E__;
	threadStruct->running_ = false;

} //end detechedBufferTestThread()
catch(...)
{
	__COUT_ERR__ << "Exception caught. Exiting detechedBufferTestThread()." << __E__;
	threadStruct->running_ = false;
	try
	{throw;} 
	catch(const std::runtime_error& e)
	{
		__COUT_ERR__ << "Error message: " << e.what() << __E__;
	}
	catch(...)
	{
		__COUT_ERR__ << "Unknown error." << __E__;
	}
} //end detechedBufferTestThread() exception handling

//========================================================================
void DTCFrontEndInterface::BufferTest_detached(__ARGS__)
{
	__FE_COUT__ << "Operation \"BufferTest_detached\"" << std::endl;


	// arguments
	std::string command = __GET_ARG_IN__("Command to Start, Status (to read counters, etc.), or Halt (Default: Status)",
		std::string, "Status");

	bool dataAreSubEvents = __GET_ARG_IN__("Data are SubEvents (Default: true)", bool, true);
	// unsigned int numberOfEvents = __GET_ARG_IN__("Number of [Sub]Events (Default: 1)", uint32_t, 1);
	bool activeMatch = __GET_ARG_IN__("Match Event Tags (Default: false)", bool);
	unsigned int timestampStart = __GET_ARG_IN__("Starting Event Window Tag (Default: 0)", unsigned int);
	bool saveBinaryDataToFile = __GET_ARG_IN__("Save Binary Data to File (Default: false)", bool);
	// bool displayPayloadAtGUI = __GET_ARG_IN__("Display Payload at GUI (Default: true)", bool, true);


	__FE_COUTV__(command);
	__FE_COUTV__(dataAreSubEvents);
	__FE_COUTV__(activeMatch);
	__FE_COUTV__(timestampStart);
	__FE_COUTV__(saveBinaryDataToFile);


	// print the result
	std::stringstream outSs;
	outSs << "Command: " << command << __E__;
	if(command == "Start")
	{
		__FE_COUT__ << "Detaching thread and reading data DMA-0 starting at event tag " << timestampStart << 
			" (0x" << std::hex << timestampStart << ")" << __E__;

		if(!bufferTestThreadStruct_) //initialize shared pointer for first time
			bufferTestThreadStruct_ = std::make_shared<DTCFrontEndInterface::DetachedBufferTestThreadStruct>();

		if(bufferTestThreadStruct_->running_)
			outSs << "Found buffer test thread already running, doing nothing. Please 'Halt' before restarting. Or run 'Status' to read the latest status." << __E__;
		else
		{
			__FE_COUT__  << "Launching detached Buffer Test thread..." << __E__;

			// start mutex scope
			{
				std::lock_guard<std::mutex> lock(bufferTestThreadStruct_->lock_);
				bufferTestThreadStruct_->inSubeventMode_ 		= dataAreSubEvents;
				bufferTestThreadStruct_->activeMatch_ 			= activeMatch;
				bufferTestThreadStruct_->expectedEventTag_ 		= timestampStart;
				bufferTestThreadStruct_->saveBinaryData_ 		= saveBinaryDataToFile;
				bufferTestThreadStruct_->exitThread_ 			= false;
				bufferTestThreadStruct_->resetStartEventTag_ 	= false;
				bufferTestThreadStruct_->thisDTC_				= thisDTC_;
				bufferTestThreadStruct_->running_ 				= true;	
			}
			std::thread([](std::shared_ptr<DTCFrontEndInterface::DetachedBufferTestThreadStruct> threadStruct) { 
						DTCFrontEndInterface::detechedBufferTestThread(threadStruct); },
					bufferTestThreadStruct_)
			.detach();
			outSs << "Launced detached Buffer Test thread and reading data DMA-0 starting at event tag " << timestampStart << 
				" (0x" << std::hex << timestampStart << ")" << __E__;
		}
	}
	else if(command == "Status")
	{
		__FE_COUT__ << "Reading thread status..." << __E__;
		outSs << "Reading thread status..." << __E__;

		if(!bufferTestThreadStruct_) //initialize shared pointer for first time
			bufferTestThreadStruct_ = std::make_shared<DTCFrontEndInterface::DetachedBufferTestThreadStruct>();

		outSs << DTCFrontEndInterface::getDetachedBufferTestStatus(bufferTestThreadStruct_);
	}
	else if(command == "Halt")
	{
		__FE_COUT__ << "Halting thread... " << __E__;

		if(!bufferTestThreadStruct_) //initialize shared pointer for first time
			bufferTestThreadStruct_ = std::make_shared<DTCFrontEndInterface::DetachedBufferTestThreadStruct>();

		// start mutex scope
		{
			std::lock_guard<std::mutex> lock(bufferTestThreadStruct_->lock_);
			bufferTestThreadStruct_->exitThread_ = true;
		}

		//check for thread to exit
		for(int i=0;i<10;++i)
		{
			usleep(100 * 1000 /*100ms*/);  // sleep for exit time
			if(!bufferTestThreadStruct_->running_) break;
			__FE_COUT__ << "Waiting for thread to exit... #" << i << __E__;
		}

		outSs << "Detached Buffer Test thread exited. " << __E__;
		outSs << "Reading final status..." << __E__;
		outSs << DTCFrontEndInterface::getDetachedBufferTestStatus(bufferTestThreadStruct_);
	}
	else
	{
		outSs << "Unrecognized command '" << command << "' found. Valid commands are Start, Status, and Halt." << __E__;
	}
	// outSs << "Active Event Match: " << (activeMatch?"true":"false") << __E__;
	// // outSs << "Event Duration: " << cfoDelay << " = " << cfoDelay*25 << " ns" << __E__;
	// // outSs << "Reading back: " << (doNotReadBack?"false":"true") << __E__;
	// if(fp) outSs << "Binary data file saved at: " << filename << __E__;
	// outSs << ostr.str();

	std::cout << "Untruncated output: \n" << outSs.str() << __E__; //for no truncation!

	__SET_ARG_OUT__("response", outSs.str());
} //end BufferTest_detached()

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
	bool dataAreSubEvents = __GET_ARG_IN__("Data are SubEvents (Default: true)", bool, true);
	unsigned int numberOfEvents = __GET_ARG_IN__("Number of [Sub]Events (Default: 1)", uint32_t, 1);
	bool activeMatch = __GET_ARG_IN__("Match Event Tags (Default: false)", bool);
	unsigned int timestampStart = __GET_ARG_IN__("Starting Event Window Tag (Default: 0)", unsigned int);
	bool saveBinaryDataToFile = __GET_ARG_IN__("Save Binary Data to File (Default: false)", bool);
	bool displayPayloadAtGUI = __GET_ARG_IN__("Display Payload at GUI (Default: true)", bool, true);
	
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
	DTCLib::DTC_EventWindowTag eventTag = DTCLib::DTC_EventWindowTag(static_cast<uint64_t>(timestampStart));

	// // create the emulated CFO instance
	// DTCLib::DTCSoftwareCFO* cfo = new DTCLib::DTCSoftwareCFO(thisDTC_,
	// 															useCFOinDTCEmulator,
	// 															debugPacketCount,
	// 															debugType,
	// 															stickyDebugType,
	// 															quiet,
	// 															asyncRR,
	// 															forceNoDebugMode);
	// // send the request for a range of events
	// cfo->SendRequestsForRange(numberOfEvents,
	// 							eventTag,
	// 							incrementTimestamp,
	// 							cfoDelay,
	// 							requestsAhead,
	// 							16 /* heartbeatsAfter */,
	// 							!doNotSendHeartbeats /* sendHeartbeats */);


	std::string filename = "/macroOutput_" + std::to_string(time(0)) + "_" +
			                       std::to_string(clock()) + ".bin";
	FILE *fp = nullptr;
	if(saveBinaryDataToFile)
	{
		filename = std::string(__ENV__("OTSDAQ_DATA")) + "/" + filename;
		__FE_COUTV__(filename);
		fp = fopen(filename.c_str(), "wb");
		if(!fp)
		{
			__FE_SS__ << "Failed to open file to save macro output '"
						<< filename << "'..." << __E__;
			__FE_SS_THROW__;
		}
	}
	
	if(!dataAreSubEvents) //treat as an Event
	{
		// get the data requested
		for(unsigned int ii = 0; //!doNotReadBack &&  
			ii < numberOfEvents; ++ii)
		{ 
			// get the data
			std::vector<std::unique_ptr<DTCLib::DTC_Event>> events = thisDTC_->GetData(eventTag + ii, activeMatch);
			ostr << "Read " << std::dec << ii << ": Events returned by the DTC: " << events.size() << std::endl;
			if (!events.empty()) 
			{
				for(auto& eventPtr : events) 
				{
					if (eventPtr == nullptr) 
					{
						ostr << "Error: Null pointer!" << std::endl;
						continue;
					}

					// get the event
					auto event = eventPtr.get();

					// check the event tag window
					ostr << "Request event tag:\t" << "0x" << std::hex << std::setw(4) << std::setfill('0') << eventTag.GetEventWindowTag(true) + ii << 
						" (" << std::dec << eventTag.GetEventWindowTag(true) + ii << ")" << std::endl;
					ostr << "Response event tag:\t" << "0x" << std::hex << std::setw(4) << std::setfill('0') << event->GetEventWindowTag().GetEventWindowTag(true) << 
						" (" << std::dec << event->GetEventWindowTag().GetEventWindowTag(true) << ")" << std::endl;

					// get the event and the relative sub events
					DTCLib::DTC_EventHeader *eventHeader = event->GetHeader();
					std::vector<DTCLib::DTC_SubEvent> subevents = event->GetSubEvents();

					// print the event header
					ostr << eventHeader->toJson() << std::endl
							<< "Subevents count: " << event->GetSubEventCount() << std::endl;
					
					// iterate over the subevents
					for (unsigned int i = 0; i < subevents.size(); ++i)
					{
						// print the subevents header
						DTCLib::DTC_SubEvent subevent = subevents[i];
						ostr << "Subevent [" << i << "]:" << std::endl;
						ostr << subevent.GetHeader()->toJson() << std::endl;

						// check if there is an error on the link
						if (subevent.GetHeader()->link0_status > 0)
						{
							ostr << "Error: " << std::endl;
							std::bitset<8> link0_status(subevent.GetHeader()->link0_status);
							if (link0_status.test(0)) 
							{
								ostr << "ROC Timeout Error!" << std::endl;
							}
							if (link0_status.test(2)) 
							{
								ostr << "Packet sequence number Error!" << std::endl;
							}
							if (link0_status.test(3)) 
							{
								ostr << "CRC Error!" << std::endl;
							}
							if (link0_status.test(6)) 
							{
								ostr << "Fatal Error!" << std::endl;
							}
							
							continue;
						}

						// print the number of data blocks
						ostr << "Number of Data Block: " << subevent.GetDataBlockCount() << std::endl;
						
						// iterate over the data blocks
						std::vector<DTCLib::DTC_DataBlock> dataBlocks = subevent.GetDataBlocks();
						for (unsigned int j = 0; j < dataBlocks.size(); ++j)
						{
							ostr << "Data block [" << j << "]:" << std::endl;
							// print the data block header
							DTCLib::DTC_DataHeaderPacket *dataHeader = dataBlocks[j].GetHeader().get();
							ostr << dataHeader->toJSON() << std::endl;	
				
							// print the data block ROC fragment header raw data
							{
								auto dataPtr = reinterpret_cast<const uint8_t*>(dataBlocks[j].GetRawBufferPointer());
								ostr << "Data header raw:" << std::endl;
								for (int l = 0; l < 16; l+=2)
								{
									if(fp) fwrite(&dataPtr[l-16],sizeof(uint32_t), 1, fp);
									ostr << "\t0x" << std::hex << std::setw(8) << std::setfill('0') << *((uint32_t *)(&(dataPtr[l-16]))) << std::endl;
								}
							}

							// print the data block payload raw data
							{
								auto dataPtr = reinterpret_cast<const uint8_t*>(dataBlocks[j].GetData());
								if(displayPayloadAtGUI) ostr << "Data payload:" << std::endl;
								for (int l = 0; l < dataHeader->GetByteCount() - 16; l+=2)
								{
									if(fp) fwrite(&dataPtr[l],sizeof(uint32_t), 1, fp);
									if(displayPayloadAtGUI) ostr << "\t0x" << std::hex << std::setw(8) << std::setfill('0') << *((uint32_t *)(&(dataPtr[l]))) << std::endl;
								}
							}

						}
					}
					ostr << std::endl << std::endl;
				}
			}
		
		}
	}
	else //Treat as Subevent
	{
		uint32_t numberOfSubEvents = numberOfEvents;
		uint32_t numberOfSubEventsReceived = 0;

		// get the data requested
		for(unsigned int ii = 0; //!doNotReadBack &&  
			ii < numberOfSubEvents; ++ii)
		{ 
			if(numberOfSubEventsReceived) {
				ostr << "Received requested number of SubEvents: " << std::dec << numberOfSubEventsReceived << __E__;
				break;
			}

			// get the data
			std::vector<std::unique_ptr<DTCLib::DTC_SubEvent>> subevents = thisDTC_->GetSubEventData(eventTag + ii, activeMatch);
			numberOfSubEventsReceived += subevents.size();
			ostr << "Read " << std::dec << ii << ": SubEvents returned by the DTC: " << subevents.size() << ". Total received so far: " << 
				numberOfSubEventsReceived << std::endl;
			if (subevents.empty()) continue;
			
			for(auto& subeventPtr : subevents) 
			{
				if (subeventPtr == nullptr) 
				{
					ostr << "Error: Null pointer!" << std::endl;
					continue;
				}

				// get the subevent
				auto subevent = subeventPtr.get();

				// check the subevent tag window
				ostr << "Request subevent tag:\t" << "0x" << std::hex << std::setw(4) << std::setfill('0') << eventTag.GetEventWindowTag(true) + ii << 
					" (" << std::dec << eventTag.GetEventWindowTag(true) + ii << ")" << std::endl;
				ostr << "Response subevent tag:\t" << "0x" << std::hex << std::setw(4) << std::setfill('0') << subevent->GetEventWindowTag().GetEventWindowTag(true) << 
					" (" << std::dec << subevent->GetEventWindowTag().GetEventWindowTag(true) << ")" <<std::endl;

				// print the event header
				ostr << subevent->GetHeader()->toJson() << std::endl;
				
				// check if there is an error on the link
				if (subevent->GetHeader()->link0_status > 0)
				{
					ostr << "ROC-0 Error: " << std::endl;
					std::bitset<8> link0_status(subevent->GetHeader()->link0_status);
					if (link0_status.test(0)) 
					{
						ostr << "ROC Timeout Error!" << std::endl;
					}
					if (link0_status.test(2)) 
					{
						ostr << "Packet sequence number Error!" << std::endl;
					}
					if (link0_status.test(3)) 
					{
						ostr << "CRC Error!" << std::endl;
					}
					if (link0_status.test(6)) 
					{
						ostr << "Fatal Error!" << std::endl;
					}
					
					// continue;
				}

				if (subevent->GetHeader()->link1_status > 0)
				{
					ostr << "ROC-1 Error: " << std::endl;
					std::bitset<8> link1_status(subevent->GetHeader()->link1_status);
					if (link1_status.test(0)) 
					{
						ostr << "ROC Timeout Error!" << std::endl;
					}
					if (link1_status.test(2)) 
					{
						ostr << "Packet sequence number Error!" << std::endl;
					}
					if (link1_status.test(3)) 
					{
						ostr << "CRC Error!" << std::endl;
					}
					if (link1_status.test(6)) 
					{
						ostr << "Fatal Error!" << std::endl;
					}
					
					// continue;
				}

				if (subevent->GetHeader()->link2_status > 0)
				{
					ostr << "ROC-2 Error: " << std::endl;
					std::bitset<8> link2_status(subevent->GetHeader()->link2_status);
					if (link2_status.test(0)) 
					{
						ostr << "ROC Timeout Error!" << std::endl;
					}
					if (link2_status.test(2)) 
					{
						ostr << "Packet sequence number Error!" << std::endl;
					}
					if (link2_status.test(3)) 
					{
						ostr << "CRC Error!" << std::endl;
					}
					if (link2_status.test(6)) 
					{
						ostr << "Fatal Error!" << std::endl;
					}
					
					// continue;
				}

				// print the number of data blocks
				ostr << "Number of Data Block (ROC Fragments): " << subevent->GetDataBlockCount() << std::endl;
				
				// iterate over the data blocks
				std::vector<DTCLib::DTC_DataBlock> dataBlocks = subevent->GetDataBlocks();
				for (unsigned int j = 0; j < dataBlocks.size(); ++j)
				{
					ostr << "Data block [" << j << "]:" << std::endl;
					// print the data block header
					DTCLib::DTC_DataHeaderPacket *dataHeader = dataBlocks[j].GetHeader().get();
					ostr << dataHeader->toJSON() << std::endl;	

					// print the data block ROC fragment header raw data
					{
						auto dataPtr = reinterpret_cast<const uint8_t*>(dataBlocks[j].GetRawBufferPointer());
						ostr << "Data header raw:" << std::endl;
						for (int l = 0; l < 16; l+=4)
						{
							if(fp) fwrite(&dataPtr[l],sizeof(uint32_t), 1, fp);
							ostr << "\t0x" << std::hex << std::setw(8) << std::setfill('0') << *((uint32_t *)(&(dataPtr[l]))) << std::endl;
						}
					}

					// print the data block payload raw data
					{
						auto dataPtr = reinterpret_cast<const uint8_t*>(dataBlocks[j].GetData());
						if(displayPayloadAtGUI) ostr << "Data payload:" << std::endl;
						for (int l = 0; l < dataHeader->GetByteCount() - 16; l+=4)
						{
							if(fp) fwrite(&dataPtr[l],sizeof(uint32_t), 1, fp);
							if(displayPayloadAtGUI) ostr << "\t0x" << std::hex << std::setw(8) << std::setfill('0') << *((uint32_t *)(&(dataPtr[l]))) << std::endl;
						}
					}

				} //end Data Block ROC fragment loop
				
				ostr << std::endl << std::endl;
			} //end Sub Event loop (should only be one Sub Event)
		} //end primary Sub Event loop
	} // end Sub Event handling

	if(fp) fclose(fp); //close binary file

	// print the result
	std::stringstream outSs;
	outSs << "Number of events  requested: " + 
		std::to_string(numberOfEvents) <<  " from starting event tag " << timestampStart << __E__;
	outSs << "Active Event Match: " << (activeMatch?"true":"false") << __E__;
	// outSs << "Event Duration: " << cfoDelay << " = " << cfoDelay*25 << " ns" << __E__;
	// outSs << "Reading back: " << (doNotReadBack?"false":"true") << __E__;
	if(fp) outSs << "Binary data file saved at: " << filename << __E__;
	outSs << ostr.str();

	std::cout << "Untruncated output: \n" << outSs.str() << __E__; //for no truncation!

	__SET_ARG_OUT__("response", outSs.str());
	// delete cfo;
} //end BufferTest()


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
	bool dataAreSubEvents = __GET_ARG_IN__("Data are SubEvents (Default: true)", bool, true);
	unsigned int numberOfEvents = __GET_ARG_IN__("Number of [Sub]Events (Default: 1)", uint32_t, 1);
	bool activeMatch = __GET_ARG_IN__("Match Event Tags (Default: false)", bool);
	unsigned int timestampStart = __GET_ARG_IN__("Starting Event Window Tag (Default: 0)", unsigned int);
	bool saveBinaryDataToFile = __GET_ARG_IN__("Save Binary Data to File (Default: false)", bool);
	bool displayPayloadAtGUI = __GET_ARG_IN__("Display Payload at GUI (Default: true)", bool, true);
	

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
	DTCLib::DTC_EventWindowTag eventTag = DTCLib::DTC_EventWindowTag(static_cast<uint64_t>(timestampStart));

	// // create the emulated CFO instance
	// DTCLib::DTCSoftwareCFO* cfo = new DTCLib::DTCSoftwareCFO(thisDTC_,
	// 															useCFOinDTCEmulator,
	// 															debugPacketCount,
	// 															debugType,
	// 															stickyDebugType,
	// 															quiet,
	// 															asyncRR,
	// 															forceNoDebugMode);
	// // send the request for a range of events
	// cfo->SendRequestsForRange(numberOfEvents,
	// 							eventTag,
	// 							incrementTimestamp,
	// 							cfoDelay,
	// 							requestsAhead,
	// 							16 /* heartbeatsAfter */,
	// 							!doNotSendHeartbeats /* sendHeartbeats */);


	std::string filename = "/macroOutput_" + std::to_string(time(0)) + "_" +
			                       std::to_string(clock()) + "_pattern.bin";
	FILE *fp = nullptr;
	if(saveBinaryDataToFile)
	{
		filename = std::string(__ENV__("OTSDAQ_DATA")) + "/" + filename;
		__FE_COUTV__(filename);
		fp = fopen(filename.c_str(), "wb");
		if(!fp)
		{
			__FE_SS__ << "Failed to open file to save macro output '"
						<< filename << "'..." << __E__;
			__FE_SS_THROW__;
		}
	}
	
    int hit_in[64] = 
      { 1,  2,  3,  0,  0,  0,  7,  8,
        9, 10, 11, 12, 13, 14, 15, 16,
        0, 20, 21, 22, 12, 13, 11, 12,
        0,  0,  8,  4, 12, 11, 12, 13,
       16,  6,  3,  1, 12,  0, 16, 17,
       18, 19, 12,  1, 12, 12, 11, 11,
        0,  0,  0,  0, 13, 14, 10, 13,
       11, 14, 14, 15,  8,  9, 10, 32};

	if(!dataAreSubEvents) //treat as an Event
	{
		// get the data requested
		for(unsigned int ii = 0; //!doNotReadBack &&  
			ii < numberOfEvents; ++ii)
		{ 
			// get the data
			std::vector<std::unique_ptr<DTCLib::DTC_Event>> events = thisDTC_->GetData(eventTag + ii, activeMatch);
			ostr << "Read " << ii << ": Events returned by the DTC: " << events.size() << std::endl;
			if (!events.empty()) 
			{
				for(auto& eventPtr : events) 
				{
					if (eventPtr == nullptr) 
					{
						ostr << "Error: Null pointer!" << std::endl;
						continue;
					}

					// get the event
					auto event = eventPtr.get();

					// check the event tag window
					ostr << "Request event tag:\t" << "0x" << std::hex << std::setw(4) << std::setfill('0') << eventTag.GetEventWindowTag(true) + ii << 
						" (" << std::dec << eventTag.GetEventWindowTag(true) + ii << ")" << std::endl;
					ostr << "Response event tag:\t" << "0x" << std::hex << std::setw(4) << std::setfill('0') << event->GetEventWindowTag().GetEventWindowTag(true) << 
						" (" << std::dec << event->GetEventWindowTag().GetEventWindowTag(true) << ")" << std::endl;

					// get the event and the relative sub events
					//DTCLib::DTC_EventHeader *eventHeader = event->GetHeader();
					std::vector<DTCLib::DTC_SubEvent> subevents = event->GetSubEvents();

					// print the event header
					//ostr << eventHeader->toJson() << std::endl
					//		<< "Subevents count: " << event->GetSubEventCount() << std::endl;
					
					// iterate over the subevents
					for (unsigned int i = 0; i < subevents.size(); ++i)
					{
						// print the subevents header
						DTCLib::DTC_SubEvent subevent = subevents[i];
						ostr << "Subevent [" << i << "]:" << std::endl;
						//ostr << subevent.GetHeader()->toJson() << std::endl;

						// check if there is an error on the link
						if (subevent.GetHeader()->link0_status > 0)
						{
							ostr << "Error: " << std::endl;
							std::bitset<8> link0_status(subevent.GetHeader()->link0_status);
							if (link0_status.test(0)) 
							{
								ostr << "ROC Timeout Error!" << std::endl;
							}
							if (link0_status.test(2)) 
							{
								ostr << "Packet sequence number Error!" << std::endl;
							}
							if (link0_status.test(3)) 
							{
								ostr << "CRC Error!" << std::endl;
							}
							if (link0_status.test(6)) 
							{
								ostr << "Fatal Error!" << std::endl;
							}
							
							continue;
						}



						// print the number of data blocks
						ostr << "Number of Data Block: " << subevent.GetDataBlockCount() << std::endl;
						
						// iterate over the data blocks
						std::vector<DTCLib::DTC_DataBlock> dataBlocks = subevent.GetDataBlocks();
						for (unsigned int j = 0; j < dataBlocks.size(); ++j)
						{
							ostr << "Data block [" << j << "]:" << std::endl;
							// print the data block header
							DTCLib::DTC_DataHeaderPacket *dataHeader = dataBlocks[j].GetHeader().get();
							//ostr << dataHeader->toJSON() << std::endl;	
				
							// print the data block ROC fragment header raw data
							{
								auto dataPtr = reinterpret_cast<const uint8_t*>(dataBlocks[j].GetRawBufferPointer());
								//ostr << "Data header raw:" << std::endl;
								for (int l = 0; l < 16; l+=2)
								{
									if(fp) fwrite(&dataPtr[l-16],sizeof(uint32_t), 1, fp);
									//ostr << "\t0x" << std::hex << std::setw(8) << std::setfill('0') << *((uint32_t *)(&(dataPtr[l-16]))) << std::endl;
								}
							}

							// print the data block payload raw data
							{
								auto dataPtr = reinterpret_cast<const uint8_t*>(dataBlocks[j].GetData());
								if(displayPayloadAtGUI) ostr << "Data payload:" << std::endl;
								for (int l = 0; l < dataHeader->GetByteCount() - 16; l+=2)
								{

									if(fp) fwrite(&dataPtr[l],sizeof(uint32_t), 1, fp);
									if(displayPayloadAtGUI) ostr << "\t0x" << std::hex << std::setw(8) << std::setfill('0') << *((uint32_t *)(&(dataPtr[l]))) << std::endl;
								}
							}

						}
					}
					ostr << std::endl << std::endl;
				}
			}
		
		}
	}
	else //Treat as Subevent
	{
		uint32_t numberOfSubEvents = numberOfEvents;
		uint32_t numberOfSubEventsReceived = 0;

        uint32_t last_data = -1;
		


		// get the data requested
		for(unsigned int ii = 0; //!doNotReadBack &&  
			ii < numberOfSubEvents; ++ii)
		{ 
			// get the data
			std::vector<std::unique_ptr<DTCLib::DTC_SubEvent>> subevents = thisDTC_->GetSubEventData(eventTag + ii, activeMatch);
			numberOfSubEventsReceived += subevents.size();
			ostr << "Read " << ii << ": SubEvents returned by the DTC: " << subevents.size() << ". Total received so far: " << numberOfSubEventsReceived << std::endl;
			if (subevents.empty()) continue;
			
			for(auto& subeventPtr : subevents) 
			{
				if (subeventPtr == nullptr) 
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
				if (subevent->GetHeader()->link0_status > 0)
				{
					ostr << "ROC-0 Error: ";
					std::bitset<8> link0_status(subevent->GetHeader()->link0_status);
					if (link0_status.test(0)) 
					{
						ostr << "ROC Timeout Error!" << std::endl;
					}
					if (link0_status.test(2)) 
					{
						ostr << "Packet sequence number Error!" << std::endl;
					}
					if (link0_status.test(3)) 
					{
						ostr << "CRC Error!" << std::endl;
					}
					if (link0_status.test(6)) 
					{
						ostr << "Fatal Error!" << std::endl;
					}
					
					// continue;
				}

				if (subevent->GetHeader()->link1_status > 0 and 0)
				{
					ostr << "ROC-1 Error: ";
					std::bitset<8> link1_status(subevent->GetHeader()->link1_status);
					if (link1_status.test(0)) 
					{
						ostr << "ROC Timeout Error!" << std::endl;
					}
					if (link1_status.test(2)) 
					{
						ostr << "Packet sequence number Error!" << std::endl;
					}
					if (link1_status.test(3)) 
					{
						ostr << "CRC Error!" << std::endl;
					}
					if (link1_status.test(6)) 
					{
						ostr << "Fatal Error!" << std::endl;
					}
					
					// continue;
				}


				// print the number of data blocks
				//ostr << "Number of Data Block (ROC Fragments): " << subevent->GetDataBlockCount() << std::endl;
				
				// iterate over the data blocks
				std::vector<DTCLib::DTC_DataBlock> dataBlocks = subevent->GetDataBlocks();
				for (unsigned int j = 0; j < dataBlocks.size(); ++j)
				{
					//ostr << "Data block [" << j << "]:" << std::endl;
					// print the data block header
					DTCLib::DTC_DataHeaderPacket *dataHeader = dataBlocks[j].GetHeader().get();
					//ostr << dataHeader->toJSON() << std::endl;	

					// print the data block ROC fragment header raw data
					{
						auto dataPtr = reinterpret_cast<const uint8_t*>(dataBlocks[j].GetRawBufferPointer());
						//ostr << "Data header raw:" << std::endl;
						for (int l = 0; l < 16; l+=4)
						{
							if(fp) fwrite(&dataPtr[l],sizeof(uint32_t), 1, fp);
							//ostr << "\t0x" << std::hex << std::setw(8) << std::setfill('0') << *((uint32_t *)(&(dataPtr[l]))) << std::endl;
						}
					}

                    int payload_size = ( (dataHeader->GetByteCount() - 16)/4);
					//ostr << payload_size << std::endl;
					//ostr << hit_in[ii%64] * 8 << std::endl;
                    if(j==0 and payload_size != hit_in[ii%64] * 8){
								ostr << "##################################################" << std::endl;
								ostr << "Wrong size of pattern at event " << ii << std::endl;
								ostr << "##################################################" << std::endl;
					}

					// print the data block payload raw data
					{
						auto dataPtr = reinterpret_cast<const uint8_t*>(dataBlocks[j].GetData());
						//if(displayPayloadAtGUI) ostr << "Data payload:" << std::endl;
						for (int l = 0; l < dataHeader->GetByteCount() - 16; l+=4)
						{
                          

                            uint32_t pdata = *((uint32_t *)(&(dataPtr[l])));
                            //ostr << last_data << "   " << pdata << std::endl;

							if(j==0 and pdata != last_data+1){

								ostr << "##################################################" << std::endl;
								ostr << "Wrong number in the pattern stream of event " << ii << std::endl;
								ostr << "##################################################" << std::endl;
                                
								last_data = 0;
								for(unsigned int i=0; i<=ii%64; i++){
								  last_data += (hit_in[i] * 8) - 1;
								}

							}
							else{
								last_data = pdata;
							}

							if(fp) fwrite(&dataPtr[l],sizeof(uint32_t), 1, fp);
							//if(displayPayloadAtGUI) ostr << "\t0x" << std::hex << std::setw(8) << std::setfill('0') << *((uint32_t *)(&(dataPtr[l]))) << std::endl;
						}
					}

				} //end Data Block ROC fragment loop
				
				ostr << std::endl << std::endl;
			} //end Sub Event loop (should only be one Sub Event)
		} //end primary Sub Event loop
	} // end Sub Event handling

	if(fp) fclose(fp); //close binary file

	// print the result
	std::stringstream outSs;
	outSs << "Number of events  requested: " + 
		std::to_string(numberOfEvents) <<  " from starting event tag " << timestampStart << __E__;
	outSs << "Active Event Match: " << (activeMatch?"true":"false") << __E__;
	// outSs << "Event Duration: " << cfoDelay << " = " << cfoDelay*25 << " ns" << __E__;
	// outSs << "Reading back: " << (doNotReadBack?"false":"true") << __E__;
	if(fp) outSs << "Binary data file saved at: " << filename << __E__;
	outSs << ostr.str();

	std::cout << "Untruncated output: \n" << outSs.str() << __E__; //for no truncation!

	__SET_ARG_OUT__("response", outSs.str());
	// delete cfo;
} //end PatternTest()



//========================================================================
void DTCFrontEndInterface::ManualLoopbackSetup(__ARGS__)
{	
	bool setAsPassthrough = __GET_ARG_IN__("setAsPassthrough",bool);
	const int ROC_Link = __GET_ARG_IN__("ROC_Link",int);

	__COUTV__(setAsPassthrough);
	__COUTV__(ROC_Link);
	
	thisDTC_->EnableLink(DTCLib::DTC_Link_CFO);
	thisDTC_->DisableLink(DTCLib::DTC_Link_EVB);
	for(size_t i=0;i<DTCLib::DTC_ROC_Links.size();++i)
		thisDTC_->DisableLink(DTCLib::DTC_ROC_Links[i]);

	if(setAsPassthrough)
	{
		thisDTC_->DisableCFOLoopback();
		return;
	}

	thisDTC_->EnableLink(DTCLib::DTC_ROC_Links[ROC_Link]);
	

} //end ManualLoopbackSetup()

//========================================================================
void DTCFrontEndInterface::loopbackTest(int step)
{	
	// TODO: read from configuration
	const int ROCsPerDTC = 6;
	const unsigned int DTCsPerChain = 8; //getConfigurationManager()
	        							//->getNode("/Mu2eGlobalsTable/SyncDemoConfig/DTCsPerChain").getValue<unsigned int>();
	
	unsigned int n_steps = DTCsPerChain * ROCsPerDTC;	// 6 * 10 = 60

	//call virtual readStatus
	if (step == -1)
		step = getIterationIndex();	// get the current index
	
	// alternate with the CFO
	if ((step % 2) != 0)
	{
		indicateIterationWork();
		__FE_COUT__ << "Step " << step << " is odd, letting DTCs have a turn" << __E__;
		return;
	}
	unsigned int loopback_step = step / 2;
	// end by restoring the status of the registers
	if (loopback_step >= n_steps)	
	{
		__FE_COUT__ << "Loopback over!" << __E__;
		return;
	}

	// select the active DTC AND ROC
	int active_DTC = loopback_step / ROCsPerDTC;	// each DTC can have up to 6 ROCs
	int active_ROC = loopback_step % ROCsPerDTC; // [0,5] possible link of the DTC

	__FE_COUT__ << "step " << loopback_step << ") active DTC: " << active_DTC 
				<< " active ROC on link: " << active_ROC << __E__;

	// set up the DTC based on its position in the chain
	if (active_DTC == dtc_location_in_chain_)
	{	
		// 0x9100 set bit 28 = 1
		__FE_COUT__ << "DTC" << active_DTC << "loopback mode ENABLE" << __E__;
		thisDTC_->DisableCFOLoopback();
		// thisDTC_->EnableCFOLoopback();
	}
	else 
	{
		// 0x9100 set bit 28 = 0
		__FE_COUT__ << "active DTC = " << active_DTC
	 	            << " is NOT this DTC = " << dtc_location_in_chain_
	 	        	<< "... pass signal through" << __E__;
		thisDTC_->EnableCFOLoopback();
		// thisDTC_->DisableCFOLoopback();
	}
	// enable the links of the DTC
	DTCLib::DTC_Link_ID link = static_cast<DTCLib::DTC_Link_ID>(active_ROC);
	thisDTC_->EnableReceiveCFOLink();
	thisDTC_->EnableTransmitCFOLink();
	thisDTC_->EnableLink(link, DTCLib::DTC_LinkEnableMode(true, true)); // enable Tx and Rx

	indicateIterationWork();	
} //end loopbackTest()


DEFINE_OTS_INTERFACE(DTCFrontEndInterface)
