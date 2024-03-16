#include "otsdaq-mu2e/FEInterfaces/CFOFrontEndInterface.h"
#include "otsdaq/Macros/InterfacePluginMacros.h"
//#include "otsdaq/DAQHardware/FrontEndHardwareTemplate.h"
//#include "otsdaq/DAQHardware/FrontEndFirmwareTemplate.h"

//#include "mu2e_driver/mu2e_mmap_ioctl.h"	// m_ioc_cmd_t

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "CFOFrontEndInterface"

//===========================================================================================
CFOFrontEndInterface::CFOFrontEndInterface(
    const std::string&       interfaceUID,
    const ConfigurationTree& theXDAQContextConfigTree,
    const std::string&       interfaceConfigurationPath)
    : CFOandDTCCoreVInterface(
          interfaceUID, theXDAQContextConfigTree, interfaceConfigurationPath)
{
	__FE_COUT__ << "instantiate CFO... " << getInterfaceUID() << " "
	            << theXDAQContextConfigTree << " " << interfaceConfigurationPath << __E__;

	std::string expectedDesignVersion = "";
	auto        mode                  = DTCLib::DTC_SimMode_NoCFO;

	__COUT__ << "CFO arguments..." << std::endl;
	__COUTV__(mode);
	__COUTV__(deviceIndex_);
	__COUTV__(expectedDesignVersion);
	__COUT__ << "END CFO arguments..." << std::endl;
	//Note: if we do not skip init, then the CFO::SetSimMode writes registers!
	thisCFO_ = new CFOLib::CFO(
		mode, deviceIndex_, expectedDesignVersion, 
		true /*skipInit*/, getInterfaceUID());

	registerFEMacros();

	try //attempt to print out firmware version to the log
	{
		std::string designVersion = thisCFO_->ReadDesignVersion();
		__FE_COUTV__(designVersion);
	} 
	catch (...) //hide exception to finish instantiation (likely exception is from a need to reset PCIe)
	{
		__FE_COUT_WARN__ << "Failed to read the firmware version, likely a PCIe reset is needed!" << __E__;
	} 

	__FE_COUT_INFO__ << "CFO instantiated with name: " << getInterfaceUID()
	            << " talking to /dev/mu2e" << deviceIndex_ << __E__;
	__FE_COUT__ << "Linux Kernel Driver Version: " << thisCFO_->GetDevice()->get_driver_version() << __E__;
}  // end constructor()

//===========================================================================================
CFOFrontEndInterface::~CFOFrontEndInterface(void)
{
	delete thisCFO_;
	__FE_COUT__ << "Destructed." << __E__;
}  // end destructor()

//==============================================================================
void CFOFrontEndInterface::registerFEMacros(void)
{
	__FE_COUT__ << "Registering CFO FE Macros..." << __E__;	

	mapOfFEMacroFunctions_.clear();

	// clang-format off

	// registerFEMacroFunction(
	// 	"Get Firmware Version",  // feMacroName
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&CFOFrontEndInterface::GetFirmwareVersion),  // feMacroFunction
	// 				std::vector<std::string>{},
	// 				std::vector<std::string>{"Firmware Version Date"},  // namesOfOutputArgs
	// 				1,   //"allUsers:0 | TDAQ:255");
	// 				"*",  /* allowedCallingFEs */
	// 				"Read the modification date of the CFO firmware using <b>MON/DD/20YY HH:00</b> format."
	// );
					
	// registerFEMacroFunction(
	// 	"Flash_LEDs",  // feMacroName
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&CFOFrontEndInterface::FlashLEDs),  // feMacroFunction
	// 				std::vector<std::string>{},
	// 				std::vector<std::string>{},  // namesOfOutputArgs
	// 				1);                          // requiredUserPermissions
    
	

	// registerFEMacroFunction(
	// 	"Get Status",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&CFOFrontEndInterface::GetStatus),            // feMacroFunction
	// 				std::vector<std::string>{},  // namesOfInputArgs
	// 				std::vector<std::string>{"Status"},
	// 				1,   // requiredUserPermissions 
	// 				"*",  // allowedCallingFEs
	// 				"Reads and displays all registers in a print friendly format."
	// );

	registerFEMacroFunction(
		"CFO Reset",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&CFOFrontEndInterface::CFOReset),
					std::vector<std::string>{},
					std::vector<std::string>{},
					1,  // requiredUserPermissions
					"*",  // allowedCallingFEs
					"Executes a soft reset of the CFO by setting the reset bit (31) to true on the <b>CFO Control Register</b> (0x9100)." 
	);

	registerFEMacroFunction(
		"CFO Halt",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&CFOFrontEndInterface::CFOHalt),
					std::vector<std::string>{},
					std::vector<std::string>{},
					1,  // requiredUserPermissions
					"*",
					"Transitions the state machine to <b>Halt</b> by setting the Enable Beam Off Mode Register to off."
	);

	registerFEMacroFunction(
		"CFO Write",  // feMacroName
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&CFOFrontEndInterface::WriteCFO),  // feMacroFunction
					std::vector<std::string>{"address", "writeData"},
					std::vector<std::string>{},  // namesOfOutput
					1,    // requiredUserPermissions
					"*",  // allowedCallingFEs
					"This FE Macro writes to the CFO registers."
	);

	registerFEMacroFunction(
		"Loopback Test",  // feMacroName
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&CFOFrontEndInterface::LoopbackTest),  // feMacroFunction
					std::vector<std::string>{"loopbacks", "link", "delay"},
					std::vector<std::string>{"Response"},  // namesOfOutput
					1,
					"*", 
					"Similar to <b>Test Loopback marker</b>, this FE Macro repeatedly measures the delay of markers from ROCs for a specified link. "
					"The average delay is returned given the number of iterations (loopbacks), link, and delay (sleep between iterations). "
					"This FE Macro is useful for Event Window synchronization.\n\n"
					"Constraints:\n"
					"\t-Loopback must be less than 10,000.\n"
	); 

	registerFEMacroFunction(
		"Test Loopback marker",  // feMacroName
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&CFOFrontEndInterface::TestMarker),  // feMacroFunction
					std::vector<std::string>{"DTC-chain link index (0-7)"},
					std::vector<std::string>{"Response"},  // namesOfOutput
					1,
					"*",
					"This FE Macro measures the delay of a marker from ROCs. "
					"Optionally, the delay can be measured over mutliple iterations with the <b>Loopback Test</B> Macro."
	); 

	registerFEMacroFunction(
		"CFO Read",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&CFOFrontEndInterface::ReadCFO),                  // feMacroFunction
					std::vector<std::string>{"address"},  // namesOfInputArgs
					std::vector<std::string>{"readData"},
					1,  // requiredUserPermissions
					"*", 
					"Read from the CFO Memory Map.\n\n"
					"Parameters:\n"
					"\taddress (uint16_t): Address in Memory Map.\n"
	); 

	// registerFEMacroFunction(
	// 	"Select Jitter Attenuator Source",
	// 		static_cast<FEVInterface::frontEndMacroFunction_t>(
	// 				&CFOFrontEndInterface::SelectJitterAttenuatorSource),
	// 			        std::vector<std::string>{"Source (0 is Local oscillator, 1 is RTF Copper Clock)",
	// 											"DoNotSet",
	// 											"AlsoResetJA"},
	// 					std::vector<std::string>{"Register Write Results"},
	// 				1, // requiredUserPermissions
	// 				"*",
	// 				"The Jitter Attenuator is used to remove jitter, or variation in the timing of signals. "
	// 				"Select the source of the jitter attenuator: a local oscilator on the CFO or the RTF.\n"
	// 				"The RTF (RJ45 Timing Fanout) is a separate board to alleviate jitter accumulation. <b>Not all DTCs are connected to the RTF</b>."
	// ); 

	registerFEMacroFunction(
		"Reset Runplan",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&CFOFrontEndInterface::ResetRunplan),                  // feMacroFunction
					std::vector<std::string>{},  // namesOfInputArgs
					std::vector<std::string>{},
					1,   // requiredUserPermissions					
					"*",
					"Resets the Event Building run plan by setting the reset bit (27) to true on the <b>CFO Control Register</b>."
	);

	registerFEMacroFunction(
		"Compile Runplan",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&CFOFrontEndInterface::CompileRunplan),                  // feMacroFunction
					std::vector<std::string>{"Input Text File", "Output Binary File"},//"Input Text Run Plan", "Output Binary Run File"},  // namesOfInputArgs
					std::vector<std::string>{"Result"},
					1,    // requiredUserPermissions	
					"*" /* allowedCallingFEs */,
					"This FE Macro compiles the CFO run plan to a binary file. You must compile before running <b>Set Runplan</b> "
					"which downloads the binary run plan to the CFO.\n\nDefault text run plan: srcs/mu2e_pcie_utils/cfoInterfaceLib/Command.txt\nDefault binary run plan: srcs/mu2e_pcie_utils/cfoInterfaceLib/Command.bin" /* feMacroTooltip */
					); 

	registerFEMacroFunction(
		"Set Runplan",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&CFOFrontEndInterface::SetRunplan),                  // feMacroFunction
					std::vector<std::string>{"Binary Run File"},         // namesOfInputArgs
					std::vector<std::string>{"Result"},
					1,   // requiredUserPermissions	
					"*", /* allowedCallingFEs */
					"Download the binary run plan to the CFO. <b>You must first compile your run plan</b>.\n\n\n\n" /* feMacroTooltip */
					"Paramters:\n" 
					"\tBinary Run File (string): Path to the binary run plan. Default: srcs/mu2e_pcie_utils/cfoInterfaceLib/Commands.bin\n"
	);

	registerFEMacroFunction(
		"Launch Runplan",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&CFOFrontEndInterface::LaunchRunplan),                  // feMacroFunction
					std::vector<std::string>{},  // namesOfInputArgs
					std::vector<std::string>{},
					1,   // requiredUserPermissions	 
					"*" /* allowedCallingFEs */,
					"Launchs the Event Building run plan. You must <b>Compile Runplan</b> and <b>Set Runplan</b> before launching. " /* feMacroTooltip */
					"You do not need to compile and set the same runplan more than once. Use <b>Reset Runplan</b> and <b>Launch Runplan</b> thereafter."
	);
	
	registerFEMacroFunction(
		"Configure for Timing Chain",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&CFOFrontEndInterface::ConfigureForTimingChain),                  // feMacroFunction
					std::vector<std::string>{"StepIndex"},  // namesOfInputArgs
					std::vector<std::string>{},
					1, 
					"*", 
					"This FE Macro configures the CFO for DTC chain synchronization."
	);  // requiredUserPermissions	

	// clang-format on


	CFOandDTCCoreVInterface::registerCFOandDTCFEMacros();

} //end registerFEMacros()

