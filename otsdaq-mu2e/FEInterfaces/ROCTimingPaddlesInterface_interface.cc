#include "otsdaq-mu2e/FEInterfaces/ROCTimingPaddlesInterface_interface.h"
#include "otsdaq/Macros/InterfacePluginMacros.h"

#include "cetlib/filepath_maker.h"

#include <array>
#include <cstring>
#include <fstream>
#include <regex>
#include <utility>

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "FE-ROCTimingPaddlesInterface"

// extern "C" {
//     #include "MU2E-API/API_I2C.h"
//     #include "MU2E-API/SBL_utils.h"
// }

// 259 (and others) ==> the number of words in block read is written first as a block
// write

//=========================================================================================
ROCTimingPaddlesInterface::ROCTimingPaddlesInterface(
    const std::string&       rocUID,
    const ConfigurationTree& theXDAQContextConfigTree,
    const std::string&       theConfigurationPath)
    : ROCPolarFireCoreInterface(rocUID, theXDAQContextConfigTree, theConfigurationPath)
{
	INIT_MF("." /*directory used is USER_DATA/LOG/.*/);

	__CFG_COUT__ << "Constructor..." << __E__;

	__CFG_COUT_INFO__ << "ROCPolarFireCoreInterface instantiated with link: " << linkID_
	                  << " and EventWindowDelayOffset = " << delay_ << __E__;

	registerFEMacroFunction("Configure State Machine",
	                        static_cast<FEVInterface::frontEndMacroFunction_t>(
	                            &ROCTimingPaddlesInterface::Configure),
	                        std::vector<std::string>{},  // inputs parameters
	                        std::vector<std::string>{},  // output parameters
	                        1                            // requiredUserPermissions
	);

	registerFEMacroFunction("Read Histograms",
	                        static_cast<FEVInterface::frontEndMacroFunction_t>(
	                            &ROCTimingPaddlesInterface::ReadMarkerHistograms),
	                        std::vector<std::string>{},  // inputs parameters
	                        std::vector<std::string>{"Plotly_Plot0",
	                                                 "Plotly_Plot1",
	                                                 "Plotly_Plot2",
	                                                 "histograms"},  // output parameters
	                        1  // requiredUserPermissions
	);

	registerFEMacroFunction("Reset",
	                        static_cast<FEVInterface::frontEndMacroFunction_t>(
	                            &ROCTimingPaddlesInterface::Reset),
	                        std::vector<std::string>{},  // inputs parameters
	                        std::vector<std::string>{},  // output parameters
	                        1                            // requiredUserPermissions
	);

	registerFEMacroFunction(
	    "Setup For Run",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &ROCTimingPaddlesInterface::PrepareDataRuns),
	    std::vector<std::string>{"Enable Delay (Default := true)",
	                             "Delay amount (Default := 0)"},  // inputs parameters
	    std::vector<std::string>{},                               // output parameters
	    1  // requiredUserPermissions
	);

	registerFEMacroFunction(
	    "Selective Reset",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &ROCTimingPaddlesInterface::SelectiveReset),
	    std::vector<std::string>{"40MHz Clock Generator (Default := false)",
	                             "Forward Detector (Default := false)",
	                             "Command Handler (Default := false)",
	                             "Timestamp Manager (Default := false)",
	                             "DCS Receive FIFO (Default := false)",
	                             "ROC Monitor (Default := false)",
	                             "DCS Response FIFO (Default := false)",
	                             "Packet Sender (Default := false)",
	                             "Receive FIFO (Default := false)",
	                             "Response FIFO (Default := false)",
	                             "BERT (Default := false)"},  // inputs parameters
	    std::vector<std::string>{},                           // output parameters
	    1                                                     // requiredUserPermissions
	);

	registerFEMacroFunction(
	    "Reset Histograms",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &ROCTimingPaddlesInterface::ResetHistograms),
	    std::vector<std::string>{
	        "Forward Detector Loopback Markers (Default := false)",
	        "Forward Detector Clock Markers (Default := false)",
	        "Forward Detector Event Markers (Default := false)",
	        "Timestamp Manager Event Markers (Default := false)",
	        "Packet Sender Loopback Markers(Default := false)"},  // inputs parameters
	    std::vector<std::string>{},                               // output parameters
	    1  // requiredUserPermissions
	);

	registerFEMacroFunction(
	    "varTest",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &ROCTimingPaddlesInterface::varTest),
	    std::vector<std::string>{},                                // inputs parameters
	    std::vector<std::string>{"Plotly_Plot1", "Plotly_Plot2"},  // output parameters
	    1  // requiredUserPermissions
	);

	registerFEMacroFunction("Read Rx FIFO",
	                        static_cast<FEVInterface::frontEndMacroFunction_t>(
	                            &ROCTimingPaddlesInterface::ReadRxFIFO),
	                        std::vector<std::string>{},          // inputs parameters
	                        std::vector<std::string>{"Result"},  // output parameters
	                        1  // requiredUserPermissions
	);

	registerFEMacroFunction("Read Tx FIFO",
	                        static_cast<FEVInterface::frontEndMacroFunction_t>(
	                            &ROCTimingPaddlesInterface::ReadTxFIFO),
	                        std::vector<std::string>{},          // inputs parameters
	                        std::vector<std::string>{"Result"},  // output parameters
	                        1  // requiredUserPermissions
	);

	registerFEMacroFunction(
	    "BERT Setup",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &ROCTimingPaddlesInterface::BERT),
	    std::vector<std::string>{
	        "Transmit PRBS7 (Default := false)",
	        "BER Received data (Default := false)",
	        "RX fixed pattern (Default := true)",
	        "External loopback mode (Default := false)"},  // inputs parameters
	    std::vector<std::string>{},                        // output parameters
	    1                                                  // requiredUserPermissions
	);
}  // end constructor()

