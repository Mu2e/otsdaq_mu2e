#include "otsdaq-mu2e/RunInfoPlugins/DBRunInfo.h"
#include "otsdaq/Macros/CoutMacros.h"
#include "otsdaq/Macros/RunInfoPluginMacros.h"
#include "otsdaq/MessageFacility/MessageFacility.h"

#include <libpq-fe.h> /* for PGconn */
#include <boost/algorithm/string.hpp>
#include <chrono>     

using namespace ots;

//==============================================================================
DBRunInfo::DBRunInfo(
    std::string              interfaceUID)
	// ,
    // const ConfigurationTree& theXDAQContextConfigTree,
    // const std::string&       configurationPath)
    : RunInfoVInterface(interfaceUID)//, theXDAQContextConfigTree, configurationPath)  
{
	dbname_ = const_cast < char *> (getenv("OTSDAQ_RUNINFO_DATABASE")? getenv("OTSDAQ_RUNINFO_DATABASE") : "run_info");
	dbhost_ = const_cast < char *> (getenv("OTSDAQ_RUNINFO_DATABASE_HOST")? getenv("OTSDAQ_RUNINFO_DATABASE_HOST") : "");
	dbport_ = const_cast < char *> (getenv("OTSDAQ_RUNINFO_DATABASE_PORT")? getenv("OTSDAQ_RUNINFO_DATABASE_PORT") : "");
	dbuser_ = const_cast < char *> (getenv("OTSDAQ_RUNINFO_DATABASE_USER")? getenv("OTSDAQ_RUNINFO_DATABASE_USER") : "");
	dbpwd_  = const_cast < char *> (getenv("OTSDAQ_RUNINFO_DATABASE_PWD")? getenv("OTSDAQ_RUNINFO_DATABASE_PWD") : "");
	dbSchema_  = const_cast < char *> (getenv("OTSDAQ_RUNINFO_DATABASE_SCHEMA")? getenv("OTSDAQ_RUNINFO_DATABASE_SCHEMA") : "production");

	//open db connection
	openDbConnection();
}

//==============================================================================
DBRunInfo::~DBRunInfo(void) { 
    PQfinish(runInfoDbConn_);
}

//==============================================================================
void DBRunInfo::openDbConnection()
{
	//open db connection
	char runInfoDbConnInfo [1024];
	sprintf(runInfoDbConnInfo, "dbname=%s host=%s port=%s  \
		user=%s password=%s", dbname_, dbhost_, dbport_, dbuser_, dbpwd_);
	runInfoDbConn_ = PQconnectdb(runInfoDbConnInfo);
}

//==============================================================================
unsigned int DBRunInfo::insertRunCondition(const std::string& runInfoConditions)
{
	unsigned int conditionID = (unsigned int)-1;

	__COUT__ << "insert Run Condition" << __E__;

	int runInfoDbConnStatus_ = 0;

	char* mu2eOwner = __ENV__("MU2E_OWNER");
	char* hostName = __ENV__("HOSTNAME");

	if(PQstatus(runInfoDbConn_) == CONNECTION_BAD)
	{
		__COUT__ << "Unable to connect to the run_info database inserting run condition\n" << __E__;
		PQfinish(runInfoDbConn_);

		//Try to open again the db connection
		openDbConnection();
		if(PQstatus(runInfoDbConn_) == CONNECTION_BAD)
		{
			__COUT__ << "Unable to connect for the second time to the run_info database inserting the run condition!\n" << __E__;
			PQfinish(runInfoDbConn_);
		}
		else
		{
			__COUT__ << "Connected to the run_info database after a second tentative inserting the run condition! hostName: " << hostName << " mu2eOwner: " << mu2eOwner << "\n" << __E__;
			runInfoDbConnStatus_ = 1;
		}
	}
	else
	{
		__COUT__ << "Connected to the run_info database inserting the run condition! hostName: " << hostName << " mu2eOwner: " << mu2eOwner << "\n" << __E__;
		runInfoDbConnStatus_ = 1;
	}

	// write run condition into db
	if(runInfoDbConnStatus_ == 1)
	{
		PGresult* res;
		char      buffer[4194304];

		//extract run condition from runInfoConditions
		std::string condition = runInfoConditions.substr(runInfoConditions.find("Configuration := ") + sizeof("Configuration := ") - 1);

		snprintf(buffer,
				sizeof(buffer),
				"INSERT INTO %s.run_condition(						\
											  condition				\
											, commit_time)			\
											  VALUES ('%s',CURRENT_TIMESTAMP) \
                                              RETURNING condition_id;",
				dbSchema_,
				condition.c_str());

		res = PQexec(runInfoDbConn_, buffer);

		if(PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			__SS__ << "INSERT INTO 'run_condition' DATABASE TABLE FAILED!!! PQ ERROR: " << PQresultErrorMessage(res)
				<< __E__;
			PQclear(res);
			__SS_THROW__;
		}

		if(PQntuples(res) == 1)
		{
			conditionID = atoi(PQgetvalue(res, 0, 0));
			__COUTV__(conditionID);
		}
		else
		{
			__SS__ << "RETRIVE CONDITION_ID FROM 'run_condition' DATABASE TABLE FAILED!!! PQ ERROR: " << PQresultErrorMessage(res) << __E__;
			PQclear(res);
			__SS_THROW__;
		}

		PQclear(res);
	}

	if(conditionID == (unsigned int)-1)
	{
		__SS__ << "Impossible condition_id not defined by run info plugin!" << __E__;
		__SS_THROW__;
	}

	return conditionID;
} //end insertRunCondition()