// //===============================================================================================
// // registerWrite: return = value readback from register at address "address"
// //	Use base class CFOandDTCCoreVInterface::, and do readback verification in DTCFrontEndInterface::registerWrite() and CFOFrontEndInterface::registerWrite()
// dtc_data_t CFOFrontEndInterface::registerWrite(
// 	dtc_address_t address, dtc_data_t dataToWrite)
// {
// 	dtc_data_t readbackValue = CFOandDTCCoreVInterface::registerWrite(address,dataToWrite);

// 	//--------------------------------------------------------
// 	//do CFO-specific readback verification here...

// 	//end CFO-specific readback verification here...
// 	//--------------------------------------------------------

// 	return readbackValue;
// }  // end registerWrite()

// //=====================================================================================
// //
// int CFOFrontEndInterface::getLinkStatus()
// {
// 	int overall_link_status = registerRead(0x9140);

// 	int link_status = (overall_link_status >> 0) & 0x1;

// 	return link_status;
// }

//=====================================================================================
// TODO: function to do a loopback test on the specified link
uint32_t CFOFrontEndInterface::measureDelay(CFOLib::CFO_Link_ID link)
{
	// TODO: how can I understand if the measure fails?
	__FE_COUT__ << "Send loopback marker on link " << link << __E__;

	thisCFO_->ResetDelayRegister();	// reset 0x9380
	thisCFO_->DisableLinks();	// reset 0x9114
	// configure the DTC (to configure the ROC in a loop)
	thisCFO_->EnableLink(link, DTC_LinkEnableMode(true, true)); // enable Tx and Rx
	thisCFO_->EnableDelayMeasureMode(link);
	thisCFO_->EnableDelayMeasureNow(link);
	u_int32_t delay = thisCFO_->ReadCableDelayValue(link);	// read delay
	__FE_COUT__ << "Delay measured: " << delay << " (ns) on link: " <<  link << __E__;
	// reset registers
	thisCFO_->ResetDelayRegister();
	thisCFO_->DisableLinks();

	return delay;
} //end measureDelay()

// //========================================================================
// void CFOFrontEndInterface::GetFPGATemperature(__ARGS__)
// {	
// 	// rd << "Celsius: " << val << ", Fahrenheit: " << val*9/5 + 32 << ", " << (val < 65?"GOOD":"BAD");
// 	std::stringstream ss;
// 	ss << thisCFO_->FormatFPGATemperature() << "\n\n" << thisCFO_->FormatFPGAAlarms();
// 	__SET_ARG_OUT__("Temperature", ss.str());
// } //end GetFPGATemperature()

//=====================================================================================
// TODO: function to do a loopback test on the specified link, handle the boadcast
void CFOFrontEndInterface::LoopbackTest(__ARGS__)
{
	__FE_COUT__ << "Operation \"Loopback test\"" << std::endl;

	// stream to print the output
	std::stringstream ostr;
	ostr << std::endl;
	
	// parameters 
	int numberOfLoopback = __GET_ARG_IN__("loopbacks", uint32_t);
	int input_link = __GET_ARG_IN__("link", uint8_t);
	int delay = __GET_ARG_IN__("delay", uint32_t);

	// config parameters (TODO: read from config)
	unsigned alignament_marker = 10;
	const int MAX_LOOPBACK = 10000;
	const int MIN_LOOPBACK = 1;

	if (numberOfLoopback > MAX_LOOPBACK)
		numberOfLoopback = MAX_LOOPBACK;

	if (numberOfLoopback < MIN_LOOPBACK)
		numberOfLoopback = MIN_LOOPBACK;

	// variable to compuet the average
	float avg_delay = 0.0;
	unsigned int comulative_delay = 0;
	// int fail_measure = 0;

	// log the input
	__FE_COUTV__(numberOfLoopback);
	__FE_COUTV__(input_link);
	__FE_COUTV__(delay);

	ostr << "Number of loopbacks: " << numberOfLoopback << std::endl;

	// TODO: read from config with special param
	if (numberOfLoopback < 0)
	{
		return;
	}
	if (input_link < 0)
	{
		return;
	}

	CFOLib::CFO_Link_ID link = static_cast<CFOLib::CFO_Link_ID>(input_link);


	// sending first marker to align the clock of the ROC
	__FE_COUT__ << "Align the ROC's clock..." <<  __E__;
	for (unsigned int n = 0; n < alignament_marker; ++n)
	{
		measureDelay(link);
	}

	// send marker
	for (int n = 0; n < numberOfLoopback; ++n)
	{
		// TODO: check if it fail
		uint32_t link_delay = measureDelay(link);
		comulative_delay += link_delay;
		__FE_COUT__ << "[" << (n+1) << "] Record: " << link_delay <<  __E__;
		usleep(delay);	// maybe not defined
	}

	avg_delay = comulative_delay / numberOfLoopback;
	ostr << "Average delay: " << avg_delay << std::endl;
	__FE_COUT__ << "Average delay: " << avg_delay << __E__;

	ostr << std::endl << std::endl;

	__SET_ARG_OUT__("Response", ostr.str());

} // end LoopbackTest()

//=====================================================================================
// TODO: function to do a loopback test on the specified link
void CFOFrontEndInterface::TestMarker(__ARGS__)
{
	__FE_COUT__ << "Operation \"Marker test\"" << std::endl;

	// stream to print the output
	std::stringstream ostr;
	ostr << std::endl;
	
	// parameters (TODO: make the default)
	int input_link = __GET_ARG_IN__("DTC-chain link index (0-7)", uint8_t);

	if (input_link < 0)
	{
		return;
	}

	__FE_COUTV__(input_link);
	CFOLib::CFO_Link_ID link = static_cast<CFOLib::CFO_Link_ID>(input_link);
	uint32_t link_delay = measureDelay(link);

	ostr << "Marker sent on link: " << link << std::endl
		 << "\t Delay: " << link_delay << std::endl;
	__FE_COUT__ << "Marker sent on link: " << link << std::endl
	 			<< "\t Delay: " << link_delay << std::endl;

	ostr << std::endl << std::endl;
	__SET_ARG_OUT__("Response", ostr.str());
} // end TestMarker()

