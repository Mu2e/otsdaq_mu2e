#include <iostream>
#include <set>
#include "otsdaq-mu2e/FEInterfaces/CAPTANSignalGenerator.h"
#include "otsdaq/Macros/CoutMacros.h"
#include "otsdaq/Macros/InterfacePluginMacros.h"
#include "otsdaq/MessageFacility/MessageFacility.h"

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "FE-CAPTANSignalGenerator"

//==============================================================================
CAPTANSignalGenerator::CAPTANSignalGenerator(
    const std::string&       interfaceUID,
    const ConfigurationTree& theXDAQContextConfigTree,
    const std::string&       interfaceConfigurationPath)
    : Socket(theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                 .getNode("HostIPAddress")
                 .getValue<std::string>(),
             theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                 .getNode("HostPort")
                 .getValue<unsigned int>())
    , FEVInterface(interfaceUID, theXDAQContextConfigTree, interfaceConfigurationPath)
    , OtsUDPHardware(theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                         .getNode("InterfaceIPAddress")
                         .getValue<std::string>(),
                     theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                         .getNode("InterfacePort")
                         .getValue<unsigned int>())
    , OtsUDPFirmwareDataGen(theXDAQContextConfigTree.getNode(interfaceConfigurationPath)
                                .getNode("FirmwareVersion")
                                .getValue<unsigned int>())
{
	// registration of FEMacro 'varTest2' generated, Oct-11-2018 02:28:57, by
	// 'admin' using MacroMaker.
	registerFEMacroFunction(
	    "varTest2",  // feMacroName
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &CAPTANSignalGenerator::varTest2),         // feMacroFunction
	    std::vector<std::string>{"myOtherArg"},        // namesOfInputArgs
	    std::vector<std::string>{"myArg", "outArg1"},  // namesOfOutputArgs
	    1);                                            // requiredUserPermissions

	// registration of FEMacro 'varTest' generated, Oct-11-2018 11:36:28, by
	// 'admin' using MacroMaker.
	registerFEMacroFunction(
	    "varTest",  // feMacroName
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &CAPTANSignalGenerator::varTest),          // feMacroFunction
	    std::vector<std::string>{"myOtherArg"},        // namesOfInputArgs
	    std::vector<std::string>{"myArg", "outArg1"},  // namesOfOutputArgs
	    1);                                            // requiredUserPermissions

	registerFEMacroFunction(
	    "Get Firmware Version",  // feMacroName
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &CAPTANSignalGenerator::getFirmwareVersion), // feMacroFunction
	    std::vector<std::string>{},        				 // namesOfInputArgs
	    std::vector<std::string>{},  					 // namesOfOutputArgs
	    1);                                              // requiredUserPermissions
	
	registerFEMacroFunction(
	    "Get Pulse Period",  // feMacroName
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &CAPTANSignalGenerator::getPulsePeriod), 	 // feMacroFunction
	    std::vector<std::string>{},        				 // namesOfInputArgs
	    std::vector<std::string>{},  					 // namesOfOutputArgs
	    1);                                              // requiredUserPermissions

	registerFEMacroFunction(
	    "Get Trigger Mode",  // feMacroName
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &CAPTANSignalGenerator::getTriggerMode), 	 // feMacroFunction
	    std::vector<std::string>{},        				 // namesOfInputArgs
	    std::vector<std::string>{},  					 // namesOfOutputArgs
	    1);                                              // requiredUserPermissions

	registerFEMacroFunction(
	    "Set Trigger Mode",  // feMacroName
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &CAPTANSignalGenerator::setTriggerMode), 	 // feMacroFunction
	    std::vector<std::string>{"Trigger Mode"},		 // namesOfInputArgs
	    std::vector<std::string>{},  					 // namesOfOutputArgs
	    1);                                              // requiredUserPermissions

	registerFEMacroFunction(
	    "Set Burst Mode",  // feMacroName
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &CAPTANSignalGenerator::setupBurstMode), 	 // feMacroFunction
	    std::vector<std::string>{"Burst Count"},		 // namesOfInputArgs
	    std::vector<std::string>{},  					 // namesOfOutputArgs
	    1);                                              // requiredUserPermissions	
	
	registerFEMacroFunction(
	    "Set Pulse Period",  // feMacroName
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &CAPTANSignalGenerator::setPulsePeriod), 	 // feMacroFunction
	    std::vector<std::string>{
			"Trigger Period (s, ms, us, ns, and clocks allowed) [clocks := 10ns]"
			},		 									 // namesOfInputArgs
	    std::vector<std::string>{},  					 // namesOfOutputArgs
	    1);                                              // requiredUserPermissions	

	universalAddressSize_ = 8;
	universalDataSize_    = 8;
}