//==========================================================================================
ROCTimingPaddlesInterface::~ROCTimingPaddlesInterface(void)
{
	// NOTE:: be careful not to call __FE_COUT__ decoration because it uses the
	// tree and it may already be destructed partially
	__COUT__ << FEVInterface::interfaceUID_ << "Destructed." << __E__;
}  // end destructor()

//======================================================================================================

void ROCTimingPaddlesInterface::configure(void)
try
{
	// ROCPolarFireCoreInterface::configure();

	// consider that we know all the init files
	// all the init information are stored in the configuration tree    //
	// set parameter
	// int linkID = getSelfNode().getNode("linkID").getValue<int>();
	//	__COUTV__(linkID);

	// runSequenceOfCommands("ROCTypeLinkTable/LinkToConfigureSequence"); /*Run Configure Sequence Commands*/

	// reset all blocks using reset controller (including 40mhz clock)
	writeRegister(0, 0);
	usleep(500000);

	// disable PRBS7 generator & error checker
	writeRegister(49, 0);
	usleep(100000);

	// reset ROC Mon blocks (enables packet sender)
	readRegister(43);

	// check reset is done, register 0x0 is debug register
	readRegister(0);

	// event start and loopback delay
	writeRegister(21, 0);
	usleep(100000);

	// Event start and loopback delay enable
	writeRegister(45, 1);
	usleep(100000);
	// check reset is done, register 0x0 is debug register
	readRegister(1);

	// writeRegister(22, 1);  // DCS alignment request  (reset histograms in FD)
	readRegister(44);  // reset loopback counters and histograms
	usleep(100000);
	// check reset is done, register 0x0 is debug register
	readRegister(2);
	usleep(100000);

	// reset FSM and FIFOs for timestamping
	readRegister(36);

	//    int myTemp = GetTemperature(1);
	// 	__COUTV__(myTemp);
	// 	__COUT__<<"my temp"<<myTemp<<__E__;
	//     __COUTV__(myTemp);
	//     __MCOUTV__(myTemp);
}
catch(const std::runtime_error& e)
{
	__FE_COUT__ << "Error caught: " << e.what() << __E__;
	throw;
}
catch(...)
{
	__FE_SS__ << "Unknown error caught. Check printouts!" << __E__;
	try
	{
		throw;
	}  // one more try to printout extra info
	catch(const std::exception& e)
	{
		ss << "Exception message: " << e.what();
	}
	catch(...)
	{
	}
	__FE_SS_THROW__;
}

//==================================================================================================
bool ROCTimingPaddlesInterface::running(void) { return false; }

