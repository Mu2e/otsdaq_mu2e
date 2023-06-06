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
	__FE_COUT__ << "instantiate CFO... " << interfaceUID << " "
	            << theXDAQContextConfigTree << " " << interfaceConfigurationPath << __E__;



	//unsigned    roc_mask              = 0x1;
	std::string expectedDesignVersion = "";
	auto        mode                  = DTCLib::DTC_SimMode_NoCFO;

	//Note: if we do not skip init, then the CFO::SetSimMode writes registers!
	thisCFO_ = new CFOLib::CFO_Registers(mode, device_, expectedDesignVersion, true /*skipInit*/);

	registerFEMacros();

	__FE_COUT_INFO__ << "CFO instantiated with name: " << device_name_
	            << " talking to /dev/mu2e" << device_ << __E__;
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

	registerFEMacroFunction(
		"CFO Write",  // feMacroName
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&CFOFrontEndInterface::WriteCFO),  // feMacroFunction
					std::vector<std::string>{"address", "writeData"},
					std::vector<std::string>{},  // namesOfOutput
					1);                          // requiredUserPermissions

	registerFEMacroFunction(
		"CFO Read",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&CFOFrontEndInterface::ReadCFO),                  // feMacroFunction
					std::vector<std::string>{"address"},  // namesOfInputArgs
					std::vector<std::string>{"readData"},
					1);  // requiredUserPermissions

	registerFEMacroFunction(
		"Reset Runplan",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&CFOFrontEndInterface::ResetRunplan),                  // feMacroFunction
					std::vector<std::string>{},  // namesOfInputArgs
					std::vector<std::string>{},
					1);  // requiredUserPermissions					

	registerFEMacroFunction(
		"Compile Runplan",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&CFOFrontEndInterface::CompileRunplan),                  // feMacroFunction
					std::vector<std::string>{},//"Input Text Run Plan", "Output Binary Run File"},  // namesOfInputArgs
					std::vector<std::string>{},
					1);  // requiredUserPermissions	

	registerFEMacroFunction(
		"Set Runplan",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&CFOFrontEndInterface::SetRunplan),                  // feMacroFunction
					std::vector<std::string>{},//"Binary Run File"},  // namesOfInputArgs
					std::vector<std::string>{},
					1);  // requiredUserPermissions	

	registerFEMacroFunction(
		"Launch Runplan",
			static_cast<FEVInterface::frontEndMacroFunction_t>(
					&CFOFrontEndInterface::LaunchRunplan),                  // feMacroFunction
					std::vector<std::string>{},  // namesOfInputArgs
					std::vector<std::string>{},
					1);  // requiredUserPermissions	
	

	// clang-format on


	CFOandDTCCoreVInterface::registerCFOandDTCFEMacros();

} //end registerFEMacros()

//===============================================================================================
// registerWrite: return = value readback from register at address "address"
//	Use base class CFOandDTCCoreVInterface::, and do readback verification in DTCFrontEndInterface::registerWrite() and CFOFrontEndInterface::registerWrite()
dtc_data_t CFOFrontEndInterface::registerWrite(
	dtc_address_t address, dtc_data_t dataToWrite)
{
	dtc_data_t readbackValue = CFOandDTCCoreVInterface::registerWrite(address,dataToWrite);

	//--------------------------------------------------------
	//do CFO-specific readback verification here...

	//end CFO-specific readback verification here...
	//--------------------------------------------------------

	return readbackValue;
}  // end registerWrite()

