#ifndef _ots_DBRunInfo_h_
#define _ots_DBRunInfo_h_

#include <libpq-fe.h>                                     /* for PGconn */
#include "otsdaq/FiniteStateMachine/RunInfoVInterface.h"  // for Run Info plugins
#include "otsdaq/TableCore/TableView.h"

namespace ots
{

class DBRunInfo : public RunInfoVInterface
{
  public:
	DBRunInfo(std::string interfaceUID);
	// const ConfigurationTree& theXDAQContextConfigTree,
	// const std::string&       configurationPath);
	virtual ~DBRunInfo(void);

	virtual unsigned int insertRunCondition(const std::string& runInfoConditions = "");
	virtual unsigned int claimNextRunNumber(unsigned int       conditionID,
	                                        const std::string& runInfoConditions = "");
	virtual void         updateRunInfo(unsigned int                   runNumber,
	                                   RunInfoVInterface::RunStopType runStopType);

	//start queryFilter with 'AND' to fiter more the selection
	virtual std::vector<std::vector<std::string>> getRunRecords(
	    unsigned int       startTime,
	    unsigned int       endTime,
	    const std::string& queryFilter = "");

	virtual std::vector<std::vector<std::string>> getRunConditionByID(
	    uint64_t conditionID);

	virtual std::vector<std::vector<std::string>> getRunConfigSubsystemInfo(
		uint64_t configID);

  private:
	const char* dbname_;
	const char* dbhost_;
	const char* dbport_;
	const char* dbuser_;
	const char* dbpwd_;
	const char* dbSchema_;
	PGconn*     runInfoDbConn_ = nullptr;

	void openDbConnection();
};
}  // namespace ots

#endif