//==================================================================================================
void ROCTimingPaddlesInterface::varTest(__ARGS__)
{
	std::vector<uint16_t> eventHistAddrsFD   = {25, 26, 27, 28, 29};
	std::vector<uint16_t> eventHistAddrsTest = {20, 15, 14, 13, 9};
	std::string           data = StringMacros::vectorToString(eventHistAddrsFD, ",");
	std::string dataTest       = StringMacros::vectorToString(eventHistAddrsTest, ",");
	data                       = "[" + data + "]";
	dataTest                   = "[" + dataTest + "]";

	std::string histogramTemplate = R"({
        "data" : [{
                "x": [-2, -1, 0, 1, 2],
				"y": <LOOPBACK>,
                "type": "bar",
                "name": "Loopback Markers",
                "opacity": 0.75
            },
            {
				"x": [-2, -1, 0, 1, 2],
                "y": <CLOCK>,
                "type": "bar",
                "name": "Clock Markers",
                "opacity": 0.75
            },
			{
				"x": [-2, -1, 0, 1, 2],
                "y": <EVENT>,
                "type": "bar",
                "name": "Event Markers",
                "opacity": 0.75
            }],
        "layout" : {
                "title" : { "text" : "Awesome Histogram"},
                "xaxis" : { "title" : {"text" : "Bin"}, "titlefont": { "size" : 10 }, "showticklabels" : true },
                "yaxis" : { "title" : {"text" : "Count"}, "titlefont": { "size" : 10 }, "zeroline" : true }
            }
    })";

	std::string histogramTemplateTest = R"({
        "data" : [{
                "x": [-2, -1, 0, 1, 2],
				"y": <LOOPBACK>,
                "type": "bar",
                "name": "Loopback Markers",
                "opacity": 0.75
            },
            {
				"x": [-2, -1, 0, 1, 2],
                "y": <CLOCK>,
                "type": "bar",
                "name": "Clock Markers",
                "opacity": 0.75
            },
			{
				"x": [-2, -1, 0, 1, 2],
                "y": [1,3,4,6,9],
				"mode": "lines+markers",
				"type": "scatter",
                "name": "Event Markers",
                "opacity": 0.75
            }],
        "layout" : {
                "title" : { "text" : "Dummy Data Test #2"},
                "xaxis" : { "title" : {"text" : "Bin"}, "titlefont": { "size" : 10 }, "showticklabels" : true },
                "yaxis" : { "title" : {"text" : "Count"}, "titlefont": { "size" : 10 }, "zeroline" : true }
            }
    })";

	histogramTemplate =
	    std::regex_replace(histogramTemplate, std::regex("<LOOPBACK>"), data);
	histogramTemplate =
	    std::regex_replace(histogramTemplate, std::regex("<CLOCK>"), data);
	histogramTemplate =
	    std::regex_replace(histogramTemplate, std::regex("<EVENT>"), data);

	histogramTemplateTest =
	    std::regex_replace(histogramTemplateTest, std::regex("<LOOPBACK>"), data);
	histogramTemplateTest =
	    std::regex_replace(histogramTemplateTest, std::regex("<CLOCK>"), dataTest);
	// histogramTemplateTest = std::regex_replace(histogramTemplateTest, std::regex("<EVENT>"), dataTest);

	__COUT_INFO__ << histogramTemplate << __E__;
	__SET_ARG_OUT__("Plotly_Plot1", histogramTemplate);
	__SET_ARG_OUT__("Plotly_Plot2", histogramTemplateTest);
}

//==================================================================================================
void ROCTimingPaddlesInterface::Configure(__ARGS__)
{
	__COUT_INFO__ << "Configure called" << __E__;
	configure();
}

//==================================================================================================
std::string ROCTimingPaddlesInterface::readBuffer(uint32_t loc_addr)
{
	std::stringstream outss;
	for(u_int16_t i = 5; i < 20; i++)
	{
		outss << "0x" << std::hex << getDTC()->ReadExtROCRegister(linkID_, loc_addr, i)
		      << __E__;
	}
	return outss.str();
}

//==================================================================================================
void ROCTimingPaddlesInterface::ReadTxFIFO(__ARGS__)
{
	__COUT_INFO__ << "ReadTxFIFO called" << __E__;
	__SET_ARG_OUT__("Result", ROCTimingPaddlesInterface::readBuffer(11));
}