//==============================================================================
CAPTANSignalGenerator::~CAPTANSignalGenerator(void) {}

//==============================================================================
void CAPTANSignalGenerator::configure(void)
{
	//	unsigned int i = VStateMachine::getIterationIndex();
	//	unsigned int j = VStateMachine::getSubIterationIndex();
	//	if(i == 0 && j < 5)
	//		VStateMachine::indicateSubIterationWork();
	//	else if(i < 10)
	//		VStateMachine::indicateIterationWork();
	//
	//
	//	__FE_COUTV__(VStateMachine::getSubIterationIndex());
	//	__FE_COUTV__(VStateMachine::getSubIterationWork());
	//	__FE_COUTV__(VStateMachine::getIterationIndex());
	//	__FE_COUTV__(VStateMachine::getIterationWork());
	//
	//
	//	return;

	__FE_COUT__ << "configure" << __E__;
	__FE_COUT__ << "Clearing receive socket buffer: " << OtsUDPHardware::clearReadSocket()
	            << " packets cleared." << __E__;

	std::string sendBuffer;
	std::string recvBuffer;
	uint64_t    readQuadWord;

	__FE_COUT__
	    << "Configuration Path Table: "
	    << theXDAQContextConfigTree_.getNode(theConfigurationPath_).getTableName() << "-v"
	    << theXDAQContextConfigTree_.getNode(theConfigurationPath_).getTableVersion()
	    << __E__;

	__FE_COUT__ << "Interface name: "
	            << theXDAQContextConfigTree_.getNode(theConfigurationPath_) << __E__;

	__FE_COUT__ << "Configured Firmware Version: "
	            << theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	                   .getNode("FirmwareVersion")
	                   .getValue<unsigned int>()
	            << __E__;

	__FE_COUT__ << "Setting Destination IP: "
	            << theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	                   .getNode("StreamToIPAddress")
	                   .getValue<std::string>()
	            << __E__;
	__FE_COUT__ << "And Destination Port: "
	            << theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	                   .getNode("StreamToPort")
	                   .getValue<unsigned int>()
	            << __E__;

	OtsUDPFirmwareCore::setDataDestination(
	    sendBuffer,
	    theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	        .getNode("StreamToIPAddress")
	        .getValue<std::string>(),
	    theXDAQContextConfigTree_.getNode(theConfigurationPath_)
	        .getNode("StreamToPort")
	        .getValue<uint64_t>());
	OtsUDPHardware::write(sendBuffer);

	__FE_COUT__ << "Reading back burst dest MAC/IP/Port: " << __E__;

	OtsUDPFirmwareCore::readDataDestinationMAC(sendBuffer);
	OtsUDPHardware::read(sendBuffer, readQuadWord);

	OtsUDPFirmwareCore::readDataDestinationIP(sendBuffer);
	OtsUDPHardware::read(sendBuffer, readQuadWord);

	OtsUDPFirmwareCore::readDataDestinationPort(sendBuffer);
	OtsUDPHardware::read(sendBuffer, readQuadWord);

	OtsUDPFirmwareCore::readControlDestinationPort(sendBuffer);
	OtsUDPHardware::read(sendBuffer, readQuadWord);

	// Run Configure Sequence Commands
	FEVInterface::runSequenceOfCommands("LinkToConfigureSequence");

	__FE_COUT__ << "Done with ots Template configuring." << __E__;
}

//==============================================================================
// void CAPTANSignalGenerator::configureDetector(const DACStream& theDACStream)
//{
//	__FE_COUT__ << "\tconfigureDetector" << __E__;
//}

//==============================================================================
void CAPTANSignalGenerator::halt(void)
{
	__FE_COUT__ << "\tHalt" << __E__;
	stop();
}

//==============================================================================
void CAPTANSignalGenerator::pause(void)
{
	__FE_COUT__ << "\tPause" << __E__;
	stop();
}

//==============================================================================
void CAPTANSignalGenerator::resume(void)
{
	__FE_COUT__ << "\tResume" << __E__;
	start("");
}

