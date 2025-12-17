#ifndef _ots_DBRunInfo_h_
#define _ots_DBRunInfo_h_

#include <libpq-fe.h>                                     /* for PGconn */
#include "otsdaq/FiniteStateMachine/RunInfoVInterface.h"  // for Run Info plugins
#include "otsdaq/TableCore/TableView.h"
#include <sstream>
#include <vector>
#include <string>
#include <map>

namespace ots
{

// Structure to hold transition type mapping information
struct TransitionTypeInfo
{
	int         typeId;
	std::string description;
};

class DBRunInfo : public RunInfoVInterface
{
  public:
	DBRunInfo(std::string interfaceUID);
	// const ConfigurationTree& theXDAQContextConfigTree,
	// const std::string&       configurationPath);
	virtual ~DBRunInfo(void);

	virtual unsigned int insertRunCondition(const std::string& runInfoConditions = "",
	                                        const std::string& configTypeName = "");
	virtual unsigned int claimNextRunNumber(unsigned int       conditionID,
	                                        const std::string& runInfoConditions = "",
	                                        const std::string& comment = "");
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
	
	// Helper functions for error reporting
	std::vector<std::string> getTableNames(const std::string& tableName);
	void appendNotFoundError(std::stringstream& ss,
	                         const std::string& providedName,
	                         const std::string& tableName,
	                         const std::string& entityDescription,
	                         const std::vector<std::string>& availableNames,
	                         const std::string& additionalNote = "");
	
	// Helper function to get transition type information
	static TransitionTypeInfo getTransitionTypeInfo(RunInfoVInterface::RunStopType runStopType);
	
	// Helper function to check and reconnect database connection if needed
	int checkAndReconnectDb(const std::string& operationDescription);
	
	// Helper function to convert PGresult to vector<vector<string>>
	std::vector<std::vector<std::string>> convertResultToVector(PGresult* res);
};
}  // namespace ots

#endif