//=====================================================================================
//
float CFOFrontEndInterface::MeasureLoopback(int linkToLoopback)
{
/* COMMENTED 20-Jun-2023 by rrivera to start using CFO_Register directly.. will need to add features to support loopback revival
	const int maxNumberOfLoopbacks = 10000;
	int       numberOfLoopbacks =
	    getConfigurationManager()
	        ->getNode("/Mu2eGlobalsTable/SyncDemoConfig/NumberOfLoopbacks")
	        .getValue<unsigned int>();

	__FE_COUTV__(numberOfLoopbacks);

	// prepare histograms
	float numerator   = 0.0;
	float denominator = 0.0;
	failed_loopback_  = 0;

	int loopback_data[maxNumberOfLoopbacks] = {};

	max_distribution_ = 0;      // maximum value of the histogram
	min_distribution_ = 99999;  // minimum value of the histogram

	for(int n = 0; n < 10000; n++)
		loopback_distribution_[n] = 0;  // zero out the histogram

	// get initial states
	unsigned initial_9380 = registerRead(0x9380);
	unsigned initial_9114 = registerRead(0x9114);

	// clean up after the DTC has done all of its resetting...
	__FE_COUT__ << "LOOPBACK: CFO reset serdes RX " << __E__;
	registerWrite(0x9118, 0x000000ff);
	registerWrite(0x9118, 0x0);
	sleep(1);

	__FE_COUT__ << "LOOPBACK: CFO status before loopback" << __E__;
	readStatus();

	//	__FE_COUT__ << "LOOPBACK: BEFORE max_distribution_: " <<
	// max_distribution_ << __E__;

	for(int n = 0; n <= numberOfLoopbacks; n++)
	{
		//----------take out of delay measurement mode
		registerWrite(0x9380, 0x00000000);

		//-------- Disable tx and rx data
		registerWrite(0x9114, 0x00000000);

		//--- enable tx and rx for link linkToLoopback
		int dataToWrite = (0x00000101 << linkToLoopback);
		registerWrite(0x9114, dataToWrite);

		//----- Put linkToLoopback in delay measurement mode
		dataToWrite = (0x00000100 << linkToLoopback);
		registerWrite(0x9380, dataToWrite);

		//------ begin delay measurement
		dataToWrite = (0x00000101 << linkToLoopback);
		registerWrite(0x9380, dataToWrite);
		usleep(5);

		//--------read delay value
		unsigned int delay = registerRead(0x9360);

		//__MCOUT_INFO__("LOOPBACK iteration " << std::dec << n << " gives " << delay <<
		//__E__);

		if(delay < 10000 && n > 5)
		{  // skip the first events since the ROC is
		   // resetting its alignment

			numerator += (float)delay;
			denominator += 1.0;
			// 		__FE_COUT__ << "LOOPBACK iteration " << std::dec << n <<
			// " gives " << delay << __E__;

			loopback_data[n] = delay;

			loopback_distribution_[delay]++;

			if(delay > max_distribution_)
			{
				max_distribution_ = delay;
				//		    __FE_COUT__ << "LOOPBACK: new max_distribution_: " <<
				// max_distribution_ << __E__;
			}

			if(delay < min_distribution_)
				min_distribution_ = delay;
		}
		else
		{
			loopback_data[n] = -999;

			if(n > 5)
			{  // skip the first events since the ROC is resetting its alignment
				failed_loopback_++;
			}
		}

		//----------clear delay measurement mode
		// registerWrite(0x9380,0x00000000);

		//-------- Disable tx and rx data
		// registerWrite(0x9114,0x00000000);

		usleep(5);
	}

	__FE_COUT__ << "LOOPBACK: CFO status after loopback" << __E__;
	readStatus();

	// return back to initial state
	registerWrite(0x9380, initial_9380);
	registerWrite(0x9114, initial_9114);

	// ---------------------------
	// do a little bit of analysis
	// ---------------------------

	average_loopback_ = -999.;
	if(denominator > 0.5)
		average_loopback_ = numerator / denominator;

	rms_loopback_ = 0.;

	for(int n = 0; n < numberOfLoopbacks; n++)
	{
		if(loopback_data[n] > 0)
		{
			rms_loopback_ += (loopback_data[n] - average_loopback_) *
			                 (loopback_data[n] - average_loopback_);
		}
	}

	if(denominator > 0.5)
	{
		rms_loopback_ = sqrt(rms_loopback_ / denominator);
		average_loopback_ *= 5;  // convert from 5ns bins (200MHz) to 1ns bins
		rms_loopback_ *= 5;      // convert from 5ns bins (200MHz) to 1ns bins
	}

	__FE_COUT__ << "LOOPBACK: distribution: " << __E__;
	//	__FE_COUT__ << "LOOPBACK: min_distribution_: " << min_distribution_ <<
	//__E__;
	//	__FE_COUT__ << "LOOPBACK: max_distribution_: " << max_distribution_ <<
	//__E__;

	for(unsigned int n = (min_distribution_ - 5); n < (max_distribution_ + 5); n++)
	{
		__MCOUT_INFO__(" delay [ " << n << " ] = " << loopback_distribution_[n] << __E__);
	}

	__MCOUT_INFO__(" average = " << average_loopback_ << " ns, RMS = " << rms_loopback_
	                             << " ns, failures = " << failed_loopback_ << __E__);

	__FE_COUT__ << __E__;

	__FE_COUT__ << "LOOPBACK: number of failed loopbacks = " << std::dec
	            << failed_loopback_ << __E__;
*/
	return average_loopback_;

}  // end MeasureLoopback()