//==============================================================================
void CAPTANSignalGenerator::start(std::string)  // runNumber)
{
	__FE_COUT__ << "\tStart" << __E__;

	//		unsigned int i = VStateMachine::getIterationIndex();
	//		unsigned int j = VStateMachine::getSubIterationIndex();
	//		if(i == 0 && j < 5)
	//			VStateMachine::indicateSubIterationWork();
	//		else if(i < 10)
	//			VStateMachine::indicateIterationWork();
	//
	//
	//		__FE_COUTV__(VStateMachine::getSubIterationIndex());
	//		__FE_COUTV__(VStateMachine::getSubIterationWork());
	//		__FE_COUTV__(VStateMachine::getIterationIndex());
	//		__FE_COUTV__(VStateMachine::getIterationWork());
	//
	//
	//		return;

	// Run Start Sequence Commands
	FEVInterface::runSequenceOfCommands("LinkToStartSequence");

	std::string sendBuffer;
	OtsUDPFirmwareCore::startBurst(sendBuffer);
	OtsUDPHardware::write(sendBuffer);
}

//==============================================================================
void CAPTANSignalGenerator::stop(void)
{
	__FE_COUT__ << "\tStop" << __E__;

	int numberOfCAPTANPulses =
	    getConfigurationManager()
	        ->getNode("/Mu2eGlobalsTable/SyncDemoConfig/NumberOfCAPTANPulses")
	        .getValue<unsigned int>();
	__FE_COUTV__(numberOfCAPTANPulses);

	if(numberOfCAPTANPulses == 0)
	{
		return;
	}

	int stopIndex = getIterationIndex();

	if(stopIndex > numberOfCAPTANPulses)
	{
		__FE_COUT_INFO__ << " loopback DONE" << __E__;

		// Run Stop Sequence Commands

		FEVInterface::runSequenceOfCommands("LinkToStopSequence");

		std::string sendBuffer;
		OtsUDPFirmwareCore::stopBurst(sendBuffer);
		OtsUDPHardware::write(sendBuffer);

		return;
	}

	// Generated Macro Name:	CAPTAN_OnePulse
	// Macro Notes:
	// Generated Time: 		Dec-10-2018 03:44:58
	// Paste this whole file into an interface to transfer Macro functionality.
	{
		char* address = new char[universalAddressSize_]{
		    0};  //create address buffer of interface size and init to all 0
		char* data = new char[universalDataSize_]{
		    0};                 //create data buffer of interface size and init to all 0
		uint64_t macroAddress;  // create macro address buffer (size 8 bytes)
		uint64_t macroData;     // create macro address buffer (size 8 bytes)
		std::map<std::string /*arg name*/, uint64_t /*arg val*/>
		    macroArgs;  // create map from arg name to 64-bit number

		// command-#0: Write(0x1001 /*address*/,0x1 /*data*/);
		macroAddress = 0x1001;
		memcpy(address, &macroAddress, 8);  // copy macro address to buffer
		macroData = 0x1;
		memcpy(data, &macroData, 8);  // copy macro data to buffer
		universalWrite(address, data);

		delete[] address;  // free the memory
		delete[] data;     // free the memory
	}

	indicateIterationWork();
	return;
}

//==============================================================================
bool CAPTANSignalGenerator::running(void)
{
	__FE_COUT__ << "\tRunning" << __E__;

	//__SS__ << "?" << __E__; //test exceptions during running
	//__SS_THROW__;

	int state = -1;
	while(WorkLoop::continueWorkLoop_)
	{
		// while running
		// play with the LEDs at address 0x1003

		++state;
		if(state < 8)
			sleep(1);
		else
			break;
	}

	//		//example!
	//		//play with array of 8 LEDs at address 0x1003

	//
	//	bool flashLEDsWhileRunning = false;
	//	if(flashLEDsWhileRunning)
	//	{
	//		std::string writeBuffer;
	//		int state = -1;
	//		while(WorkLoop::continueWorkLoop_)
	//		{
	//			//while running
	//			//play with the LEDs at address 0x1003
	//
	//			++state;
	//			if(state < 8)
	//			{
	//				writeBuffer.resize(0);
	//				OtsUDPFirmwareCore::write(writeBuffer,
	// 0x1003,1<<state);
	//				OtsUDPHardware::write(writeBuffer);
	//			}
	//			else if(state%2 == 1 && state < 11)
	//			{
	//				writeBuffer.resize(0);
	//				OtsUDPFirmwareCore::write(writeBuffer, 0x1003,
	// 0xFF); 				OtsUDPHardware::write(writeBuffer);
	//			}
	//			else if(state%2 == 0 && state < 11)
	//			{
	//				writeBuffer.resize(0);
	//				OtsUDPFirmwareCore::write(writeBuffer,
	// 0x1003,0); 				OtsUDPHardware::write(writeBuffer);
	//			}
	//			else
	//				state = -1;
	//
	//			sleep(1);
	//		}
	//	}

	return false;
}

