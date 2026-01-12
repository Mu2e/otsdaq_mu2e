#ifndef _ots_test_utils_h_
#define _ots_test_utils_h_

#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq/FECore/FEVInterface.h"
#include "otsdaq/FECore/FEVInterfacesManager.h"

#include "otsdaq/ConfigurationInterface/ConfigurationInterface.h"
#include "otsdaq/FECore/MakeInterface.h"
#include "otsdaq/TableCore/MakeTable.h"

using namespace ots;

namespace ots::test::util
{
void check_and_make_envs()
{
	//==============================================================================
	// Define environment variables
	//	Note: normally these environment variables are set by ots script

	if(getenv("OTSDAQ_LOG_DIR") == NULL)
		setenv(
		    "OTSDAQ_LOG_DIR", (std::string(__ENV__("USER_DATA")) + "/Logs").c_str(), 1);

	if(getenv("OTSDAQ_LOG_ROOT") == NULL)
		setenv("OTSDAQ_LOG_ROOT", __ENV__("OTSDAQ_LOG_DIR"), 1);

	if(getenv("OTSDAQ_LOG_FHICL") == NULL)
		setenv("OTSDAQ_LOG_FHICL",
		       (std::string(__ENV__("USER_DATA")) +
		        "/MessageFacilityConfigurations/MessageFacilityWithCout_dev.fcl")
		           .c_str(),
		       1);

	// The configuration uses __ENV__("SERVICE_DATA_PATH") in init() so define it if it is not defined
	if(getenv("SERVICE_DATA_PATH") == NULL)
		setenv("SERVICE_DATA_PATH",
		       (std::string(__ENV__("USER_DATA")) + "/ServiceData").c_str(),
		       1);
}
}  // namespace ots::test::util

#endif