//===============================================================================================
void CFOFrontEndInterface::configure(void)
{
	__FE_COUTV__(getIterationIndex());
	__FE_COUTV__(getSubIterationIndex());

	// if(regWriteMonitorStream_.is_open())
	// {
	// 	regWriteMonitorStream_ << "Timestamp: " << std::dec << time(0) << 
	// 		", \t ---------- Start configure step " << 
	// 		getIterationIndex() << ":" << getSubIterationIndex() << "\n";
	// 	regWriteMonitorStream_.flush();
	// }

	if(skipInit_) return;

	if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_HARDWARE_DEV)
	{
		__FE_COUT_INFO__ << "Not configuring CFO for hardware development mode!" << __E__;
		return;
	}
	else if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_EVENT_BUILDING ||
			operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_LOOPBACK)
	{
		__FE_COUT_INFO__ << "Configuring for Event Building mode!" << __E__;
		configureEventBuildingMode();
	}
	// else if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_LOOPBACK)
	// {
	// 	__FE_COUT_INFO__ << "Configuring for Loopback mode!" << __E__;
	// 	configureEventBuildingMode();
	// }
	else
	{
		__FE_SS__ << "Unknown system operating mode: " << operatingMode_ << __E__
		          << " Please specify a valid operating mode in the 'Mu2eGlobalsTable.'"
		          << __E__;
		__FE_SS_THROW__;
	}

	return;




	// NOTE: otsdaq/xdaq has a soap reply timeout for state transitions.
	// Therefore, break up configuration into several steps so as to reply before
	// the time out As well, there is a specific order in which to configure the
	// links in the chain of CFO->DTC0->DTC1->...DTCN

	const int number_of_system_configs =
	    2;  // if < 0, keep trying until links are OK.
	        // If > 0, go through configuration steps this many times
	int       config_clock = configure_clock_;  // 1 = yes, 0 = no
	const int reset_tx     = 1;                 // 1 = yes, 0 = no

	const int number_of_dtc_config_steps = 7;

	int number_of_total_config_steps =
	    number_of_system_configs * number_of_dtc_config_steps;

	int config_step = getIterationIndex();

	if(number_of_system_configs > 0)
	{
		if(config_step >= number_of_total_config_steps)  // done - exit system config
			return;
	}

	if((config_step % number_of_dtc_config_steps) == 0)
	{
		// disable outputs
		thisCFO_->DisableAllOutputs();

		// __FE_COUT__ << "CFO disable Event Start character output " << __E__;
		// registerWrite(0x9100, 0x0);		

		// __FE_COUT__ << "CFO disable serdes transmit and receive " << __E__;
		// registerWrite(0x9114, 0x00000000);

		// __FE_COUT__ << "CFO turn off Event Windows" << __E__;
		// registerWrite(0x91a0, 0x00000000);

		// __FE_COUT__ << "CFO turn off 40MHz marker interval" << __E__;
		// registerWrite(0x9154, 0x00000000);
	}
	else if((config_step % number_of_dtc_config_steps) == 1)
	{
		// reset clocks

		if(config_clock == 1 && config_step < number_of_dtc_config_steps)
		{
			// only configure the clock/crystal the first loop through...

			__MCOUT_INFO__("Step " << config_step << ": CFO reset clock..." << __E__);

			__FE_COUT__ << "CFO set crystal frequency to 156.25 MHz" << __E__;
			thisCFO_->SetSERDESOscillatorFrequency(0x09502F90);
			// registerWrite(0x9160, 0x09502F90);

			// set RST_REG bit
			thisCFO_->WriteSERDESIICInterface(
				DTC_IICSERDESBusAddress::DTC_IICSERDESBusAddress_EVB /* device */, 
				0x87 /* address */, 0x01 /* data */);
			// registerWrite(0x9168, 0x55870100);
			// registerWrite(0x916c, 0x00000001);

			// sleep(5);

			//-----begin code snippet pulled from: mu2eUtil program_clock -C 0 -F
			// 200000000 ---
			// C=0 = main board SERDES clock
			// C=1 = DDR clock
			// C=2 = Timing board SERDES clock

			int targetFrequency = 200000000;

			//auto oscillator = DTCLib::DTC_OscillatorType_SERDES;  //-C 0 = CFO (main
			                                                      // board SERDES clock)
			// auto oscillator = DTCLib::DTC_OscillatorType_DDR; //-C 1 (DDR clock)
			// auto oscillator = DTCLib::DTC_OscillatorType_Timing; //-C 2 = DTC (with
			// timing card)

			__FE_COUT__ << "CFO set oscillator frequency to " << std::dec
			            << targetFrequency << " MHz" << __E__;

			thisCFO_->SetNewOscillatorFrequency(targetFrequency);

			//-----end code snippet pulled from: mu2eUtil program_clock -C 0 -F
			// 200000000

			sleep(5);
		}
		else
		{
			__MCOUT_INFO__("Step " << config_step << ": CFO do NOT reset clock..."
			                       << __E__);
		}
	}
	else if((config_step % number_of_dtc_config_steps) == 3)
	{
		// after DTC jitter attenuator OK, config CFO SERDES PLLs and TX
		if(reset_tx == 1)
		{
			__MCOUT_INFO__("Step " << config_step << ": CFO reset TX..." << __E__);

			__FE_COUT__ << "CFO reset serdes PLLs " << __E__;
			thisCFO_->ResetAllSERDESPlls();
			// registerWrite(0x9118, 0x0000ff00);
			// registerWrite(0x9118, 0x0);
			// sleep(3);

			__FE_COUT__ << "CFO reset serdes TX " << __E__;
			thisCFO_->ResetAllSERDESTx();
			// registerWrite(0x9118, 0x00ff0000);
			// registerWrite(0x9118, 0x0);
			// sleep(3);
		}
		else
		{
			__MCOUT_INFO__("Step " << config_step << "CFO do NOT reset TX..." << __E__);
		}
	}
	else if((config_step % number_of_dtc_config_steps) == 6)
	{
		__MCOUT_INFO__("Step " << config_step
		                       << ": CFO enable Event start characters, SERDES Tx "
		                          "and Rx, and event window interval"
		                       << __E__);

		__FE_COUT__ << "CFO reset serdes RX " << __E__;
		thisCFO_->ResetSERDES(CFOLib::CFO_Link_ID::CFO_Link_ALL);
		// registerWrite(0x9118, 0x000000ff);
		// registerWrite(0x9118, 0x0);
		// sleep(3);

		__FE_COUT__ << "CFO enable Event Start character output " << __E__;
		thisCFO_->EnableEmbeddedClockMarker();
		thisCFO_->EnableAcceleratorRF0();
		// registerWrite(0x9100, 0x5); //bit-0 is clock enable, bit-2 enables accelerator RF-0 input

		__FE_COUT__ << "CFO enable serdes transmit and receive " << __E__;
		thisCFO_->EnableLink(CFOLib::CFO_Link_ID::CFO_Link_ALL);
		// registerWrite(0x9114, 0x0000ffff);

		__FE_COUT__ << "CFO Event Window interval time now controlled by CFO Run Plan, as of Firmware version: Nov/09/2023 11:00" << __E__;
		// thisCFO_->SetEventWindowEmulatorInterval(0x1f40 /* 40us */); //0x154 = 1.7us, 0x1f40 = 40us, 0 = NO markers
		//    registerWrite(0x91a0,0x154);   //1.7us
		// registerWrite(0x91a0, 0x1f40);  // 40us
		// 	registerWrite(0x91a0,0x00000000); 	// for NO markers, write these
		// values

		__FE_COUT__ << "CFO set 40MHz marker interval" << __E__;
		thisCFO_->SetClockMarkerIntervalCount(0x0800);  // 0 = NO markers
		// registerWrite(0x9154, 0x0800);
		// 	registerWrite(0x9154,0x00000000); 	// for NO markers, write these
		// values

		__FE_COUT_INFO__ << "--------------" << __E__;
		__FE_COUT_INFO__ << "CFO configured" << __E__;

		if(thisCFO_->ReadSERDESRXCDRLock(CFOLib::CFO_Link_ID::CFO_Link_0))
		{
			__FE_COUT_INFO__ << "CFO links OK \n" << thisCFO_->FormatSERDESRXCDRLock() << __E__;
			// __MCOUT_INFO__("CFO links OK = 0x" << std::hex << registerRead(0x9140)
			//                                    << std::dec << __E__);

			if(number_of_system_configs < 0)
			{
				return;  // links OK, kick out
			}
		}
		else
		{
			__FE_COUT_INFO__ << "CFO links not OK \n" << thisCFO_->FormatSERDESRXCDRLock() << __E__;
			// __MCOUT_INFO__("CFO links not OK = 0x" << std::hex << registerRead(0x9140)
			//                                        << std::dec << __E__);
		}
		__FE_COUT__ << __E__;
	}

	__FE_COUT__ << "\n" << thisCFO_->FormattedRegDump(120, thisCFO_->formattedDumpFunctions_);  // spit out link status at every step
	indicateIterationWork();  // I still need to be touched
	return;
} //end configure()

//==============================================================================
void CFOFrontEndInterface::configureEventBuildingMode(int step)
{
	if(step == -1)
		step = getIterationIndex();

	__FE_COUT_INFO__ << "configureEventBuildingMode() " << step << __E__;
	
	if(step < CFOandDTCCoreVInterface::CONFIG_DTC_TIMING_CHAIN_START_INDEX)
	{
		// in order to start from zero
		if(timing_chain_first_substep_ == -1)
			timing_chain_first_substep_ = getSubIterationIndex();

		configureForTimingChain();
		indicateIterationWork();
	}
	else if(step < CFOandDTCCoreVInterface::CONFIG_DTC_TIMING_CHAIN_START_INDEX + 
		CFOandDTCCoreVInterface::CONFIG_DTC_TIMING_CHAIN_STEPS)
	{
		__FE_COUT__ << "Do nothing while DTCs finish configureForTimingChain..." << __E__;
		indicateIterationWork();
	}
	else if(step == CFOandDTCCoreVInterface::CONFIG_DTC_TIMING_CHAIN_START_INDEX + 
		CFOandDTCCoreVInterface::CONFIG_DTC_TIMING_CHAIN_STEPS)
	{
		__FE_COUT__ << "CFO reset serdes TX " << __E__;
		thisCFO_->ResetAllSERDESTx();
		indicateIterationWork();
	}
	else if(step == 1 + CFOandDTCCoreVInterface::CONFIG_DTC_TIMING_CHAIN_START_INDEX + 
		CFOandDTCCoreVInterface::CONFIG_DTC_TIMING_CHAIN_STEPS)
	{ 
		__FE_COUT__ << "Enable communication over links" << __E__;
		thisCFO_->EnableEmbeddedClockMarker();
		thisCFO_->EnableAcceleratorRF0();

		thisCFO_->EnableLink(CFOLib::CFO_Link_ID::CFO_Link_ALL);

		__FE_COUT__ << "CFO Event Window interval time now controlled by CFO Run Plan, as of Firmware version: Nov/09/2023 11:00" << __E__;		
		//thisCFO_->SetEventWindowEmulatorInterval(0x1f40 /* 40us */);

		__FE_COUT__ << "CFO set 40MHz marker interval" << __E__;
		thisCFO_->SetClockMarkerIntervalCount(0x0800);  // 0 = NO markers
	}
	else
		__FE_COUT__ << "Do nothing while other configurable entities finish..." << __E__;
	

}  // end configureEventBuildingMode()