//==============================================================================
// NOTE: buffer for address must be at least size universalAddressSize_
// NOTE: buffer for returnValue must be max UDP size to handle return
// possibility
void ots::CAPTANSignalGenerator::universalRead(char* address, char* returnValue)
{
	__FE_COUT__ << "address size " << universalAddressSize_ << __E__;

	__FE_COUT__ << "Request: ";
	for(unsigned int i = 0; i < universalAddressSize_; ++i)
		printf("%2.2X", (unsigned char)address[i]);
	std::cout << __E__;

	std::string readBuffer, sendBuffer;
	OtsUDPFirmwareCore::readAdvanced(sendBuffer, address, 1 /*size*/);

	OtsUDPHardware::read(sendBuffer, readBuffer);  // data reply

	__FE_COUT__ << "Result SIZE: " << readBuffer.size() << __E__;
	std::memcpy(returnValue, readBuffer.substr(2).c_str(), universalDataSize_);

}  // end universalRead()

//==============================================================================
// NOTE: buffer for address must be at least size universalAddressSize_
// NOTE: buffer for writeValue must be at least size universalDataSize_
void ots::CAPTANSignalGenerator::universalWrite(char* address, char* writeValue)
{
	__FE_COUT__ << "address size " << universalAddressSize_ << __E__;
	__FE_COUT__ << "data size " << universalDataSize_ << __E__;
	__FE_COUT__ << "Sending: ";
	for(unsigned int i = 0; i < universalAddressSize_; ++i)
		printf("%2.2X", (unsigned char)address[i]);
	std::cout << __E__;

	std::string sendBuffer;
	OtsUDPFirmwareCore::writeAdvanced(sendBuffer, address, writeValue, 1 /*size*/);
	OtsUDPHardware::write(sendBuffer);  // data request
}  // end universalWrite()

//==============================================================================
void ots::CAPTANSignalGenerator::getFirmwareVersion(__ARGS__)
{
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	std::map<std::string, uint64_t> macroArgs;
	char* address = new char[universalAddressSize_]{0};
	char* data = new char[universalDataSize_]{0};
	
	uint64_t macroAddress = 0x0;
	memcpy(address, &macroAddress, 8);
	universalRead(address, data);

	memcpy(&macroArgs["pulsePeriod"], data, 8);
	__SET_ARG_OUT__("Pulse Period (us)", macroArgs["pulsePeriod"]);

	delete[] address;  // free the memory
	delete[] data;     // free the memory
} // end getFirmwareVersion()

//==============================================================================
void ots::CAPTANSignalGenerator::getPulsePeriod(__ARGS__)
{
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	std::map<std::string, uint64_t> macroArgs;
	char* address = new char[universalAddressSize_]{0};
	char* data = new char[universalDataSize_]{0};
	
	uint64_t macroAddress = 0x2;
	memcpy(address, &macroAddress, 8);
	universalRead(address, data);

	uint64_t scaleMicrosecond = 100;
	uint64_t highPulseWidth = 50;
	uint64_t lowPulseWidth = std::stoi(std::string(data));
	uint64_t pulsePeriod = (lowPulseWidth + highPulseWidth) / scaleMicrosecond;

	char* pulsePeriod_c = new char[universalDataSize_]{0};
	std::string pulsePeriod_s = std::to_string(pulsePeriod);
	std::strcpy(pulsePeriod_c, pulsePeriod_s.c_str());

	memcpy(&macroArgs["pulsePeriod"], pulsePeriod_c, 8);
	__SET_ARG_OUT__("Pulse Period (us)", macroArgs["pulsePeriod"]);

	delete[] address;  		// free the memory
	delete[] data;     		// free the memory
	delete[] pulsePeriod_c; // free the memory
} // end getPulsePeriod()