//==============================================================================
unsigned int DBRunInfo::claimNextRunNumber(unsigned int conditionID, const std::string& runInfoConditions)
{
	if(conditionID == (unsigned int)-1)
	{
		__SS__ << "Impossible condition ID number not retrived by run info plugin!" << __E__;
		__SS_THROW__;
	}

	unsigned int runNumber = (unsigned int)-1;
	__COUT__ << "claiming next Run Number" << __E__;
	__COUTV__(runInfoConditions);

	int runInfoDbConnStatus_ = 0;

	char* mu2eOwner = __ENV__("MU2E_OWNER");
	char* hostName = __ENV__("HOSTNAME");
	char* artadqPartition = __ENV__("ARTDAQ_PARTITION");

	if(PQstatus(runInfoDbConn_) == CONNECTION_BAD)
	{
		__COUT__ << "Unable to connect to the run_info database for insert new run number and info!\n" << __E__;
		PQfinish(runInfoDbConn_);

		//Try to open again the db connection
		openDbConnection();
		if(PQstatus(runInfoDbConn_) == CONNECTION_BAD)
		{
			__COUT__ << "Unable to connect for the second time to the run_info database to update the transition!\n" << __E__;
			PQfinish(runInfoDbConn_);
		}
		else
		{
			__COUT__ << "Connected to the run_info database after a second tentative for insert new run number and info! hostName: " << hostName << " mu2eOwner: " << mu2eOwner << "\n" << __E__;
			runInfoDbConnStatus_ = 1;
		}
	}
	else
	{
		__COUT__ << "Connected to the run_info database for insert new run number and info! hostName: " << hostName << " mu2eOwner: " << mu2eOwner << "\n" << __E__;
		runInfoDbConnStatus_ = 1;
	}

	// write run info into db
	if(runInfoDbConnStatus_ == 1)
	{
		PGresult* res;
		char      buffer[1024];

		// extract configuraiton name and version from runInfoConditions
        auto start_pos = runInfoConditions.find("Configuration := ");
        if(start_pos == std::string::npos ) {
            __SS__ << "Could not find \"Configuration := \" in the configuration dump. You might need to set \"ConfigurationDumpOnRunFormat\" of your StateMachine to \"All\"." << __E__
                   << "the supplied config dump is: " << runInfoConditions << __E__;
		    __SS_THROW__;
        }

		std::string runConfiguration = runInfoConditions.substr(start_pos + sizeof("Configuration := ") - 1);
		runConfiguration = runConfiguration.substr(0, runConfiguration.find(')'));

		std::string runConfigurationVersion = runConfiguration.substr(runConfiguration.find('(') + 1);
		boost::trim_right(runConfigurationVersion);

		runConfiguration = runConfiguration.substr(0, runConfiguration.find('('));
		boost::trim_right(runConfiguration);

		//extract context name and version from runInfoConditions
		std::string runContext = runInfoConditions.substr(runInfoConditions.find("Context := ") + sizeof("Context := ") - 1);
		runContext = runContext.substr(0, runContext.find(')'));

		std::string runContextVersion = runContext.substr(runContext.find('(') + 1);
		boost::trim_right(runContextVersion);

		runContext = runContext.substr(0, runContext.find('('));
		boost::trim_right(runContext);

		//insert a new row in the run_configuration table
		__COUT__ << "Insert new run info in the run_configuration database table, run configuration is: "
				 << runConfiguration
				 << " , run context is: "
				 << runContext
				 << __E__;

		char* runType  = const_cast < char *> (getenv("OTSDAQ_RUNINFO_DATABASE_RUNTYPE")? getenv("OTSDAQ_RUNINFO_DATABASE_RUNTYPE") : "1");

		snprintf(buffer,
				sizeof(buffer),
				"INSERT INTO %s.run_configuration(					\
											  run_type				\
											, condition_id			\
											, artdaq_partition		\
											, host_name				\
											, configuration_name	\
											, configuration_version	\
											, context_name			\
											, context_version		\
											, commit_time)			\
											VALUES ('%s','%d','%d','%s','%s','%s','%s','%s',CURRENT_TIMESTAMP) \
                                            RETURNING run_number;",
				dbSchema_,
				runType,
				conditionID,
				std::stoi(artadqPartition),
				hostName,
				runConfiguration.c_str(),
				runConfigurationVersion.c_str(),
				runContext.c_str(),
				runContextVersion.c_str());

		res = PQexec(runInfoDbConn_, buffer);

		if(PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			__SS__ << "INSERT INTO 'run_configuration' DATABASE TABLE FAILED!!! PQ ERROR: " << PQresultErrorMessage(res)
				<< __E__;
			PQclear(res);
			__SS_THROW__;
		}

		if(PQntuples(res) == 1)
		{
			runNumber = atoi(PQgetvalue(res, 0, 0));
			__COUTV__(runNumber);
		}
		else
		{
			__SS__ << "RETRIVE RUN NUMBER FROM 'run_configuration' DATABASE TABLE FAILED!!! PQ ERROR: " << PQresultErrorMessage(res) << __E__;
			PQclear(res);
			__SS_THROW__;
		}

		PQclear(res);

		// write run start transition into run_transition table
		updateRunInfo(runNumber, RunInfoVInterface::RunStopType::START);
	}

	if(runNumber == (unsigned int)-1)
	{
		__SS__ << "Impossible run number not defined by run info plugin!" << __E__;
		__SS_THROW__;
	}

	return runNumber;
} //end claimNextRunNumber()