//==================================================================================================
void ROCTimingPaddlesInterface::ReadRxFIFO(__ARGS__)
{
	__COUT_INFO__ << "ReadRxFIFO called" << __E__;
	__SET_ARG_OUT__("Result", ROCTimingPaddlesInterface::readBuffer(8));
}

//==================================================================================================
std::vector<uint16_t> ROCTimingPaddlesInterface::readHistogram(
    std::vector<DTCLib::roc_data_t>& histAddrs,
    uint32_t                         loc_addr,
    std::stringstream&               outss)
{
	std::vector<uint16_t> hist;
	uint16_t              binValue;
	for(int i = 0; i < 5; i++)
	{
		binValue = getDTC()->ReadExtROCRegister(linkID_, loc_addr, histAddrs[i]);
		hist.push_back(binValue);
		std::string label = "bin " + std::to_string(i - 2) + ":";
		outss << std::left << std::setw(12) << label << std::right << std::setw(5)
		      << binValue << __E__;
	}
	return hist;
}

//==================================================================================================
std::array<std::string, 4> ROCTimingPaddlesInterface::ReadMarkerHistograms(void)
{
	std::string histogramFD = histogramTemplate3Markers;
	std::string histogramPS = histogramTemplate1Marker;
	std::string histogramTM = histogramTemplate1Marker;

	std::vector<DTCLib::roc_data_t> loopbackHistAddrsFD = {30, 31, 32, 33, 34};
	std::vector<DTCLib::roc_data_t> loopbackHistAddrsPS = {27, 28, 29, 30, 31};
	std::vector<DTCLib::roc_data_t> clockkHistAddrsFD   = {20, 21, 22, 23, 24};
	std::vector<DTCLib::roc_data_t> eventHistAddrsFD    = {25, 26, 27, 28, 29};
	std::vector<DTCLib::roc_data_t> eventHistAddrsTM    = {1, 2, 3, 4, 5};
	std::map<std::string, std::vector<uint16_t>> histograms;
	std::stringstream                            outss;

	outss << "Loopback Marker Histogram (Forward Detector)" << __E__;
	histograms["loopbackFD"] = readHistogram(loopbackHistAddrsFD, 8, outss);

	outss << "Loopback Marker Histogram (Packet Sender)" << __E__;
	histograms["loopbackPS"] = readHistogram(loopbackHistAddrsPS, 11, outss);

	outss << "Clock Marker Histogram (Forward Detector)" << __E__;
	histograms["clockFD"] = readHistogram(clockkHistAddrsFD, 8, outss);

	outss << "Event Marker Histogram (Forward Detector)" << __E__;
	histograms["eventFD"] = readHistogram(eventHistAddrsFD, 8, outss);

	outss << "Event Marker Histogram (Timestamp Manager)" << __E__;
	histograms["eventTM"] = readHistogram(eventHistAddrsTM, 12, outss);

	std::string data =
	    "[" + StringMacros::vectorToString(histograms["loopbackFD"], ",") + "]";
	histogramFD = std::regex_replace(histogramFD, std::regex("<LOOPBACK>"), data);
	data        = "[" + StringMacros::vectorToString(histograms["clockFD"], ",") + "]";
	histogramFD = std::regex_replace(histogramFD, std::regex("<CLOCK>"), data);
	data        = "[" + StringMacros::vectorToString(histograms["eventFD"], ",") + "]";
	histogramFD = std::regex_replace(histogramFD, std::regex("<EVENT>"), data);
	histogramFD =
	    std::regex_replace(histogramFD, std::regex("<TITLE>"), "\"Forward Detector\"");

	data        = "[" + StringMacros::vectorToString(histograms["loopbackPS"], ",") + "]";
	histogramPS = std::regex_replace(histogramPS, std::regex("<MARKER-DATA>"), data);
	histogramPS = std::regex_replace(
	    histogramPS, std::regex("<MARKER-TYPE>"), "\"Loopback Marker\"");
	histogramPS =
	    std::regex_replace(histogramPS, std::regex("<TITLE>"), "\"Packet Sender\"");

	data        = "[" + StringMacros::vectorToString(histograms["eventTM"], ",") + "]";
	histogramTM = std::regex_replace(histogramTM, std::regex("<MARKER-DATA>"), data);
	histogramTM =
	    std::regex_replace(histogramTM, std::regex("<MARKER-TYPE>"), "\"Event Marker\"");
	histogramTM =
	    std::regex_replace(histogramTM, std::regex("<TITLE>"), "\"Timestamp Manager\"");

	std::array<std::string, 4> outArr = {
	    outss.str(), histogramFD, histogramPS, histogramTM};
	return outArr;
}