//==============================================================================
void ots::CAPTANSignalGenerator::setPulsePeriod(__ARGS__)
{
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	std::string pulsePeriod = __GET_ARG_IN__("Trigger Period (s, ms, us, ns, and clocks allowed) [clocks := 10ns]", std::string);

	__FE_COUTV__(pulsePeriod);
	bool   foundUnits = false;
	size_t i;
	for(i = 0; i < pulsePeriod.size(); ++i)
		if(pulsePeriod[i] == 's' || pulsePeriod[i] == 'm' ||
		   pulsePeriod[i] == 'u' || pulsePeriod[i] == 'n' || pulsePeriod[i] == 'c')
		{
			foundUnits = true;
			break;
		}
	
	if(!foundUnits)
	{
		__FE_SS__
		    << "No units were found in the input parameters 'Fixed-width Event Window "
		       "Duration' value: "
		    << pulsePeriod
		    << ". Please use units when specifying event window duration (s, ms, us, ns, "
		       "and clocks are allowed). For example '1.7us' or '1675ns' would be valid."
		    << __E__;
		__FE_SS_THROW__;
	}

	std::string pulsePeriodSplitNumber = pulsePeriod.substr(0, i);
	std::string pulsePeriodSplitUnits  = pulsePeriod.substr(i);
	__FE_COUTV__(pulsePeriodSplitNumber);
	__FE_COUTV__(pulsePeriodSplitUnits);

	//copied from CFO_Compiler.cpp::transcribeInstructions() [L494]
	uint64_t value;
	if(!StringMacros::getNumber(pulsePeriodSplitNumber, value))
	{
		__FE_SS__ << "The duration parameter value '" << pulsePeriodSplitNumber << " "
		          << pulsePeriodSplitUnits << "' is not a valid number. "
		          << "Use 0x### to indicate hex and b### to indicate binary; otherwise, "
		             "decimal is inferred."
		          << __E__;
		__FE_SS_THROW__;
	}

	//test floating point in case integer conversion dropped something
	double timeValue = strtod(pulsePeriodSplitNumber.c_str(), 0);
	__FE_COUTV__(timeValue);
	if(timeValue < value)
		timeValue = value;
	
	const uint64_t FPGAClock_ = 10;  //period of FPGA clock in ns
	__FE_COUTV__(FPGAClock_);
	__FE_COUTV__(value);
	__FE_COUTV__(timeValue);

	uint64_t pulsePeriodInClocks;

	if(pulsePeriodSplitUnits == "s")  // Wait wanted in seconds
		pulsePeriodInClocks = timeValue * 1e9 / FPGAClock_;
	else if(pulsePeriodSplitUnits == "ms")  // Wait wanted in milliseconds
		pulsePeriodInClocks = timeValue * 1e6 / FPGAClock_;
	else if(pulsePeriodSplitUnits == "us")  // Wait wanted in microseconds
		pulsePeriodInClocks = timeValue * 1e3 / FPGAClock_;
	else if(pulsePeriodSplitUnits == "ns")  // Wait wanted in nanoseconds
	{
		if((value % FPGAClock_) != 0)
		{
			__FE_SS__ << "FPGA can only wait in multiples of " << FPGAClock_
			          << " ns: the input event duration value '" << value
			          << "' yields a remainder of " << (value % FPGAClock_) << __E__;
			__FE_SS_THROW__;
		}
		pulsePeriodInClocks = value / FPGAClock_;
	}
	else if(pulsePeriodSplitUnits == "clocks")  // Wait wanted in FPGA clocks
		pulsePeriodInClocks = value;
	else  //impossible
	{
		__FE_SS__ << "The event duration input parameter is missing a valid unit type "
		             "after parameter: "
		          << pulsePeriodSplitUnits
		          << ". Accepted unit types are clocks, ns, us, ms, and s." << __E__;
		__FE_SS_THROW__;
	}

	std::map<std::string, uint64_t> macroArgs;
	char* address = new char[universalAddressSize_]{0};
	char* data = new char[universalDataSize_]{0};

	uint64_t macroAddress = 0xB;
	memcpy(address, &macroAddress, 8);

	// if(!StringMacros::getNumber(pulsePeriodInClocks, data))
	// {
	// 	__FE_SS_THROW__;
	// }

	std::string pulsePeriodInClocks_s = std::to_string(pulsePeriodInClocks);
	std::strcpy(data, pulsePeriodInClocks_s.c_str());

	memcpy(data, &macroArgs["totalPulses"], 8);
	universalWrite(address, data);

	delete[] address;  // free the memory
	delete[] data;     // free the memory
} // end setPulsePeriod()