//==============================================================================
void CFOFrontEndInterface::configureLoopbackMode(int step)
{
	__FE_COUT__ << "The loopback is performed in the strat phase." << __E__;
	// if(step == -1)
	// 	step = getIterationIndex();

	// __FE_COUT_INFO__ << "configureLoopbackMode() " << step << "." << getSubIterationIndex() << __E__;
	
	// if(step < CFOandDTCCoreVInterface::CONFIG_DTC_TIMING_CHAIN_START_INDEX)
	// {
	// 	if(timing_chain_first_substep_ == -1)
	// 			timing_chain_first_substep_ = getSubIterationIndex();
	// 	configureForTimingChain();
	// 	indicateIterationWork();
	// }
	// else if(step < CFOandDTCCoreVInterface::CONFIG_DTC_TIMING_CHAIN_START_INDEX + 
	// 	CFOandDTCCoreVInterface::CONFIG_DTC_TIMING_CHAIN_STEPS)
	// {
	// 	__FE_COUT__ << "Do nothing while DTCs finish configureForTimingChain..." << __E__;
	// 	indicateIterationWork();
	// }
	// else if(step == CFOandDTCCoreVInterface::CONFIG_DTC_TIMING_CHAIN_START_INDEX + 
	// 	CFOandDTCCoreVInterface::CONFIG_DTC_TIMING_CHAIN_STEPS)
	// {
	// 	__FE_COUT__ << "CFO reset serdes TX " << __E__;
	// 	thisCFO_->ResetAllSERDESTx();
	// 	indicateIterationWork();
	// }
	// else if(step == 1 + CFOandDTCCoreVInterface::CONFIG_DTC_TIMING_CHAIN_START_INDEX + 
	// 	CFOandDTCCoreVInterface::CONFIG_DTC_TIMING_CHAIN_STEPS)
	// { 
	// 	__FE_COUT__ << "Enable communication over links" << __E__;
	// 	thisCFO_->EnableTiming();
	// 	thisCFO_->EnableEventWindowInput();

	// 	thisCFO_->EnableLink(CFOLib::CFO_Link_ID::CFO_Link_ALL);

	// 	__FE_COUT__ << "CFO set beam off Event Window interval time" << __E__;
	// 	thisCFO_->SetEventWindowEmulatorInterval(0x1f40 /* 40us */);

	// 	__FE_COUT__ << "CFO set 40MHz marker interval" << __E__;
	// 	thisCFO_->SetClockMarkerIntervalCount(0x0800);  // 0 = NO markers
	// }
	// else
	// 	__FE_COUT__ << "Do nothing while other configurable entities finish..." << __E__;
	
}  // end configureLoopbackMode()

//==============================================================================
void CFOFrontEndInterface::configureForTimingChain(int step)
{
	//use sub-iteration index (but not the value of the index)
	//	sub-iterations focus allow one entity to finish an iteration index, while others wait,
	//	but can not be sure of starting sub-iteration index from entity to entity.
	if(step == -1) step = getSubIterationIndex() - timing_chain_first_substep_;

	__FE_COUT_INFO__ << "configureForTimingChain() " << step << __E__;
	
	std::string designVersion = thisCFO_->ReadDesignDate();
	__FE_COUTV__(designVersion);
	//Jun/13/2023 16:00 raw-data: 0x23061316
	//DTC-style: Jun/13/2023 17:00 raw-data: 0x23061317

	std::string matchDesignVersion = "Jun/13/2023 16:00   raw-data: 0x23061316";
	switch(step)
	{
		case 0:
			//put CFO in known state with DTC reset and control clear
			thisCFO_->SoftReset();
			thisCFO_->ClearControlRegister();
			
			thisCFO_->DisableAllOutputs();


			__FE_COUTV__(configure_clock_);	//1

			//NOTE on Jun/13/2023 16:00 raw-data: 0x23061316
			//	need to configure crystal!

			__FE_COUT__ << "CFO Design Version:\t" << designVersion << __E__	
						<< "Expected version:\t" << matchDesignVersion << __E__
						<< "Match:\t" << (designVersion.compare(matchDesignVersion) == 0) << __E__;
			
			if(configure_clock_ && thisCFO_->ReadDesignDate() == "Jun/13/2023 16:00   raw-data: 0x23061316")
			{
				// only configure the clock/crystal the first loop through...

				__FE_COUT_INFO__ << "CFO reset clock..." << __E__;

				if(1)
				{
					__FE_COUT__ << "CFO set crystal frequency to 156.25 MHz" << __E__;
					thisCFO_->SetSERDESOscillatorFrequency(0x09502F90);
					// registerWrite(0x9160, 0x09502F90);

					// set RST_REG bit
					thisCFO_->WriteSERDESIICInterface(DTC_IICSERDESBusAddress::DTC_IICSERDESBusAddress_EVB /* device */, 
													  0x87 /* address */, 
													  0x01 /* data */);
				}

				// registerWrite(0x9168, 0x55870100);
				// registerWrite(0x916c, 0x00000001);

				// sleep(5);

				//-----begin code snippet pulled from: mu2eUtil program_clock -C 0 -F
				// 200000000 ---
				// C=0 = main board SERDES clock
				// C=1 = DDR clock
				// C=2 = Timing board SERDES clock

				int targetFrequency = 200000000;

				//auto oscillator = DTCLib::DTC_OscillatorType_SERDES;  //-C 0 = CFO (main
																	// board SERDES clock)
				// auto oscillator = DTCLib::DTC_OscillatorType_DDR; //-C 1 (DDR clock)
				// auto oscillator = DTCLib::DTC_OscillatorType_Timing; //-C 2 = DTC (with
				// timing card)

				__FE_COUT__ << "CFO set oscillator frequency to " << std::dec
							<< targetFrequency << " MHz" << __E__;

				thisCFO_->SetNewOscillatorFrequency(targetFrequency);

				//-----end code snippet pulled from: mu2eUtil program_clock -C 0 -F
				// 200000000

				sleep(5);
			} //end special behavior for "original" CFO version 0x23061316

			indicateSubIterationWork();
			break;
		case 1:
			{
				__FE_COUT__ << "CFO go to next sub-iteration! step: " << step << __E__;
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
					//For CFO - 0 ==> Local oscillator
					//For CFO - 1 ==> RTF copper clock
					thisCFO_->SetJitterAttenuatorSelect(select, true /* alsoResetJA */);					
				}
				else
					__FE_COUT_INFO__ << "Skipping configure clock." << __E__;
			}
			// indicateSubIterationWork(); //for now, not running case 2, saving ResetAllSERDESTx for after DTCs are configured
			break;
		case 2: //for now, not running case 2
		
			// __FE_COUT__ << "CFO reset serdes PLLs " << __E__;
			// thisCFO_->ResetAllSERDESPlls();

			__FE_COUT__ << "CFO reset serdes TX " << __E__;
			thisCFO_->ResetAllSERDESTx();
				
			// __FE_COUT__ << "CFO reset serdes RX " << __E__;
			// thisCFO_->ResetSERDES(CFOLib::CFO_Link_ID::CFO_Link_ALL);

			// __FE_COUT__ << "CFO enable markers on link " << __E__;
			// thisCFO_->EnableTiming();
		
			// __FE_COUT__ << "CFO enable serdes transmit and receive " << __E__;
			// thisCFO_->EnableLink(CFOLib::CFO_Link_ID::CFO_Link_ALL);

			break;
		default:
			__FE_COUT__ << "Do nothing while other configurable entities finish..." << __E__;
	}	

}  // end configureForTimingChain()

//==============================================================================
void CFOFrontEndInterface::halt(void)
{
	__FE_COUT__ << "HALT: CFO status" << __E__;


	thisCFO_->DisableBeamOffMode(CFOLib::CFO_Link_ID::CFO_Link_ALL);

	// if(regWriteMonitorStream_.is_open())
	// {
	// 	regWriteMonitorStream_ << "Timestamp: " << std::dec << time(0) << 
	// 		", \t ---------- Halting..." << "\n";
	// 	regWriteMonitorStream_.flush();
	// }

	// readStatus();
}