//======================================================================================================
void ROCTimingPaddlesInterface::ReadMarkerHistograms(__ARGS__)
{
	std::array<std::string, 4> result = ROCTimingPaddlesInterface::ReadMarkerHistograms();
	__SET_ARG_OUT__("histograms", "\n" + result[0]);
	__SET_ARG_OUT__("Plotly_Plot0", result[1]);
	__SET_ARG_OUT__("Plotly_Plot1", result[2]);
	__SET_ARG_OUT__("Plotly_Plot2", result[3]);
}

//======================================================================================================
void ROCTimingPaddlesInterface::ResetHistograms(__ARGS__)
{
	uint16_t resetControl = 0x0;
	bool     enable       = __GET_ARG_IN__(
        "Forward Detector Loopback Markers (Default := false)", bool, false);
	resetControl = resetControl | (enable << 0);
	enable =
	    __GET_ARG_IN__("Forward Detector Clock Markers (Default := false)", bool, false);
	resetControl = resetControl | (enable << 1);
	enable =
	    __GET_ARG_IN__("Forward Detector Event Markers (Default := false)", bool, false);
	resetControl = resetControl | (enable << 2);
	enable =
	    __GET_ARG_IN__("Timestamp Manager Event Markers (Default := false)", bool, false);
	resetControl = resetControl | (enable << 3);
	enable =
	    __GET_ARG_IN__("Packet Sender Loopback Markers(Default := false)", bool, false);
	resetControl = resetControl | (enable << 4);
	writeRegister(50, resetControl);
}

//======================================================================================================
void ROCTimingPaddlesInterface::Reset(__ARGS__) { writeRegister(0, 0); }

//======================================================================================================
void ROCTimingPaddlesInterface::SelectiveReset(__ARGS__)
{
	uint16_t resetControl = 0x0;
	bool enable = __GET_ARG_IN__("40MHz Clock Generator (Default := false)", bool, false);
	resetControl = resetControl | (enable << 0);
	enable       = __GET_ARG_IN__("Forward Detector (Default := false)", bool, false);
	resetControl = resetControl | (enable << 1);
	enable       = __GET_ARG_IN__("Command Handler (Default := false)", bool, false);
	resetControl = resetControl | (enable << 2);
	enable       = __GET_ARG_IN__("Timestamp Manager (Default := false)", bool, false);
	resetControl = resetControl | (enable << 3);
	enable       = __GET_ARG_IN__("DCS Receive FIFO (Default := false)", bool, false);
	resetControl = resetControl | (enable << 4);
	enable       = __GET_ARG_IN__("ROC Monitor (Default := false)", bool, false);
	resetControl = resetControl | (enable << 5);
	enable       = __GET_ARG_IN__("DCS Response FIFO (Default := false)", bool, false);
	resetControl = resetControl | (enable << 6);
	enable       = __GET_ARG_IN__("Packet Sender (Default := false)", bool, false);
	resetControl = resetControl | (enable << 7);
	enable       = __GET_ARG_IN__("Receive FIFO (Default := false)", bool, false);
	resetControl = resetControl | (enable << 8);
	enable       = __GET_ARG_IN__("Response FIFO (Default := false)", bool, false);
	resetControl = resetControl | (enable << 9);
	enable       = __GET_ARG_IN__("BERT (Default := false)", bool, false);
	resetControl = resetControl | (enable << 10);

	writeRegister(1, resetControl);
}

//======================================================================================================
void ROCTimingPaddlesInterface::PrepareDataRuns(__ARGS__)
{
	bool     enable      = __GET_ARG_IN__("Enable Delay (Default := true)", bool, true);
	uint16_t delayAmount = __GET_ARG_IN__("Delay amount (Default := 0)", uint16_t, 0);

	writeRegister(45, enable);
	writeRegister(21, delayAmount);
	readRegister(36);
}