//==============================================================================
void ots::CAPTANSignalGenerator::setupBurstMode(__ARGS__)
{
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	std::map<std::string, uint64_t> macroArgs;
	char* address = new char[universalAddressSize_]{0};
	char* data = new char[universalDataSize_]{0};

	uint64_t mode = 1;
	setTriggerMode(mode);

	uint64_t macroAddress = 0xA;
	memcpy(address, &macroAddress, 8);
	macroArgs["totalPulses"] = __GET_ARG_IN__("Burst Count", uint64_t);

	memcpy(data, &macroArgs["totalPulses"], 8);
	universalWrite(address, data);

	delete[] address;  // free the memory
	delete[] data;     // free the memory
} // end setupBurstMode()

//==============================================================================
void ots::CAPTANSignalGenerator::getTriggerMode(__ARGS__)
{
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	std::map<std::string, uint64_t> macroArgs;
	char* address = new char[universalAddressSize_]{0};
	char* data = new char[universalDataSize_]{0};

	uint64_t macroAddress = 0x0;
	memcpy(address, &macroAddress, 8);
	universalRead(address, data);

	memcpy(&macroArgs["TriggerMode"], data, 8);
	__SET_ARG_OUT__("Trigger Mode", macroArgs["TriggerMode"]);

	delete[] address;  // free the memory
	delete[] data;     // free the memory
} // end getTriggerMode()

//==============================================================================
void ots::CAPTANSignalGenerator::setTriggerMode(__ARGS__)
{
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	std::map<std::string, uint64_t> macroArgs;
	char* address = new char[universalAddressSize_]{0};
	char* data = new char[universalDataSize_]{0};
	
	uint64_t macroAddress = 0x9;
	memcpy(address, &macroAddress, 8);
	macroArgs["TriggerMode"] = __GET_ARG_IN__("Trigger Mode", uint64_t);

	memcpy(data, &macroArgs["TriggerMode"], 8);
	universalWrite(address, data);

	delete[] address;  // free the memory
	delete[] data;     // free the memory
} // end setTriggerMode()

//==============================================================================
void ots::CAPTANSignalGenerator::setTriggerMode(uint64_t triggerMode)
{
	char* address = new char[universalAddressSize_]{0};
	char* data = new char[universalDataSize_]{0};
	uint64_t macroAddress = 0x9;

	memcpy(address, &macroAddress, 8);
	memcpy(data, &triggerMode, 8);
	universalWrite(address, data);
} // end setTriggerMode

//==============================================================================
// varTest
//	FEMacro 'varTest' generated, Oct-11-2018 11:36:28, by 'admin' using
// MacroMaker. 	Macro Notes: This is a great test!
void CAPTANSignalGenerator::varTest(__ARGS__)
{
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	// macro commands section
	{
		char* address = new char[universalAddressSize_]{
		    0};  //create address buffer of interface size and init to all 0
		char* data = new char[universalDataSize_]{
		    0};                 //create data buffer of interface size and init to all 0
		uint64_t macroAddress;  // create macro address buffer (size 8 bytes)
		//uint64_t macroData;     // create macro address buffer (size 8 bytes)
		std::map<std::string /*arg name*/, uint64_t /*arg val*/>
		    macroArgs;  // create map from arg name to 64-bit number

		// command-#0: Read(0x1002 /*address*/,myArg /*data*/);
		macroAddress = 0x1002;
		memcpy(address, &macroAddress, 8);  // copy macro address to buffer
		universalRead(address, data);
		memcpy(&macroArgs["myArg"], data, 8);  // copy buffer to argument map
		__SET_ARG_OUT__("myArg",
		                macroArgs["myArg"]);  // update output argument result

		// command-#1: Read(0x1001 /*address*/,data);
		macroAddress = 0x1001;
		memcpy(address, &macroAddress, 8);  // copy macro address to buffer
		universalRead(address, data);
		memcpy(&macroArgs["outArg1"], data, 8);  // copy buffer to argument map
		__SET_ARG_OUT__("outArg1",
		                macroArgs["outArg1"]);  // update output argument result

		// command-#2: Write(0x1002 /*address*/,myOtherArg /*data*/);
		macroAddress = 0x1002;
		memcpy(address, &macroAddress, 8);  // copy macro address to buffer
		macroArgs["myOtherArg"] =
		    __GET_ARG_IN__("myOtherArg", uint64_t);  // initialize from input
		                                             // arguments	//get macro
		                                             // data argument
		memcpy(data, &macroArgs["myOtherArg"],
		       8);  // copy macro data argument to buffer
		universalWrite(address, data);

		// command-#3: Write(0x1001 /*address*/,myArg /*data*/);
		macroAddress = 0x1001;
		memcpy(address,
		       &macroAddress,
		       8);  // copy macro address to buffer	//get macro data argument
		memcpy(data, &macroArgs["myArg"], 8);  // copy macro data argument to buffer
		universalWrite(address, data);

		// command-#4: delay(4000);
		__FE_COUT__ << "Sleeping for... " << 4000 << " milliseconds " << __E__;
		usleep(4000 * 1000 /* microseconds */);

		delete[] address;  // free the memory
		delete[] data;     // free the memory
	}

	for(auto& argOut : argsOut)
		__FE_COUT__ << argOut.first << ": " << argOut.second << __E__;

}  // end varTest()