//==============================================================================
void CFOFrontEndInterface::pause(void)
{
	__FE_COUT__ << "PAUSE: CFO status" << __E__;
	// if(regWriteMonitorStream_.is_open())
	// {
	// 	regWriteMonitorStream_ << "Timestamp: " << std::dec << time(0) << 
	// 		", \t ---------- Pausing..." << "\n";
	// 	regWriteMonitorStream_.flush();
	// }

	// readStatus();
}

//==============================================================================
void CFOFrontEndInterface::resume(void)
{
	__FE_COUT__ << "RESUME: CFO status" << __E__;
	// if(regWriteMonitorStream_.is_open())
	// {
	// 	regWriteMonitorStream_ << "Timestamp: " << std::dec << time(0) << 
	// 		", \t ---------- Resuming..." << "\n";
	// 	regWriteMonitorStream_.flush();
	// }

	// readStatus();
}

//==============================================================================
void CFOFrontEndInterface::start(std::string runNumber)  // runNumber)
{
	__FE_COUTV__(getIterationIndex());
	__FE_COUTV__(getSubIterationIndex());

	if(operatingMode_ == CFOandDTCCoreVInterface::CONFIG_MODE_LOOPBACK)
	{
		__FE_COUT_INFO__ << "Start the loopback!" << __E__;
		loopbackTest(runNumber);
		__FE_COUT_INFO__ << "End the loopback!" << __E__;
	}
	// if(regWriteMonitorStream_.is_open())
	// {
	// 	regWriteMonitorStream_ << "Timestamp: " << std::dec << time(0) << 
	// 		", \t ---------- Starting..." << "\n";
	// 	regWriteMonitorStream_.flush();
	// }

/* COMMENTED 20-Jun-2023 by rrivera to start using CFO_Register directly.. will need to add features to support loopback revival

	//bool LoopbackLock = true;
	//int  loopbackROC  = 0;

	const int numberOfChains       = 1;
	int       link[numberOfChains] = {0};

	const int numberOfDTCsPerChain = 1;  // assume 0, then 1

	const int numberOfROCsPerDTC = 1;  // assume 0, then 1

	// To do loopbacks on all CFOs, first have to setup all DTCs, then the CFO
	// (this method) work per iteration. Loop back done on all chains (in this
	// method), assuming the following order: i DTC0 DTC1 ... DTCN 0 ROC0 none ...
	// none 1 ROC1 none ... none 2 none ROC0 ... none 3 none ROC1 ... none
	// ...
	// N-1 none none ... ROC0
	// N none none ... ROC1

	int numberOfMeasurements = numberOfChains * numberOfDTCsPerChain * numberOfROCsPerDTC;

	int startIndex = getIterationIndex();

	if(startIndex == 0)  // setup
	{
		initial_9100_ = registerRead(0x9100);
		initial_9114_ = registerRead(0x9114);
		initial_91a0_ = registerRead(0x91a0);
		initial_9154_ = registerRead(0x9154);

		__FE_COUT__ << "CFO disable Event Start character output " << __E__;
		registerWrite(0x9100, 0x0);

		__FE_COUT__ << "CFO turn off Event Windows" << __E__;
		registerWrite(0x91a0, 0x00000000);

		__FE_COUT__ << "CFO turn off 40MHz marker interval" << __E__;
		registerWrite(0x9154, 0x00000000);

		__FE_COUT__ << "START: CFO status" << __E__;
		readStatus();

		for(int nChain = 0; nChain < numberOfChains; nChain++)
		{
			for(int nDTC = 0; nDTC < numberOfDTCsPerChain; nDTC++)
			{
				for(int nROC = 0; nROC < numberOfROCsPerDTC; nROC++)
				{
					delay[nChain][nDTC][nROC]        = -1;
					delay_rms[nChain][nDTC][nROC]    = -1;
					delay_failed[nChain][nDTC][nROC] = -1;
				}
			}
		}

		indicateIterationWork();  // I still need to be touched
		return;
	}

	if(startIndex > numberOfMeasurements)  // finish
	{
		__MCOUT_INFO__("-------------------------" << __E__);
		__MCOUT_INFO__("FULL SYSTEM loopback DONE" << __E__);

		for(int nChain = 0; nChain < numberOfChains; nChain++)
		{
			for(int nDTC = 0; nDTC < numberOfDTCsPerChain; nDTC++)
			{
				for(int nROC = 0; nROC < numberOfROCsPerDTC; nROC++)
				{
					__MCOUT_INFO__("chain "
					               << nChain << " - DTC " << nDTC << " - ROC " << nROC
					               << " = " << std::dec << delay[nChain][nDTC][nROC]
					               << " ns +/- " << delay_rms[nChain][nDTC][nROC] << " ("
					               << delay_failed[nChain][nDTC][nROC] << ")" << __E__);
				}
			}
		}

		float diff = delay[0][1][0] - delay[0][0][0];

		__MCOUT_INFO__("DTC1_ROC0 - DTC0_ROC0 = " << diff << __E__);
		__MCOUT_INFO__("-------------------------" << __E__);

		__FE_COUT__ << "LOOPBACK: CFO reset serdes RX " << __E__;
		registerWrite(0x9118, 0x000000ff);
		registerWrite(0x9118, 0x0);
		usleep(50);

		__FE_COUT__ << "CFO enable Event Start character output 0x" << std::hex << __E__;
		registerWrite(0x9100, initial_9100_);

		__FE_COUT__ << "CFO enable serdes transmit and receive 0x" << __E__;
		registerWrite(0x9114, initial_9114_);

		__FE_COUT__ << "CFO set Event Window interval time" << __E__;
		registerWrite(0x91a0, initial_91a0_);  // 40us

		__FE_COUT__ << "CFO set 40MHz marker interval" << __E__;
		registerWrite(0x9154, initial_9154_);

		readStatus();
		return;
	}

	//=========== Perform loopback=============

	// where are we in the procedure?
	int activeROC = (startIndex - 1) % numberOfROCsPerDTC;

	int activeDTC = -1;

	for(int nDTC = 0; nDTC < numberOfDTCsPerChain; nDTC++)
	{
		//	__FE_COUT__ << "loopback index = " << startIndex
		//		<< " nDTC = " << nDTC
		//		<< " numberOfDTCsPerChain = " << numberOfDTCsPerChain
		//		<< __E__;
		if((startIndex - 1) >= (nDTC * numberOfROCsPerDTC) &&
		   (startIndex - 1) < ((nDTC + 1) * numberOfROCsPerDTC))
		{
			//				__FE_COUT__ << "ACTIVE DTC " << nDTC <<
			//__E__;
			activeDTC = nDTC;
		}
	}

	//	__MOUT__ 	<< "loopback index = " << startIndex;
	__MCOUT_INFO__(" Looping back DTC" << activeDTC << " ROC" << activeROC << __E__);

	int chainIndex = 0;

	while((chainIndex < numberOfChains))
	{
		//__MCOUT__( "LOOPBACK: on DTC " << link[chainIndex] <<__E__);
		MeasureLoopback(link[chainIndex]);

		delay[chainIndex][activeDTC][activeROC]        = average_loopback_;
		delay_rms[chainIndex][activeDTC][activeROC]    = rms_loopback_;
		delay_failed[chainIndex][activeDTC][activeROC] = failed_loopback_;

		__FE_COUT__ << "LOOPBACK: link " << link[chainIndex]
		            << " -> delay = " << delay[chainIndex][activeDTC][activeROC]
		            << " ns,  rms = " << delay_rms[chainIndex][activeDTC][activeROC]
		            << " failed = " << delay_failed[chainIndex][activeDTC][activeROC]
		            << __E__;

		chainIndex++;

	}  // (chainIndex < numberOfChains)

	indicateIterationWork();  // I still need to be touched
	return;
*/
}