//==============================================================================
void DBRunInfo::updateRunInfo(unsigned int runNumber, RunInfoVInterface::RunStopType runStopType)
{
	__COUT__ << "Updating run transition for run number " << runNumber << __E__;

	int runInfoDbConnStatus_ = 0;

	if(PQstatus(runInfoDbConn_) == CONNECTION_BAD)
	{
		__COUT__ << "Unable to connect to the run_info database to update the transition!\n" << __E__;
		PQfinish(runInfoDbConn_);
		
		//Try to open again the db connection
		openDbConnection();
		if(PQstatus(runInfoDbConn_) == CONNECTION_BAD)
		{
			__COUT__ << "Unable to connect for the second time to the run_info database to update the transition!\n" << __E__;
			PQfinish(runInfoDbConn_);
		}
		else
		{
			__COUT__ << "Connected after a second tentative to the run_info database to update the transition!\n" << __E__;
			runInfoDbConnStatus_ = 1;
		}
	}
	else
	{
		__COUT__ << "Connected to the run_info database to update the transition!\n" << __E__;
		runInfoDbConnStatus_ = 1;
	}

	// Insert the transition and time into db
	if(runInfoDbConnStatus_ == 1)
	{
		int runTransitionType;
		std::string transitionDescription = "";

		// Insert 'Running to Configure - Stop' transition and time into db
		if(runStopType == RunInfoVInterface::RunStopType::HALT)
		{
			runTransitionType = 0;
			transitionDescription = "'Running to Halt - Abort'";
		}

		// Insert 'Running to Configure - Stop' transition and time into db
		if(runStopType == RunInfoVInterface::RunStopType::STOP)
		{
			runTransitionType = 1;
			transitionDescription = "'Running to Configure - Stop'";
		}

		// Insert 'Running to Pause - Pause' transition and time into db
		if(runStopType == RunInfoVInterface::RunStopType::ERROR)
		{
			runTransitionType = 2;
			transitionDescription = "'Other state to Error - Error'";
		}

		// Insert 'Running to Pause - Pause' transition and time into db
		if(runStopType == RunInfoVInterface::RunStopType::PAUSE)
		{
			runTransitionType = 3;
			transitionDescription = "'Running to Pause - Pause'";
		}

		// Insert 'Pause to Running - Resume' transition and time into db
		if(runStopType == RunInfoVInterface::RunStopType::RESUME)
		{
			runTransitionType = 4;
			transitionDescription = "'Pause to Running - Resume'";
		}

		// Insert 'Pause to Running - Resume' transition and time into db
		if(runStopType == RunInfoVInterface::RunStopType::START)
		{
			runTransitionType = 5;
			transitionDescription = "'Configeure to Running - Start'";
		}

		PGresult* res;
		char      buffer[1024];

		snprintf(buffer,
				sizeof(buffer),
				"INSERT INTO %s.run_transition(					\
											  run_number		\
											, transition_type	\
											, transition_time)	\
											VALUES (%ld,'%d',CURRENT_TIMESTAMP);",
				dbSchema_,
				boost::numeric_cast<long int>(runNumber),
				boost::numeric_cast<int>(runTransitionType));

		res = PQexec(runInfoDbConn_, buffer);

		if(PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			__SS__ << "INSERT "<< transitionDescription << " TRANSITION INTO DATABASE TABLE FAILED!!! PQ ERROR: " << PQresultErrorMessage(res)
				<< __E__;
			PQclear(res);
			__SS_THROW__;
		}
		PQclear(res);

		__COUT__ << "Insert: "<< transitionDescription << " transition into the run_transition Database table" << __E__;
	}

	if(runNumber == (unsigned int)-1)
	{
		__SS__ << "Impossible run number not defined by run info plugin!" << __E__;
		__SS_THROW__;
	}
	
} //end updateRunInfo()