//=====================================================================================
std::string CFOFrontEndInterface::readStatus(void)
{
	std::stringstream ss;
	ss << "firmware version    (0x9004) = 0x" << GetFirmwareVersion() << __E__;

	ss << printVoltages() << __E__;

	ss << device_name_ << " temperature = " << readTemperature() << " degC"
	            << __E__ << __E__;

	ss << "link enable         (0x9114) = 0x" << std::hex << registerRead(0x9114) << __E__;
	ss << "SERDES reset        (0x9118) = 0x" << std::hex << registerRead(0x9118) << __E__;
	ss << "SERDES unlock error (0x9124) = 0x" << std::hex << registerRead(0x9124) << __E__;
	ss << "PLL locked          (0x9128) = 0x" << std::hex << registerRead(0x9128) << __E__;
	ss << "SERDES Rx status....(0x9134) = 0x" << std::hex << registerRead(0x9134) << __E__;
	ss << "SERDES reset done...(0x9138) = 0x" << std::hex << registerRead(0x9138) << __E__;
	ss << "SERDES Rx CDR lock..(0x9140) = 0x" << std::hex << registerRead(0x9140) << __E__;
	ss << "SERDES ref clk freq.(0x9160) = 0x" << std::hex << registerRead(0x9160) << " = " <<
		 std::dec << registerRead(0x9160) << __E__;

	__FE_COUT__ << ss.str() << __E__;

	return ss.str();
} //end readStatus()

//=====================================================================================
//
int CFOFrontEndInterface::getLinkStatus()
{
	int overall_link_status = registerRead(0x9140);

	int link_status = (overall_link_status >> 0) & 0x1;

	return link_status;
}

//=====================================================================================
//
float CFOFrontEndInterface::MeasureLoopback(int linkToLoopback)
{
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

	return average_loopback_;

}  // end MeasureLoopback()

//===============================================================================================
void CFOFrontEndInterface::configure(void)
{
	__FE_COUTV__(getIterationIndex());
	__FE_COUTV__(getSubIterationIndex());

	if(regWriteMonitorStream_.is_open())
	{
		regWriteMonitorStream_ << "Timestamp: " << std::dec << time(0) << 
			", \t ---------- Start configure step " << 
			getIterationIndex() << ":" << getSubIterationIndex() << "\n";
		regWriteMonitorStream_.flush();
	}

	try
	{
		if(getConfigurationManager()
		->getNode("/Mu2eGlobalsTable/SyncDemoConfig/SkipCFOandDTCConfigureSteps")
		.getValue<bool>())
		{
			__FE_COUT_INFO__ << "Skipping configure steps!" << __E__;
			return;
		}
	}
	catch(const std::runtime_error& e)
	{
		__FE_SS__ << "The Mu2eGlobalsTable is missing the record named 'SyncDemoConfig.' This record is required (representing Mu2e global parameters) for the configuration of the CFO." <<
			 __E__ << e.what() << __E__;
		__SS_THROW__;
	}

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

		__FE_COUT__ << "CFO disable Event Start character output " << __E__;
		registerWrite(0x9100, 0x0);

		__FE_COUT__ << "CFO disable serdes transmit and receive " << __E__;
		registerWrite(0x9114, 0x00000000);

		__FE_COUT__ << "CFO turn off Event Windows" << __E__;
		registerWrite(0x91a0, 0x00000000);

		__FE_COUT__ << "CFO turn off 40MHz marker interval" << __E__;
		registerWrite(0x9154, 0x00000000);
	}
	else if((config_step % number_of_dtc_config_steps) == 1)
	{
		// reset clocks

		if(config_clock == 1 && config_step < number_of_dtc_config_steps)
		{
			// only configure the clock/crystal the first loop through...

			__MCOUT_INFO__("Step " << config_step << ": CFO reset clock..." << __E__);
			__FE_COUT__ << "CFO set crystal frequency to 156.25 MHz" << __E__;

			registerWrite(0x9160, 0x09502F90);

			// set RST_REG bit
			registerWrite(0x9168, 0x55870100);
			registerWrite(0x916c, 0x00000001);

			sleep(5);

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
			registerWrite(0x9118, 0x0000ff00);
			registerWrite(0x9118, 0x0);
			sleep(3);

			__FE_COUT__ << "CFO reset serdes TX " << __E__;
			registerWrite(0x9118, 0x00ff0000);
			registerWrite(0x9118, 0x0);
			sleep(3);
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
		registerWrite(0x9118, 0x000000ff);
		registerWrite(0x9118, 0x0);
		sleep(3);

		__FE_COUT__ << "CFO enable Event Start character output " << __E__;
		registerWrite(0x9100, 0x5); //bit-0 is clock enable, bit-2 enables accelerator RF-0 input

		__FE_COUT__ << "CFO enable serdes transmit and receive " << __E__;
		registerWrite(0x9114, 0x0000ffff);

		__FE_COUT__ << "CFO set Event Window interval time" << __E__;
		//    registerWrite(0x91a0,0x154);   //1.7us
		registerWrite(0x91a0, 0x1f40);  // 40us
		// 	registerWrite(0x91a0,0x00000000); 	// for NO markers, write these
		// values

		__FE_COUT__ << "CFO set 40MHz marker interval" << __E__;
		registerWrite(0x9154, 0x0800);
		// 	registerWrite(0x9154,0x00000000); 	// for NO markers, write these
		// values

		__MCOUT_INFO__("--------------" << __E__);
		__MCOUT_INFO__("CFO configured" << __E__);

		if(getLinkStatus() == 1)
		{
			__MCOUT_INFO__("CFO links OK = 0x" << std::hex << registerRead(0x9140)
			                                   << std::dec << __E__);

			if(number_of_system_configs < 0)
			{
				return;  // links OK, kick out
			}
		}
		else
		{
			__MCOUT_INFO__("CFO links not OK = 0x" << std::hex << registerRead(0x9140)
			                                       << std::dec << __E__);
		}
		__FE_COUT__ << __E__;
	}

	readStatus();             // spit out link status at every step
	indicateIterationWork();  // I still need to be touched
	return;
}