//======================================================================================================
void ROCTimingPaddlesInterface::GetStatus(__ARGS__)
{
	std::stringstream outss;

	// XCVR Block Status
	// outss << "===================================" << __E__;
	// outss << "Transceiver Lock: " << std::hex << delayEnabled << __E__;
	// outss << "Transceiver Loss Counter: " << std::hex << delayEnabled << __E__;
	// outss << "Invalid K Char: " << std::hex << delayAmount << __E__;
	// outss << "Invalid RX Data: " << markersReceived << __E__;
	// outss << "Transceiver Aligned: " << markersSent << __E__;

	outss << "===================================" << __E__;
	outss << "Event Delay Enabled: 0x" << std::hex << readRegister(45) << __E__;
	outss << "Event Delay Amount: 0x" << std::hex << readRegister(21) << __E__;
	outss << "Loopback Markers Received: " << getDTC()->ReadExtROCRegister(linkID_, 8, 41)
	      << __E__;
	outss << "Loopback Markers Sent: " << getDTC()->ReadExtROCRegister(linkID_, 11, 32)
	      << __E__;

	outss << "Last Heart Beat Received: " << getDTC()->ReadExtROCRegister(linkID_, 12, 6)
	      << __E__;
	outss << "Last Data Request Packet: " << std::hex
	      << getDTC()->ReadExtROCRegister(linkID_, 9, 5) << std::hex
	      << getDTC()->ReadExtROCRegister(linkID_, 9, 4) << std::hex
	      << getDTC()->ReadExtROCRegister(linkID_, 9, 5) << __E__;

	outss << "Number of Heart Beat Packets: "
	      << getDTC()->ReadExtROCRegister(linkID_, 9, 22) << __E__;
	outss << "Number of Data Request Packets: "
	      << getDTC()->ReadExtROCRegister(linkID_, 9, 23) << __E__;
	outss << "Number of DCS Packets: " << getDTC()->ReadExtROCRegister(linkID_, 9, 5)
	      << __E__;

	// BER Status
	outss << "===================================" << __E__;
	DTCLib::roc_data_t BERstatus = readRegister(49);
	outss << "BER Block Status: " << std::bitset<3>(BERstatus) << __E__;
	outss << "XCVR Loss Lock Counter: " << readRegister(8) << __E__;
	// TODO: bit error count

	outss << "===================================" << __E__;
	outss << ROCTimingPaddlesInterface::ReadMarkerHistograms()[0];

	__SET_ARG_OUT__("Result", "\n" + outss.str());
}

//======================================================================================================
void ROCTimingPaddlesInterface::BERT(__ARGS__)
{
	bool enableTX = __GET_ARG_IN__("Transmit PRBS7 (Default := false)", bool, false);
	bool enableRX = __GET_ARG_IN__("Calculate BER (Default := false)", bool, false);

	DTCLib::roc_data_t enable = 0x0;
	enable                    = enableTX & (enableRX << 1);
	getDTC()->WriteExtROCRegister(linkID_, 13, 1, enable, 0, 1000);

	bool fixedPatternEnabled =
	    __GET_ARG_IN__("Fixed Pattern Enabled (Default := true)", bool, true);
	if(fixedPatternEnabled)
		getDTC()->WriteExtROCRegister(linkID_, 13, 6, 0, 0, 1000);
	else
		getDTC()->WriteExtROCRegister(linkID_, 13, 7, 0, 0, 1000);

	DTCLib::roc_data_t fixedPattern =
	    __GET_ARG_IN__("Fixed Pattern (Default := 0xBC3C)", DTCLib::roc_data_t, 0xBC3C);
	getDTC()->WriteExtROCRegister(linkID_, 13, 3, fixedPattern, 0, 1000);

	DTCLib::roc_data_t fixedKChar =
	    __GET_ARG_IN__("Fixed K Char (Default := 0X3)", DTCLib::roc_data_t, 0x3);
	getDTC()->WriteExtROCRegister(linkID_, 13, 2, fixedKChar, 0, 1000);

	DTCLib::roc_data_t loopbackModeEnabled =
	    __GET_ARG_IN__("Loopback Mode (Default := false)", bool, false);
	getDTC()->WriteExtROCRegister(linkID_, 13, 5, loopbackModeEnabled, 0, 1000);
}

DEFINE_OTS_INTERFACE(ROCTimingPaddlesInterface)
