#ifndef _ots_DBRunInfo_h_
#define _ots_DBRunInfo_h_

#include "otsdaq/FiniteStateMachine/RunInfoVInterface.h" // for Run Info plugins

namespace ots
{
class ConfigurationManager;
class DemoDQMHistos;

class DBRunInfo : public RunInfoVInterface
{
  public:
	DBRunInfo								(std::string              interfaceUID);
	                      					// const ConfigurationTree& theXDAQContextConfigTree,
	                      					// const std::string&       configurationPath);
	virtual ~DBRunInfo						(void);

	
	virtual unsigned int 	claimNextRunNumber	(void);
	virtual void 			updateRunInfo		(unsigned int runNumber, RunInfoVInterface::RunStopType runStopType);
};
}  // namespace ots

#endif