//==============================================================================
void CFOFrontEndInterface::halt(void)
{
	__FE_COUT__ << "HALT: CFO status" << __E__;
	if(regWriteMonitorStream_.is_open())
	{
		regWriteMonitorStream_ << "Timestamp: " << std::dec << time(0) << 
			", \t ---------- Halting..." << "\n";
		regWriteMonitorStream_.flush();
	}

	readStatus();
}

//==============================================================================
void CFOFrontEndInterface::pause(void)
{
	__FE_COUT__ << "PAUSE: CFO status" << __E__;
	if(regWriteMonitorStream_.is_open())
	{
		regWriteMonitorStream_ << "Timestamp: " << std::dec << time(0) << 
			", \t ---------- Pausing..." << "\n";
		regWriteMonitorStream_.flush();
	}

	readStatus();
}

//==============================================================================
void CFOFrontEndInterface::resume(void)
{
	__FE_COUT__ << "RESUME: CFO status" << __E__;
	if(regWriteMonitorStream_.is_open())
	{
		regWriteMonitorStream_ << "Timestamp: " << std::dec << time(0) << 
			", \t ---------- Resuming..." << "\n";
		regWriteMonitorStream_.flush();
	}

	readStatus();
}

//==============================================================================
void CFOFrontEndInterface::start(std::string)  // runNumber)
{
	if(regWriteMonitorStream_.is_open())
	{
		regWriteMonitorStream_ << "Timestamp: " << std::dec << time(0) << 
			", \t ---------- Starting..." << "\n";
		regWriteMonitorStream_.flush();
	}

	__MCOUT_INFO__("CFO Ignoring loopback for now..." << __E__);
	return;

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
}

//==============================================================================
void CFOFrontEndInterface::stop(void)
{
	if(regWriteMonitorStream_.is_open())
	{
		regWriteMonitorStream_ << "Timestamp: " << std::dec << time(0) << 
			", \t ---------- Stopping..." << "\n";
		regWriteMonitorStream_.flush();
	}

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
	uint32_t address = __GET_ARG_IN__("address", uint32_t);
	uint32_t writeData = __GET_ARG_IN__("writeData", uint32_t);
	__FE_COUTV__((unsigned int)address);
	__FE_COUTV__((unsigned int)writeData);
	registerWrite(address, writeData);  
} //end WriteCFO()