std::string DBRunInfo::getRunInfo(int runNumber) { 
    std::ostringstream oss;
    oss << "{\"name\":\"" << mfSubject_ << "\"";
    int runInfoDbConnStatus_ = 0;
	if(PQstatus(runInfoDbConn_) == CONNECTION_BAD)
	{
		__COUT__ << "Unable to connect to the run_info database to get run status!\n" << __E__;
		PQfinish(runInfoDbConn_);
		
		//Try to open again the db connection
		openDbConnection();
		if(PQstatus(runInfoDbConn_) == CONNECTION_BAD)
		{
			__COUT__ << "Unable to connect for the second time to the run_info database to get run status!\n" << __E__;
			PQfinish(runInfoDbConn_);
		}
		else
		{
			__COUT__ << "Connected after a second tentative to the run_info databaseto get run status!\n" << __E__;
			runInfoDbConnStatus_ = 1;
		}
	}
	else
	{
		__COUT__ << "Connected to the run_info database to get run status!\n" << __E__;
		runInfoDbConnStatus_ = 1;
	}

	// get run info
	if(runInfoDbConnStatus_ == 1)
	{
        char buffer[1024];
        PGresult* res;
        if(runNumber > 0) {
		    snprintf(buffer,
				sizeof(buffer),
                "SELECT r.run_number as run_number, host_name, artdaq_partition, configuration_name, commit_time, \
                transition_time, transition_description, run_type_description, \
                configuration_version, trigger_table_name, trigger_table_version, context_name, context_version \
                FROM %s.run_configuration r \
                INNER JOIN %s.run_transition rt ON  r.run_number = rt.run_number \
                INNER JOIN %s.transition_type tt ON rt.transition_type = tt.transition_id \
                INNER JOIN %s.run_type rct ON r.run_type = rct.run_type_id  \
                WHERE r.run_number = '%i' \
                ORDER BY transition_time limit 100;",
                //#inner join cause_type on run_transition.cause_type = cause_type.cause_id 
				dbSchema_, dbSchema_, dbSchema_, dbSchema_,
                runNumber);
        } else { // get RunLog
            snprintf(buffer,
                sizeof(buffer),
                "SELECT r.run_number as run_number, host_name, artdaq_partition, configuration_name, configuration_version, \
                    commit_time, transition_description, run_type_description, transition_time, \
                    configuration_version, trigger_table_name, trigger_table_version, context_name, context_version \
                FROM %s.run_configuration r \
                LEFT JOIN ( \
                    SELECT * \
                    FROM %s.run_transition \
                    WHERE (run_number, transition_time) IN ( \
                        SELECT run_number, MAX(transition_time) \
                        FROM %s.run_transition \
                        GROUP BY run_number \
                    ) \
                ) t ON r.run_number = t.run_number \
                INNER JOIN %s.transition_type tt ON transition_type = tt.transition_id  \
                INNER JOIN %s.run_type rct ON run_type  = rct.run_type_id \
                order by commit_time desc \
                limit %i;",
                dbSchema_, dbSchema_, dbSchema_, dbSchema_, dbSchema_,
                -runNumber);
        }

		res = PQexec(runInfoDbConn_, buffer);
		if(PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			__SS__ << buffer << " PQ ERROR: " << PQresultErrorMessage(res) << __E__;
			PQclear(res);
			__SS_THROW__;
		}

		//get run information
        if(runNumber > 0) {
            if(PQntuples(res) >= 1) {
                oss << ", ";
                oss << "\"run_number\":\""            << PQgetvalue(res, 0, PQfnumber(res, "run_number"))            << "\", ";
                oss << "\"host_name\":\""             << PQgetvalue(res, 0, PQfnumber(res, "host_name"))             << "\", ";
                oss << "\"artdaq_partition\":\""      << PQgetvalue(res, 0, PQfnumber(res, "artdaq_partition"))      << "\", ";
                oss << "\"configuration\":\""         << PQgetvalue(res, 0, PQfnumber(res, "configuration_name"))    << "\", ";
                oss << "\"configuration_version\":\"" << PQgetvalue(res, 0, PQfnumber(res, "configuration_version")) << "\", ";
                oss << "\"trigger_table\":\""         << PQgetvalue(res, 0, PQfnumber(res, "trigger_table_name"))    << "\", ";
                oss << "\"time\":\""                  << PQgetvalue(res, 0, PQfnumber(res, "commit_time"))           << "\", ";
                oss << "\"run_type\":\""              << PQgetvalue(res, 0, PQfnumber(res, "run_type_description"))  << "\", ";
                oss << "\"transitions\": [";
                for(int rowidx = 0; rowidx < PQntuples(res); ++rowidx) {
                    oss << "{";
                    oss << "\"type\":\""        << PQgetvalue(res, rowidx, PQfnumber(res, "transition_description")) << "\", ";
                    oss << "\"time\":\""        << PQgetvalue(res, rowidx, PQfnumber(res, "transition_time"))        << "\"";
                    if(rowidx < PQntuples(res)-1) oss << "}, ";
                    else                          oss << "} ";
                }
                oss << "]";
            } else {
                oss << ", \"error\":\"Run " << runNumber << " not found.\"";
            }
        } else if(runNumber < 0) { // RunLog
            oss << ", \"runs\": [";
                for(int rowidx = 0; rowidx < PQntuples(res); ++rowidx) {
                    oss << "{";
                    oss << "\"run_number\":\""             << PQgetvalue(res, rowidx, PQfnumber(res, "run_number"))             << "\", ";
                    oss << "\"host_name\":\""              << PQgetvalue(res, rowidx, PQfnumber(res, "host_name"))              << "\", ";
                    oss << "\"artdaq_partition\":\""       << PQgetvalue(res, rowidx, PQfnumber(res, "artdaq_partition"))       << "\", ";
                    oss << "\"configuration\":\""          << PQgetvalue(res, rowidx, PQfnumber(res, "configuration_name"))     << "\", ";
                    oss << "\"configuration_version\":\""  << PQgetvalue(res, rowidx, PQfnumber(res, "configuration_version"))  << "\", ";
                    oss << "\"trigger_table\":\""          << PQgetvalue(res, rowidx, PQfnumber(res, "trigger_table_name"))     << "\", ";
                    oss << "\"time\":\""                   << PQgetvalue(res, rowidx, PQfnumber(res, "commit_time"))            << "\", ";
                    oss << "\"last_transition\":\""        << PQgetvalue(res, rowidx, PQfnumber(res, "transition_description")) << "\", ";
                    oss << "\"last_transition_time\":\""   << PQgetvalue(res, rowidx, PQfnumber(res, "transition_time"))        << "\", ";
                    oss << "\"run_type\":\""               << PQgetvalue(res, rowidx, PQfnumber(res, "run_type_description"))   << "\" ";
                    if(rowidx < PQntuples(res)-1) oss << "}, ";
                    else                          oss << "}";
                }
            oss << "]";
        } else { // runNumber = 0, no actice run number found (by GatewaySupervisor)
            oss << ", \"error\":\"No active run number found.\"";
        }
        PQclear(res);
    } else {
        oss << ", \"error\":\"Database connection failed.\"";
    }

    oss << "}";
    return oss.str();
}

DEFINE_OTS_PROCESSOR(DBRunInfo)