//==============================================================================
void CFOFrontEndInterface::stop(void)
{
	// if(regWriteMonitorStream_.is_open())
	// {
	// 	regWriteMonitorStream_ << "Timestamp: " << std::dec << time(0) << 
	// 		", \t ---------- Stopping..." << "\n";
	// 	regWriteMonitorStream_.flush();
	// }

	int numberOfCAPTANPulses =
	    getConfigurationManager()
	        ->getNode("/Mu2eGlobalsTable/SyncDemoConfig/NumberOfCAPTANPulses")
	        .getValue<unsigned int>();

	__FE_COUTV__(numberOfCAPTANPulses);

	if(numberOfCAPTANPulses == 0)
	{
		return;
	}

	int loopbackIndex = getIterationIndex();

	if(loopbackIndex > numberOfCAPTANPulses)
	{
		//---- begin read in data

		std::string filein1 = "/home/mu2edaq/sync_demo/ots/DTC0_ROC0data.txt";
		std::string filein2 = "/home/mu2edaq/sync_demo/ots/DTC1_ROC0data.txt";

		// file 1
		std::ifstream in1;

		int iteration_source1[10000];
		int timestamp_source1[10000];

		in1.open(filein1);

		//  std::cout << filein1 << std::endl;

		int nlines1 = 0;
		while(1)
		{
			in1 >> iteration_source1[nlines1] >> timestamp_source1[nlines1];
			if(!in1.good())
				break;
			if(nlines1 < 10)
				__FE_COUT__ << "iteration " << iteration_source1[nlines1] << " "
				            << timestamp_source1[nlines1] << __E__;
			nlines1++;
		}

		in1.close();

		// file 2
		std::ifstream in2;

		int iteration_source2[10000];
		int timestamp_source2[10000];

		in2.open(filein2);

		//  std::cout << filein1 << std::endl;

		int nlines2 = 0;
		while(1)
		{
			in2 >> iteration_source2[nlines2] >> timestamp_source2[nlines2];
			if(!in2.good())
				break;
			if(nlines2 < 10)
				__FE_COUT__ << "iteration " << iteration_source2[nlines2] << " "
				            << timestamp_source2[nlines2] << __E__;
			nlines2++;
		}

		in2.close();

		__FE_COUT__ << "Read in " << nlines1 << " lines from " << filein1 << __E__;
		__FE_COUT__ << "Read in " << nlines2 << " lines from " << filein2 << __E__;

		__FE_COUT__ << __E__;

		//__MCOUT_INFO__("iter file1  file2  diff" << __E__);

		int distribution[1001] = {};

		int max_distribution = -1000;
		int min_distribution = 1000;

		int offset = 500;

		int timestamp_diff[1000] = {};

		float numerator   = 0.;
		float denominator = 0.;

		for(int i = 0; i < nlines1; i++)
		{
			if(timestamp_source1[i] == 65535 || timestamp_source2[i] == 65535 ||
			   timestamp_source1[i] == -999 || timestamp_source2[i] == -999)
			{
				timestamp_diff[i] = -999999;
			}
			else
			{
				timestamp_diff[i] =
				    (timestamp_source2[i] - timestamp_source1[i]) + offset;

				if(timestamp_diff[i] >= 0 &&
				   timestamp_diff[i] < 1000)  // crossed from one event window to another
				{
					numerator += (float)timestamp_diff[i];
					denominator += 1.0;

					distribution[timestamp_diff[i]]++;

					if(timestamp_diff[i] > max_distribution)
					{
						max_distribution = timestamp_diff[i];
						__COUT__ << i << " new max    " << timestamp_source1[i] << "   "
						         << timestamp_source2[i] << "   " << timestamp_diff[i]
						         << __E__;
					}

					if(timestamp_diff[i] < min_distribution)
					{
						__COUT__ << i << " new min    " << timestamp_source1[i] << "   "
						         << timestamp_source2[i] << "   " << timestamp_diff[i]
						         << __E__;

						min_distribution = timestamp_diff[i];
					}
				}
				else
				{
					timestamp_diff[i] = -999999;
				}
			}
		}
		float average = numerator / denominator;

		float rms = 0.;

		for(int n = 0; n < nlines1; n++)
		{
			if(timestamp_diff[n] != -999999)
			{
				rms += (timestamp_diff[n] - average) * (timestamp_diff[n] - average);
			}
		}

		if(denominator > 0.0)
			rms = sqrt(rms / denominator);

		average -= offset;

		//    __FE_COUT__ << "LOOPBACK: min_distribution_: " << min_distribution_ <<
		//    __E__;
		//    __FE_COUT__ << "LOOPBACK: max_distribution_: " << max_distribution_ <<
		//    __E__;

		__MCOUT_INFO__("--------------------------------------------" << __E__);
		__MCOUT_INFO__("--CAPTAN timestamp difference distribution--" << __E__);
		for(int n = (min_distribution - 5); n < (max_distribution + 5); n++)
		{
			int display = n - offset;
			__MCOUT_INFO__(" diff [ " << display << " ] = " << distribution[n] << __E__);
		}
		__MCOUT_INFO__("--------------------------------------------" << __E__);

		__MCOUT_INFO__("Average = " << average << " ... RMS = " << rms << __E__);

		return;
	}

	indicateIterationWork();
	return;
}

//==============================================================================
bool CFOFrontEndInterface::running(void)
{
	while(WorkLoop::continueWorkLoop_)
	{
		break;
	}

	return false;
}

//========================================================================
void CFOFrontEndInterface::WriteCFO(__ARGS__)
{	
	dtc_address_t address = __GET_ARG_IN__("address", dtc_address_t);
	dtc_data_t writeData = __GET_ARG_IN__("writeData", dtc_data_t);
	__FE_COUTV__((unsigned int)address);
	__FE_COUTV__((unsigned int)writeData);

	int errorCode = getDevice()->write_register( address, 100, writeData);
	if (errorCode != 0)
	{
		__FE_SS__ << "Error writing register 0x" << std::hex << static_cast<uint32_t>(address) << " " << errorCode;
		__SS_THROW__;
	}
	// registerWrite(address, writeData);  
} //end WriteCFO()

//========================================================================
void CFOFrontEndInterface::ReadCFO(__ARGS__)
{	
	dtc_address_t address = __GET_ARG_IN__("address", dtc_address_t);
	__FE_COUTV__((unsigned int)address);
	dtc_data_t readData;// = registerRead(address);  
	
	int errorCode = getDevice()->read_register(address, 100, &readData);
	if (errorCode != 0)
	{
		__FE_SS__ << "Error reading register 0x" << std::hex << static_cast<uint32_t>(address) << " " << errorCode;
		__SS_THROW__;
	}
	
	char readDataStr[100];
	sprintf(readDataStr,"0x%X",readData);
	__SET_ARG_OUT__("readData",readDataStr);
} //end ReadCFO()

// //========================================================================
// void CFOFrontEndInterface::SelectJitterAttenuatorSource(__ARGS__)
// {
// 	uint32_t select = __GET_ARG_IN__(
// 	    "Source (0 is Local oscillator, 1 is RTF Copper Clock)", uint32_t);
// 	select %= 4;
// 	__FE_COUTV__((unsigned int)select);

// 	if(!__GET_ARG_IN__(
// 	    "DoNotSet", bool))
// 	{
// 		bool alsoResetJA = __GET_ARG_IN__(
// 	    		"AlsoResetJA", bool);
// 		__FE_COUTV__(alsoResetJA);
// 		thisCFO_->SetJitterAttenuatorSelect(select, alsoResetJA);
// 		sleep(1);
// 		for(int i=0;i<10;++i) //wait for JA to lock before reading
// 		{
// 			if(thisCFO_->ReadJitterAttenuatorLocked())
// 				break;
// 			sleep(1);
// 		}
// 	}

// 	__FE_COUT__ << "Done with jitter attenuator source select: " << select << __E__;

// 	__SET_ARG_OUT__("Register Write Results", thisCFO_->FormatJitterAttenuatorCSR());

// }  // end SelectJitterAttenuatorSource()

//========================================================================
void CFOFrontEndInterface::ResetRunplan(__ARGS__)
{	
	__FE_COUT__ << "Reset CFO Run Plan"  << __E__;

	thisCFO_->DisableBeamOnMode(CFOLib::CFO_Link_ID::CFO_Link_ALL);
	thisCFO_->DisableBeamOffMode(CFOLib::CFO_Link_ID::CFO_Link_ALL);
	// thisCFO_->ResetCFORunPlan();
	thisCFO_->SoftReset();

} //end ResetRunplan()

//========================================================================
void CFOFrontEndInterface::CompileRunplan(__ARGS__)
{	
	// to view output file with 8-byte rows
	// hexdump -e '"%08_ax " 1/8 "%016x "' -e '"\n"' srcs/mu2e_pcie_utils/cfoInterfaceLib/Commands.bin

	__FE_COUT__ << "Compile CFO Run Plan"  << __E__;

	CFOLib::CFO_Compiler compiler;

	std::ifstream inFile;
	std::ofstream outFile;

	const std::string SOURCE_BASE_PATH = std::string(__ENV__("MRB_SOURCE")) + 
		"/mu2e_pcie_utils/cfoInterfaceLib/";

	std::string inFileName  = __GET_ARG_IN__("Input Text File", std::string, SOURCE_BASE_PATH + "Commands.txt");
	std::string outFileName = __GET_ARG_IN__("Output Binary File", std::string, SOURCE_BASE_PATH + "Commands.bin");
		
	__SET_ARG_OUT__("Result", "\n" + compiler.processFile(inFileName, outFileName));

} //end CompileRunplan()