//========================================================================
void CFOFrontEndInterface::ReadCFO(__ARGS__)
{	
	uint32_t address = __GET_ARG_IN__("address", uint32_t);
	__FE_COUTV__((unsigned int)address);
	uint32_t readData = registerRead(address);  
	
	char readDataStr[100];
	sprintf(readDataStr,"0x%X",readData);
	__SET_ARG_OUT__("readData",readDataStr);
} //end ReadCFO()

//========================================================================
void CFOFrontEndInterface::ResetRunplan(__ARGS__)
{	
	registerWrite(0x9100, 0x08000005); 
	registerWrite(0x9100, 0x00000005); 


	__FE_COUT__ << "Reset CFO Run Plan"  << __E__;
} //end ResetRunplan()

//========================================================================
void CFOFrontEndInterface::CompileRunplan(__ARGS__)
{	
	// to view output file with 8-byte rows
	// hexdump -e '"%08_ax " 1/8 "%016x "' -e '"\n"' srcs/mu2e_pcie_utils/cfoInterfaceLib/Commands.bin

	__FE_COUT__ << "Compile CFO Run Plan"  << __E__;

	CFOLib::CFO_Compiler compiler( 40000000 /* 40MHz FPGAClock for calculating delays */);

	std::ifstream inFile;
	std::ofstream outFile;

	std::string inFileName;// = __GET_ARG_IN__("Input Text Run Plan", std::string);
	std::string outFileName;// = __GET_ARG_IN__("Output Binary Run File", std::string);
	inFileName = 
		"/home/mu2ehwdev/ots/srcs/mu2e_pcie_utils/cfoInterfaceLib/Commands.txt";
	outFileName = 
		"/home/mu2ehwdev/ots/srcs/mu2e_pcie_utils/cfoInterfaceLib/Commands.bin";

	inFile.open(inFileName.c_str(), std::ios::in);
	if (!(inFile.is_open()))
	{
		__SS__ << ("Input File (" + inFileName + ") didn't open. Does it exist?") << __E__;
		__SS_THROW__;
	}

	outFile.open(outFileName.c_str(), std::ios::out | std::ios::binary);

	if (!(outFile.is_open()))
	{
		__SS__ << ("Output File (" + outFileName + ") didn't open. Does it exist?") << __E__;
		__SS_THROW__;
	}

	std::vector<std::string> lines;
	while (!inFile.eof())
	{
		std::string line;
		getline(inFile, line);
		lines.push_back(line);
	}
	inFile.close();

	std::deque<char> output = compiler.processFile(lines);
	for (auto c : output)
	{
		outFile << c;
	}
	outFile.close();
} //end CompileRunplan()


//========================================================================
void CFOFrontEndInterface::SetRunplan(__ARGS__)
{	
	//copying functionality of..
	//	cfoUtil write_program -p /home/kwar/cfo/RunplanFiveDTCs1.bin --cfo 0

	__FE_COUT__ << "Set CFO Run Plan"  << __E__;

	std::string setFileName;// = __GET_ARG_IN__("Binary Run File", std::string);
	setFileName =
		"/home/mu2ehwdev/ots/srcs/mu2e_pcie_utils/cfoInterfaceLib/Commands.bin";

	
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

} //end SetRunplan()

//========================================================================
void CFOFrontEndInterface::LaunchRunplan(__ARGS__)
{
	__FE_COUT__ << "Launch CFO Run Plan"  << __E__;

	registerWrite(0x914c, 0x0); 
	registerWrite(0x914c, 0x0000ffff); 

} //end LaunchRunplan()


DEFINE_OTS_INTERFACE(CFOFrontEndInterface)