//==============================================================================
// varTest2
//	FEMacro 'varTest2' generated, Oct-11-2018 02:28:57, by 'admin' using
// MacroMaker. 	Macro Notes: [Modified 14:28 10/11/2018] This is a great test!
void CAPTANSignalGenerator::varTest2(__ARGS__)
{
	__FE_COUT__ << "# of input args = " << argsIn.size() << __E__;
	__FE_COUT__ << "# of output args = " << argsOut.size() << __E__;
	for(auto& argIn : argsIn)
		__FE_COUT__ << argIn.first << ": " << argIn.second << __E__;

	// macro commands section
	{
		char* address = new char[universalAddressSize_]{
		    0};  //create address buffer of interface size and init to all 0
		char* data = new char[universalDataSize_]{
		    0};                 //create data buffer of interface size and init to all 0
		uint64_t macroAddress;  // create macro address buffer (size 8 bytes)
		//uint64_t macroData;     // create macro address buffer (size 8 bytes)
		std::map<std::string /*arg name*/, uint64_t /*arg val*/>
		    macroArgs;  // create map from arg name to 64-bit number

		// command-#0: Read(0x1002 /*address*/,myArg /*data*/);
		macroAddress = 0x1002;
		memcpy(address, &macroAddress, 8);  // copy macro address to buffer
		universalRead(address, data);
		memcpy(&macroArgs["myArg"], data, 8);  // copy buffer to argument map
		__SET_ARG_OUT__("myArg",
		                macroArgs["myArg"]);  // update output argument result

		// command-#1: Read(0x1001 /*address*/,data);
		macroAddress = 0x1001;
		memcpy(address, &macroAddress, 8);  // copy macro address to buffer
		universalRead(address, data);
		memcpy(&macroArgs["outArg1"], data, 8);  // copy buffer to argument map
		__SET_ARG_OUT__("outArg1",
		                macroArgs["outArg1"]);  // update output argument result

		// command-#2: Write(0x1002 /*address*/,myOtherArg /*data*/);
		macroAddress = 0x1002;
		memcpy(address, &macroAddress, 8);  // copy macro address to buffer
		macroArgs["myOtherArg"] =
		    __GET_ARG_IN__("myOtherArg", uint64_t);  // initialize from input
		                                             // arguments	//get macro
		                                             // data argument
		memcpy(data, &macroArgs["myOtherArg"],
		       8);  // copy macro data argument to buffer
		universalWrite(address, data);

		// command-#3: Write(0x1001 /*address*/,myArg /*data*/);
		macroAddress = 0x1001;
		memcpy(address,
		       &macroAddress,
		       8);  // copy macro address to buffer	//get macro data argument
		memcpy(data, &macroArgs["myArg"], 8);  // copy macro data argument to buffer
		universalWrite(address, data);

		// command-#4: delay(4000);
		__FE_COUT__ << "Sleeping for... " << 4000 << " milliseconds " << __E__;
		usleep(4000 * 1000 /* microseconds */);

		delete[] address;  // free the memory
		delete[] data;     // free the memory
	}

	for(auto& argOut : argsOut)
		__FE_COUT__ << argOut.first << ": " << argOut.second << __E__;

}  // end varTest2()

DEFINE_OTS_INTERFACE(CAPTANSignalGenerator)