//========================================================================
void CFOFrontEndInterface::SetRunplan(__ARGS__)
{	
	//copying functionality of..
	//	cfoUtil write_program -p /home/kwar/cfo/RunplanFiveDTCs1.bin --cfo 0

	__FE_COUT__ << "Set CFO Run Plan"  << __E__;

	const std::string SOURCE_BASE_PATH = std::string(__ENV__("MRB_SOURCE")) + 
		"/mu2e_pcie_utils/cfoInterfaceLib/";
	std::string setFileName = __GET_ARG_IN__("Binary Run File", std::string, SOURCE_BASE_PATH + "Commands.bin");
	
	std::ifstream file(setFileName, std::ios::binary | std::ios::ate);
	if (file.eof())
	{
		__SS__ << ("Output File (" + setFileName + ") didn't open. Does it exist?") << __E__;
		__SS_THROW__;
		
	}
	mu2e_databuff_t inputData;
	auto inputSize = file.tellg();
	uint64_t dmaSize = static_cast<uint64_t>(inputSize) + 8;
	file.seekg(0, std::ios::beg);
	//*reinterpret_cast<uint64_t*>(inputData) = input.size();
	memcpy(&inputData[0], &dmaSize, sizeof(uint64_t));
	file.read(reinterpret_cast<char*>(&inputData[8]), inputSize);
	file.close();
	

	//set the file in hardware:
	thisCFO_->GetDevice()->write_data(DTC_DMA_Engine_DAQ, inputData, sizeof(inputData));

	std::stringstream resultSs;
	resultSs << "Downloaded to CFO binary run plan file: " << setFileName << __E__;
	__SET_ARG_OUT__("Result", resultSs.str());
} //end SetRunplan()

//========================================================================
void CFOFrontEndInterface::LaunchRunplan(__ARGS__)
{
	__FE_COUT__ << "Launch CFO Run Plan"  << __E__;

	thisCFO_->DisableBeamOffMode(CFOLib::CFO_Link_ID::CFO_Link_ALL);
	thisCFO_->DisableBeamOnMode(CFOLib::CFO_Link_ID::CFO_Link_ALL);
	thisCFO_->SoftReset();
	usleep(10);	
	thisCFO_->EnableBeamOffMode(CFOLib::CFO_Link_ID::CFO_Link_ALL);

} //end LaunchRunplan()

//========================================================================
void CFOFrontEndInterface::CFOReset(__ARGS__) { thisCFO_->SoftReset(); }

//========================================================================
void CFOFrontEndInterface::CFOHalt(__ARGS__) { halt(); }

// //==============================================================================
// // GetFirmwareVersion
//  void CFOFrontEndInterface::GetFirmwareVersion(__ARGS__)
// {	
// 	__SET_ARG_OUT__("Firmware Version Date", thisCFO_->ReadDesignDate());
// }  // end GetFirmwareVersion()

// //========================================================================
// void CFOFrontEndInterface::GetStatus(__ARGS__)
// {	
// 	__SET_ARG_OUT__("Status", thisCFO_->FormattedRegDump(20, thisCFO_->formattedDumpFunctions_));
// } //end GetStatus()

//========================================================================
void CFOFrontEndInterface::GetCounters(__ARGS__)
{	
	__SET_ARG_OUT__("Status", thisCFO_->FormattedRegDump(20, thisCFO_->formattedCounterFunctions_));
} //end GetCounters()

//========================================================================
void CFOFrontEndInterface::ConfigureForTimingChain(__ARGS__)
{	

	int stepIndex = __GET_ARG_IN__("StepIndex", int);

	// do 0, then 1
	configureForTimingChain(stepIndex);
	
} //end ConfigureForTimingChain()

//========================================================================
void CFOFrontEndInterface::loopbackTest(std::string runNumber, int step)
{	
	__FE_COUT__ << "Starting loopback test for run " << runNumber << ", step " << step << __E__;
	// TODO: read from configuratione
	
	const int ROCsPerDTC = 6;
	const unsigned int n_loopbacks = getConfigurationManager()
	        							->getNode("/Mu2eGlobalsTable/SyncDemoConfig/NumberOfLoopbacks").getValue<unsigned int>();
	const unsigned int DTCsPerChain = 8;//getConfigurationManager()
	        							//->getNode("/Mu2eGlobalsTable/SyncDemoConfig/DTCsPerChain").getValue<unsigned int>();
	
	const int alignment_marker = 10;	// TODO: check with the firmware
	unsigned int n_steps = DTCsPerChain * ROCsPerDTC;

	if (step == -1)
		step = getIterationIndex();	// get the current index

	// alternate with the DTCs
	if ((step % 2) == 0)
	{
		indicateIterationWork();
		__FE_COUT__ << "Step " << step << " is even, letting DTCs have a turn" << __E__;
		return;
	}

	unsigned int loopback_step = step / 2;
	// end by restoring the status of the registers
	if (loopback_step >= n_steps)	
	{
		__FE_COUT__ << "Loopback over!" << __E__;
		return;
	}

	// send the markers  and compute the average delay
	int active_DTC = loopback_step / ROCsPerDTC;
	int active_ROC = loopback_step % ROCsPerDTC;

	__FE_COUT__ << "step " << loopback_step << ") active DTC: " << active_DTC 
				<< " active ROC on link: " << active_ROC << __E__;
	
	// TODO: put it in a directory
	FILE* fp = 0;
	std::string filename = "/loopbackOutput_" + runNumber + ".txt";
	
	__FE_COUT__ << "Sending " << n_loopbacks << " markers on all the links..." << __E__; 
	for (auto link: CFOLib::CFO_Links)
	{
		__FE_COUT__ << "step " << loopback_step << ") CFO sending markers on" << __E__ 
							   << "CFO link:\t" << link << __E__
							   << "target DTC:\t" << active_DTC << __E__
							   << "target ROC:\t" << active_ROC << __E__; 
		unsigned int comulative_delay = 0;
		float average_delay = 0.0;
		bool timeout = false;

		// sending first marker to align the clock of the ROC
		for (unsigned int n = 0; n < alignment_marker; ++n)
		{
			measureDelay(link);
		}

		for (unsigned int n = 0; n < n_loopbacks; ++n)
		{
			std::bitset<32> delay (measureDelay(link));
			// check if the marker return timeout
			if (delay.all())
			{
				__FE_COUT__ << "Timeout link: " << link << __E__;
				timeout = true;
				break;
			}
			comulative_delay += delay.to_ulong();
			__FE_COUT__ << "step " << loopback_step << ") Delay measured on link " 
						<< link << ": " << delay.to_ulong() << __E__; 

		}
		// compute the average
		average_delay = comulative_delay / n_loopbacks;
		__FE_COUT__ << "step " << loopback_step << ") Average Delay on link " 
					<< link << ": " << average_delay << __E__; 

		// save the results on file
		try
		{
			// open the file if it is the first time
			if (!fp)
			{
				__FE_COUT__ << "File " << filename << " open." << __E__; 
				fp = fopen((std::string(__ENV__("OTSDAQ_DATA")) + filename).c_str(), "a");
			}
			// write the information
			fprintf(fp, "############################\n");
			fprintf(fp, "Chain:\t%d\n", link);
			fprintf(fp, "DTC:\t%d\n", active_DTC);
			fprintf(fp, "ROC:\t%d\n", active_ROC);
			if (timeout)
				fprintf(fp, "Delay:\tTIMEOUT\n");
			else
				fprintf(fp, "Delay:\t%f\n", average_delay);
		}
		catch(...)  // handle file close on error
		{
			if(fp)
			{
				__FE_COUT__ << "Error occurs: File close." << __E__; 
				fclose(fp);
			}
			throw;
		}
	}

	// close the file
	if(fp)
	{
		__FE_COUT__ << "File close." << __E__; 
		fclose(fp);
	}
		
	indicateIterationWork();
} //end loopbackTest()


DEFINE_OTS_INTERFACE(CFOFrontEndInterface)
