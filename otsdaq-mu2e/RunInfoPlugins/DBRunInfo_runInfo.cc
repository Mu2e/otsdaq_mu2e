#include "otsdaq-mu2e/RunInfoPlugins/DBRunInfo.h"
#include "otsdaq/Macros/CoutMacros.h"
#include "otsdaq/Macros/RunInfoPluginMacros.h"
#include "otsdaq/MessageFacility/MessageFacility.h"

#include <libpq-fe.h> /* for PGconn */
#include <boost/algorithm/string.hpp>
#include <chrono>

using namespace ots;

//==============================================================================
DBRunInfo::DBRunInfo(std::string interfaceUID) : RunInfoVInterface(interfaceUID)
{
	dbname_   = const_cast<char*>(getenv("OTSDAQ_RUNINFO_DATABASE")
	                                  ? getenv("OTSDAQ_RUNINFO_DATABASE")
	                                  : "run_info");
	dbhost_   = const_cast<char*>(getenv("OTSDAQ_RUNINFO_DATABASE_HOST")
	                                  ? getenv("OTSDAQ_RUNINFO_DATABASE_HOST")
	                                  : "");
	dbport_   = const_cast<char*>(getenv("OTSDAQ_RUNINFO_DATABASE_PORT")
	                                  ? getenv("OTSDAQ_RUNINFO_DATABASE_PORT")
	                                  : "");
	dbuser_   = const_cast<char*>(getenv("OTSDAQ_RUNINFO_DATABASE_USER")
	                                  ? getenv("OTSDAQ_RUNINFO_DATABASE_USER")
	                                  : "");
	dbpwd_    = const_cast<char*>(getenv("OTSDAQ_RUNINFO_DATABASE_PWD")
	                                  ? getenv("OTSDAQ_RUNINFO_DATABASE_PWD")
	                                  : "");
	dbSchema_ = const_cast<char*>(getenv("OTSDAQ_RUNINFO_DATABASE_SCHEMA")
	                                  ? getenv("OTSDAQ_RUNINFO_DATABASE_SCHEMA")
	                                  : "test");

	//open db connection
	openDbConnection();
}  //end constructor()

//==============================================================================
DBRunInfo::~DBRunInfo(void)
{
	if(runInfoDbConn_)
		PQfinish(runInfoDbConn_);
}

//==============================================================================
void DBRunInfo::openDbConnection()
{
	__COUT__ << "Opening Run Info db connection at " << dbhost_ << ":" << dbport_
	         << __E__;
	//open db connection
	char runInfoDbConnInfo[1024];
	sprintf(runInfoDbConnInfo,
	        "dbname=%s host=%s port=%s  \
		user=%s password=%s connect_timeout=10",
	        dbname_,
	        dbhost_,
	        dbport_,
	        dbuser_,
	        dbpwd_);
	runInfoDbConn_ = PQconnectdb(runInfoDbConnInfo);

	if(PQstatus(runInfoDbConn_) != CONNECTION_OK)
	{
		__SS__ << "Connection failed: " << PQerrorMessage(runInfoDbConn_) << std::endl;
		PQfinish(runInfoDbConn_);
		runInfoDbConn_ = nullptr;
		__SS_THROW__;
	}
	__COUT__ << "Run Info db connection opened successfully at " << dbhost_ << ":"
	         << dbport_ << __E__;
}  //end openDbConnection()

//==============================================================================
unsigned int DBRunInfo::insertRunCondition(const std::string& runInfoConditions)
{
	uint64_t conditionID = (unsigned int)-1;

	__COUT__ << "insert Run Condition" << __E__;

	int runInfoDbConnStatus_ = 0;

	char* mu2eOwner = __ENV__("MU2E_OWNER");
	char* hostName  = __ENV__("HOSTNAME");

	if(PQstatus(runInfoDbConn_) == CONNECTION_BAD)
	{
		__COUT__ << "Unable to connect to the run_info database inserting run condition\n"
		         << __E__;
		PQfinish(runInfoDbConn_);
		runInfoDbConn_ = nullptr;

		//Try to open again the db connection
		openDbConnection();
		if(PQstatus(runInfoDbConn_) == CONNECTION_BAD)
		{
			__COUT__ << "Unable to connect for the second time to the run_info database "
			            "inserting the run condition!\n"
			         << __E__;
			PQfinish(runInfoDbConn_);
			runInfoDbConn_ = nullptr;
		}
		else
		{
			__COUT__ << "Connected to the run_info database after a second tentative "
			            "inserting the run condition! hostName: "
			         << hostName << " mu2eOwner: " << mu2eOwner << "\n"
			         << __E__;
			runInfoDbConnStatus_ = 1;
		}
	}
	else
	{
		__COUT__ << "Connected to the run_info database inserting the run condition! "
		            "hostName: "
		         << hostName << " mu2eOwner: " << mu2eOwner << "\n"
		         << __E__;
		runInfoDbConnStatus_ = 1;
	}

	// write run condition into db
	if(runInfoDbConn_ && runInfoDbConnStatus_ == 1)
	{
		PGresult* res;
		char      buffer[4194304];

		//extract run condition from runInfoConditions
		// std::string condition =
		//     runInfoConditions.substr(runInfoConditions.find("Configuration := ") +
		//                              sizeof("Configuration := ") - 1);
		// StringMacros::sanitizeForSQL(condition);

		//extract configuraiton name and version from runInfoConditions
		// std::string runConfiguration =
		//     runInfoConditions.substr(runInfoConditions.find("Configuration := ") +
		//                              sizeof("Configuration := ") - 1);
		// runConfiguration = runConfiguration.substr(0, runConfiguration.find(')'));

		// std::string runConfigurationVersion =
		//     runConfiguration.substr(runConfiguration.find('(') + 1);
		// boost::trim_right(runConfigurationVersion);
		// StringMacros::sanitizeForSQL(runConfigurationVersion);

		// runConfiguration = runConfiguration.substr(0, runConfiguration.find('('));
		// boost::trim_right(runConfiguration);
		// StringMacros::sanitizeForSQL(runConfiguration);

		// //extract context name and version from runInfoConditions
		// std::string runContext = runInfoConditions.substr(
		//     runInfoConditions.find("Context := ") + sizeof("Context := ") - 1);
		// runContext = runContext.substr(0, runContext.find(')'));

		// std::string runContextVersion = runContext.substr(runContext.find('(') + 1);
		// boost::trim_right(runContextVersion);
		// StringMacros::sanitizeForSQL(runContextVersion);

		// runContext = runContext.substr(0, runContext.find('('));
		// boost::trim_right(runContext);
		// StringMacros::sanitizeForSQL(runContext);

		// std::string backbone = runInfoConditions.substr(
		//     runInfoConditions.find("Backbone := ") + sizeof("Backbone := ") - 1);
		// backbone = backbone.substr(0, backbone.find(')'));

		// std::string backboneVersion = backbone.substr(backbone.find('(') + 1);
		// boost::trim_right(backboneVersion);
		// StringMacros::sanitizeForSQL(backboneVersion);

		// backbone = backbone.substr(0, backbone.find('('));
		// boost::trim_right(backbone);
		// StringMacros::sanitizeForSQL(backbone);

		// __COUT__ << "runInfoConditions " << runInfoConditions << __E__;
		// __COUT__ << "Info from parsering dump..." << __E__;
		// __COUT__ << "\tBackbone := " << backbone << " (" << backboneVersion << ")" <<  __E__;
		// __COUT__ << "\tContext := " << runContext << " (" << runContextVersion << ")" <<  __E__;
		// __COUT__ << "\tConfiguration := " << runConfiguration << " (" << runConfigurationVersion << ")" <<  __E__;

		// __COUT__ << "Run Condition before JSON conversion " << condition.c_str() << __E__;


		std::string runInfo = runInfoConditions;
		StringMacros::sanitizeForSQL(runInfo);
		__COUT__ << "Configuration dump " << __E__ << runInfo.c_str() << __E__;

		// std::string dummyData = "{\"Data\": \"hello\"}";

		snprintf(buffer,
		         sizeof(buffer),
		         "INSERT INTO %s.global_config(						\
											  config_data			\
											, create_time)			\
											  VALUES ('%s',CURRENT_TIMESTAMP) \
                                              RETURNING config_id;",
		         dbSchema_,
		         runInfo.c_str());

		res = PQexec(runInfoDbConn_, buffer);

		if(PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			__SS__ << "INSERT INTO 'global_config' DATABASE TABLE FAILED!!! PQ ERROR: "
			       << PQresultErrorMessage(res) << __E__;
			PQclear(res);
			__SS_THROW__;
		}

		if(PQntuples(res) == 1)
		{
			conditionID = std::stoul(PQgetvalue(res, 0, 0));
			__COUTV__(conditionID);
		}
		else
		{
			__SS__ << "RETRIVE CONDITION_ID FROM 'run_condition' DATABASE TABLE "
			          "FAILED!!! PQ ERROR: "
			       << PQresultErrorMessage(res) << __E__;
			PQclear(res);
			__SS_THROW__;
		}

		std::vector<std::vector<std::string>> subsystemsInfo = {
			{
				// "1" 	// config ID
				"1"  	// subsystem ID 
				,"{\"data\": \"test\"}"
				,"alias_1"
				,"context_1"
				,"1" 				// context group key 
				,"conf_group_name_1"
				,"1"	  			// config group key 
				,"backbone_name_1" 
				,"1"
				,"config_db_uir_1"
				,"subsystem_sw_version_id_1" 
			},
			{
				// "1" 	// config ID
				"2"  	// subsystem ID 
				,"{\"data\": \"test\"}"
				,"alias_2"
				,"context_2"
				,"1" 				// context group key 
				,"conf_group_name_2"
				,"1"	  			// config group key 
				,"backbone_name_2" 
				,"1"
				,"config_db_uir_2"
				,"subsystem_sw_version_id_1" 
			},
			{
				// "1" 	// config ID
				"3"  	// subsystem ID 
				,"{\"data\": \"test\"}"
				,"alias_3"
				,"context_3"
				,"1" 				// context group key 
				,"conf_group_name_3"
				,"1"	  			// config group key 
				,"backbone_name_3" 
				,"1"
				,"config_db_uir_3"
				,"subsystem_sw_version_id_1" 
			}
		};

		for(uint8_t i=0; i<subsystemsInfo.size(); i++)
		{
			snprintf(buffer,
		         sizeof(buffer),
		         "INSERT INTO %s.subsystem_config(						\
											  config_id					\
											, subsystem_id				\
											, subsystem_config_data		\
											, create_time)				\
											  VALUES ('%ld','%s','%s',CURRENT_TIMESTAMP) \
											  RETURNING config_id;",
		         dbSchema_,
				 conditionID,
				 subsystemsInfo[i][0].c_str(),
				 subsystemsInfo[i][1].c_str());

		
			res = PQexec(runInfoDbConn_, buffer);

			if(PQresultStatus(res) != PGRES_TUPLES_OK)
			{
				__SS__ << "INSERT INTO 'subsystem_config' DATABASE TABLE FAILED!!! PQ ERROR: "
					<< PQresultErrorMessage(res) << __E__;
				PQclear(res);
				__SS_THROW__;
			}

			PQclear(res);

			snprintf(buffer,
		         sizeof(buffer),
		         "INSERT INTO %s.subsystem_config_info(					\
											  config_id					\
											, subsystem_id				\
											, config_alias				\
											, context_name				\
											, context_key				\
											, config_group_name			\
											, config_group_key			\
											, backbone_name 			\
											, backbone_key				\
											, config_db_uri				\
											, subsystem_sw_version_id	\
											, create_time)				\
											  VALUES ('%ld','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',CURRENT_TIMESTAMP) \
											  RETURNING config_id;",
		         dbSchema_,
				 conditionID,
				 (subsystemsInfo[i][0]).c_str(),
				 (subsystemsInfo[i][2]).c_str(),
				 (subsystemsInfo[i][3]).c_str(),
				 (subsystemsInfo[i][4]).c_str(),
				 (subsystemsInfo[i][5]).c_str(),
				 (subsystemsInfo[i][6]).c_str(),
				 (subsystemsInfo[i][7]).c_str(),
				 (subsystemsInfo[i][8]).c_str(),
				 (subsystemsInfo[i][9]).c_str(),
				 (subsystemsInfo[i][10]).c_str());

		
			res = PQexec(runInfoDbConn_, buffer);

			if(PQresultStatus(res) != PGRES_TUPLES_OK)
			{
				__SS__ << "INSERT INTO 'subsystem_config_info' DATABASE TABLE FAILED!!! PQ ERROR: "
					<< PQresultErrorMessage(res) << __E__;
				PQclear(res);
				__SS_THROW__;
			}

			PQclear(res);

		}// end for loop
	}

	if(conditionID == (unsigned int)-1)
	{
		__SS__ << "Impossible condition_id not defined by run info plugin!" << __E__;
		__SS_THROW__;
	}

	return conditionID;
}  //end insertRunCondition()

//==============================================================================
unsigned int DBRunInfo::claimNextRunNumber(unsigned int       conditionID,
                                           const std::string& runInfoConditions)
{
	if(conditionID == (unsigned int)-1)
	{
		__SS__ << "Impossible condition ID number not retrived by run info plugin!"
		       << __E__;
		__SS_THROW__;
	}

	unsigned int runNumber = (unsigned int)-1;
	__COUT__ << "claiming next Run Number" << __E__;
	__COUTV__(runInfoConditions);

	int runInfoDbConnStatus_ = 0;

	char* mu2eOwner       = __ENV__("MU2E_OWNER");
	char* hostName        = __ENV__("HOSTNAME");
	char* artadqPartition = __ENV__("ARTDAQ_PARTITION");

	if(PQstatus(runInfoDbConn_) == CONNECTION_BAD)
	{
		__COUT__ << "Unable to connect to the run_info database for insert new run "
		            "number and info!\n"
		         << __E__;
		PQfinish(runInfoDbConn_);
		runInfoDbConn_ = nullptr;

		//Try to open again the db connection
		openDbConnection();
		if(PQstatus(runInfoDbConn_) == CONNECTION_BAD)
		{
			__COUT__ << "Unable to connect for the second time to the run_info database "
			            "to update the transition!\n"
			         << __E__;
			PQfinish(runInfoDbConn_);
			runInfoDbConn_ = nullptr;
		}
		else
		{
			__COUT__ << "Connected to the run_info database after a second tentative for "
			            "insert new run number and info! hostName: "
			         << hostName << " mu2eOwner: " << mu2eOwner << "\n"
			         << __E__;
			runInfoDbConnStatus_ = 1;
		}
	}
	else
	{
		__COUT__ << "Connected to the run_info database for insert new run number and "
		            "info! hostName: "
		         << hostName << " mu2eOwner: " << mu2eOwner << "\n"
		         << __E__;
		runInfoDbConnStatus_ = 1;
	}

	// write run info into db
	if(runInfoDbConn_ && runInfoDbConnStatus_ == 1)
	{
		PGresult* res;
		char      buffer[1024];

		//extract configuraiton name and version from runInfoConditions
		// std::string runConfiguration =
		//     runInfoConditions.substr(runInfoConditions.find("Configuration := ") +
		//                              sizeof("Configuration := ") - 1);
		// runConfiguration = runConfiguration.substr(0, runConfiguration.find(')'));

		// std::string runConfigurationVersion =
		//     runConfiguration.substr(runConfiguration.find('(') + 1);
		// boost::trim_right(runConfigurationVersion);
		// StringMacros::sanitizeForSQL(runConfigurationVersion);

		// runConfiguration = runConfiguration.substr(0, runConfiguration.find('('));
		// boost::trim_right(runConfiguration);
		// StringMacros::sanitizeForSQL(runConfiguration);

		// //extract context name and version from runInfoConditions
		// std::string runContext = runInfoConditions.substr(
		//     runInfoConditions.find("Context := ") + sizeof("Context := ") - 1);
		// runContext = runContext.substr(0, runContext.find(')'));

		// std::string runContextVersion = runContext.substr(runContext.find('(') + 1);
		// boost::trim_right(runContextVersion);
		// StringMacros::sanitizeForSQL(runContextVersion);

		// runContext = runContext.substr(0, runContext.find('('));
		// boost::trim_right(runContext);
		// StringMacros::sanitizeForSQL(runContext);

		//insert a new row in the runs table
		// __COUT__ << "Insert new run info in the runs database table, run "
		//             "configuration is: "
		//          << runConfiguration << " , run context is: " << runContext << __E__;

		char* runType = const_cast<char*>(getenv("OTSDAQ_RUNINFO_DATABASE_RUNTYPE")
		                                      ? getenv("OTSDAQ_RUNINFO_DATABASE_RUNTYPE")
		                                      : "1");

		// NOTES : 
		// "production" -- runs
		// "tests" -- runs

		// runs renamed to "runs"
		//    runs has less columns (moved to different table)
		// run_confition renamed global_config 

		int location_id = 12;


		snprintf(buffer,
		         sizeof(buffer),
		         "INSERT INTO %s.runs(								\
											  run_type_id			\
											, config_id				\
											, artdaq_partition		\
											, host_name				\
											, location_id			\
											, commit_time)			\
											VALUES ('%s','%d','%d','%s','%d',CURRENT_TIMESTAMP) \
                                            RETURNING run_id;",
		         dbSchema_,
		         runType,
		         conditionID,
		         std::stoi(artadqPartition),
		         hostName,
				 location_id);

		res = PQexec(runInfoDbConn_, buffer);

		if(PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			__SS__
			    << "INSERT INTO 'runs' DATABASE TABLE FAILED!!! PQ ERROR: "
			    << PQresultErrorMessage(res) << __E__;
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
			__SS__ << "RETRIVE RUN NUMBER FROM 'runs' DATABASE TABLE "
			          "FAILED!!! PQ ERROR: "
			       << PQresultErrorMessage(res) << __E__;
			PQclear(res);
			__SS_THROW__;
		}

		PQclear(res);

		// write run start transition into run_transition table
		updateRunInfo(runNumber, RunInfoVInterface::RunStopType::START);
	}

		// __SS__ << "Halting..." << __E__;
		// __SS_THROW__;

		//=========================================================================
		// READ DB 
		//=========================================================================

		// PGresult* readRes;
		// char      readBuffer[1024];
		// // SELECT *
		// // FROM runs
		// // ORDER BY run_id ASC
		// // LIMIT 1;

		// snprintf(readBuffer,
		//          sizeof(readBuffer),
		//          "SELECT * FROM %s.runs(		
		// 		ORDER BY run_id ASC 			
		// 		LIMIT;",
		//          dbSchema_);

		// readRes = PQexec(runInfoDbConn_, readBuffer);

		// if(PQresultStatus(readRes) != PGRES_TUPLES_OK)
		// {
		// 	__SS__
		// 	    << "INSERT INTO 'runs' DATABASE TABLE FAILED!!! PQ ERROR: "
		// 	    << PQresultErrorMessage(res) << __E__;
		// 	PQclear(res);
		// 	__SS_THROW__;
		// }

		// if(PQntuples(readRes) == 1)
		// {
		// 	runNumber = atoi(PQgetvalue(readRes, 0, 0));
		// 	__COUTV__(runNumber);
		// }
		// else
		// {
		// 	__SS__ << "RETRIVE RUN NUMBER FROM 'runs' DATABASE TABLE "
		// 	          "FAILED!!! PQ ERROR: "
		// 	       << PQresultErrorMessage(readRes) << __E__;
		// 	PQclear(res);
		// 	__SS_THROW__;
		// }

		// PQclear(readRes);

		// __SS__ << "Halting..." << __E__;
		// __SS_THROW__;

	if(runNumber == (unsigned int)-1)
	{
		__SS__ << "Impossible run number not defined by run info plugin!" << __E__;
		__SS_THROW__;
	}

	return runNumber;
}  //end claimNextRunNumber()

//==============================================================================
void DBRunInfo::updateRunInfo(unsigned int                   runNumber,
                              RunInfoVInterface::RunStopType runStopType)
{
	__COUT__ << "Updating run transition for run number " << runNumber << __E__;

	int runInfoDbConnStatus_ = 0;

	if(PQstatus(runInfoDbConn_) == CONNECTION_BAD)
	{
		__COUT__
		    << "Unable to connect to the run_info database to update the transition!\n"
		    << __E__;
		PQfinish(runInfoDbConn_);
		runInfoDbConn_ = nullptr;

		//Try to open again the db connection
		openDbConnection();
		if(PQstatus(runInfoDbConn_) == CONNECTION_BAD)
		{
			__COUT__ << "Unable to connect for the second time to the run_info database "
			            "to update the transition!\n"
			         << __E__;
			PQfinish(runInfoDbConn_);
			runInfoDbConn_ = nullptr;
		}
		else
		{
			__COUT__ << "Connected after a second tentative to the run_info database to "
			            "update the transition!\n"
			         << __E__;
			runInfoDbConnStatus_ = 1;
		}
	}
	else
	{
		__COUT__ << "Connected to the run_info database to update the transition!\n"
		         << __E__;
		runInfoDbConnStatus_ = 1;
	}

	// Insert the transition and time into db
	if(runInfoDbConn_ && runInfoDbConnStatus_ == 1)
	{
		int         runTransitionType;
		std::string transitionDescription = "";

		// Insert 'Running to Configure - Stop' transition and time into db
		if(runStopType == RunInfoVInterface::RunStopType::HALT)
		{
			runTransitionType     = 0;
			transitionDescription = "'Running to Halt - Abort'";
		}

		// Insert 'Running to Configure - Stop' transition and time into db
		if(runStopType == RunInfoVInterface::RunStopType::STOP)
		{
			runTransitionType     = 1;
			transitionDescription = "'Running to Configure - Stop'";
		}

		// Insert 'Running to Pause - Pause' transition and time into db
		if(runStopType == RunInfoVInterface::RunStopType::ERROR)
		{
			runTransitionType     = 2;
			transitionDescription = "'Other state to Error - Error'";
		}

		// Insert 'Running to Pause - Pause' transition and time into db
		if(runStopType == RunInfoVInterface::RunStopType::PAUSE)
		{
			runTransitionType     = 3;
			transitionDescription = "'Running to Pause - Pause'";
		}

		// Insert 'Pause to Running - Resume' transition and time into db
		if(runStopType == RunInfoVInterface::RunStopType::RESUME)
		{
			runTransitionType     = 4;
			transitionDescription = "'Pause to Running - Resume'";
		}

		// Insert 'Pause to Running - Resume' transition and time into db
		if(runStopType == RunInfoVInterface::RunStopType::START)
		{
			runTransitionType     = 5;
			transitionDescription = "'Configeure to Running - Start'";
		}

		StringMacros::sanitizeForSQL(
		    transitionDescription);  //in case transitionDescription is used instead of int

		PGresult* res;
		char      buffer[1024];

		snprintf(buffer,
		         sizeof(buffer),
		         "INSERT INTO %s.run_transition(					\
											  run_id				\
											, transition_type_id	\
											, transition_time)		\
											VALUES (%ld,'%d',CURRENT_TIMESTAMP);",
		         dbSchema_,
		         boost::numeric_cast<long int>(runNumber),
		         boost::numeric_cast<int>(runTransitionType));

		res = PQexec(runInfoDbConn_, buffer);

		if(PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			__SS__ << "INSERT " << transitionDescription
			       << " TRANSITION INTO DATABASE TABLE FAILED!!! PQ ERROR: "
			       << PQresultErrorMessage(res) << __E__;
			PQclear(res);
			__SS_THROW__;
		}
		PQclear(res);

		__COUT__ << "Insert: " << transitionDescription
		         << " transition into the run_transition Database table" << __E__;
	}

	if(runNumber == (unsigned int)-1)
	{
		__SS__ << "Impossible run number not defined by run info plugin!" << __E__;
		__SS_THROW__;
	}

	__COUT__ << "done with the run_info database for updating the transition!" << __E__;
}  //end updateRunInfo()

//==============================================================================
std::vector<std::vector<std::string>> DBRunInfo::getRunRecords(
    unsigned int startTime, unsigned int endTime, const std::string& queryFilter)
{
	__COUT__ << "getRunRecords() reached" << __E__;
	std::vector<std::vector<std::string>> runRecords;

	int runInfoDbConnStatus_ = 0;

	if(PQstatus(runInfoDbConn_) == CONNECTION_BAD)
	{
		__COUT__ << "Unable to connect to the run_info database to select run records"
		         << __E__;
		PQfinish(runInfoDbConn_);
		runInfoDbConn_ = nullptr;

		//Try to open again the db connection
		openDbConnection();
		if(PQstatus(runInfoDbConn_) == CONNECTION_BAD)
		{
			__COUT__ << "Unable to connect for the second time to the run_info database "
			            "to select run records"
			         << __E__;
			PQfinish(runInfoDbConn_);
			runInfoDbConn_ = nullptr;
		}
		else
		{
			__COUT__ << "Connected to the run_info database to select run records"
			         << __E__;
			runInfoDbConnStatus_ = 1;
		}
	}
	else
	{
		__COUT__ << "Connected to the run_info database to select run records" << __E__;
		runInfoDbConnStatus_ = 1;
	}

	// select run info from db
	if(runInfoDbConn_ && runInfoDbConnStatus_ == 1)
	{
		PGresult*   res;
		char        buffer[2048];
		std::string row;

		snprintf(
		    buffer,
		    sizeof(buffer),
		    "SELECT runs.run_id as run_number"
		    ", runs.commit_time as run_time"
		    ", run_type.run_type_description as run_type"
		    ", runs.artdaq_partition"
		    ", runs.host_name"
		    ", runs.config_id"
		    // ", runs.configuration_name"
		    // ", runs.configuration_version"
		    // ", runs.context_name"
		    // ", runs.context_version"
		    // ", runs.online_software_version"
		    ", runs.shifter_comment" // runs.auto_comment for new schema 
		    ", MAX(CASE WHEN transition_type.transition_description LIKE '%%Start' THEN "
		    "run_transition.transition_time END) AS start_time"
		    ", MAX(CASE WHEN transition_type.transition_description LIKE '%%Stop' THEN "
		    "run_transition.transition_time END) AS stop_time"
		    " FROM %s.runs, %s.run_type, %s.run_transition, %s.transition_type"
		    " WHERE runs.run_type_id = run_type.run_type_id"
		    " AND runs.run_id = run_transition.run_id"
		    // " AND run_transition.transition_type_id = transition_type.transition_id"
		    " AND (transition_type.transition_description LIKE '%%Start' OR "
		    "transition_type.transition_description LIKE '%%Stop')"
		    " AND runs.commit_time BETWEEN TO_TIMESTAMP(\'%d\') AND "
		    "TO_TIMESTAMP(\'%d\')"
		    " %s"
		    " GROUP BY"
		    "	runs.run_id, run_type.run_type_description"
		    " HAVING"
		    "	COUNT(DISTINCT CASE"
		    "		WHEN transition_type.transition_description LIKE '%%Start' THEN "
		    "'Start'"
		    "		WHEN transition_type.transition_description LIKE '%%Stop' THEN 'Stop'"
		    "	END) = 2;",
		    dbSchema_,
		    dbSchema_,
		    dbSchema_,
		    dbSchema_,
		    startTime,
		    endTime,
		    queryFilter.c_str());

		res = PQexec(runInfoDbConn_, buffer);

		if(PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			__SS__ << "getRunRecords() SELECT FROM 'runs' DATABASE TABLE "
			          "FAILED!!! PQ ERROR: "
			       << PQresultErrorMessage(res) << __E__;
			PQclear(res);
			__SS_THROW__;
		}

		__COUT__ << "PQntuples(res) " << PQntuples(res) << "Query: " << buffer << __E__;
		if(PQntuples(res) >= 1)
		{
			/* first, print out the attribute names */
			int nFields = PQnfields(res);
			runRecords.resize(PQntuples(res));

			/* next, print out the rows */
			for(int i = 0; i < PQntuples(res); i++)
			{
				runRecords[i].resize(nFields);
				for(int j = 0; j < nFields; j++)
				{
					runRecords[i][j] = PQgetvalue(res, i, j);
					row.append(PQgetvalue(res, i, j));
					row.append(" ");
				}
				row.append("\n");
			}
			__COUT__ << "Run records retrived" << __E__;
		}
		else
		{
			// __SS__ << "getRunRecords() RETRIVE RUN RECORDS FROM 'runs' DATABASE TABLE "
			//           "FAILED!!! PQ ERROR: "
			//        << PQresultErrorMessage(res) << __E__;
			// PQclear(res);
			// __SS_THROW__;
		}

		PQclear(res);
	}

	return runRecords;
}  //end getRunRecords()


//==============================================================================
std::vector<std::vector<std::string>> DBRunInfo::getRunConfigSubsystemInfo(uint64_t configID)
{
	std::vector<std::vector<std::string>> configRecords;
	PGresult*   res;
	char        buffer[2048];
	std::string row;

	__COUT__ << "configID " << configID << __E__;

	snprintf(
		buffer,
		sizeof(buffer),
		" SELECT sc.config_id, sc.subsystem_id, sc.subsystem_config_data, sci.config_alias, sci.context_name, "
		" sci.context_key, sci.config_group_name, sci.config_group_key, sci.backbone_name, sci.backbone_key, " 
		" sci.config_db_uri, sci.subsystem_sw_version_id, sci.create_time "
		" FROM %s.subsystem_config as sc, %s.subsystem_config_info as sci"
		" WHERE sc.config_id = \'%ld\' AND sc.config_id = sci.config_id AND sc.subsystem_id = sci.subsystem_id;",
		dbSchema_,
		dbSchema_,
		configID);

		res = PQexec(runInfoDbConn_, buffer);

		if(PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			__SS__ << "getRunConfigSubsystemInfo() SELECT FROM 'subsystem_config' DATABASE TABLE "
			          "FAILED!!! PQ ERROR: "
			       << PQresultErrorMessage(res) << __E__;
			PQclear(res);
			__SS_THROW__;
		}

		__COUT__ << "PQntuples(res) " << PQntuples(res) << "Query: " << buffer << __E__;

		if(PQntuples(res) >= 1)
		{
			/* first, print out the attribute names */
			int nFields = PQnfields(res);
			configRecords.resize(PQntuples(res));

			/* next, print out the rows */
			for(int i = 0; i < PQntuples(res); i++)
			{
				configRecords[i].resize(nFields);
				for(int j = 0; j < nFields; j++)
				{
					configRecords[i][j] = PQgetvalue(res, i, j);
					row.append(PQgetvalue(res, i, j));
					row.append(" ");
				}
				row.append("\n");
			}
			__COUT__ << "Subsystem config retrieved" << __E__;
		}

		return configRecords;
} //end getRunConfigSubsystemInfo()

//==============================================================================
// TODO: change function name to config ID 
std::vector<std::vector<std::string>> DBRunInfo::getRunConditionByID(uint64_t conditionID)
{
	__COUT__ << "getRunConditionByID() reached" << __E__;
	std::vector<std::vector<std::string>> conditionRecords;

	int runInfoDbConnStatus_ = 0;

	if(PQstatus(runInfoDbConn_) == CONNECTION_BAD)
	{
		__COUT__
		    << "Unable to connect to the run_info database to select run condition record"
		    << __E__;
		PQfinish(runInfoDbConn_);
		runInfoDbConn_ = nullptr;

		//Try to open again the db connection
		openDbConnection();
		if(PQstatus(runInfoDbConn_) == CONNECTION_BAD)
		{
			__COUT__ << "Unable to connect for the second time to the run_info database "
			            "to select run condition record"
			         << __E__;
			PQfinish(runInfoDbConn_);
			runInfoDbConn_ = nullptr;
		}
		else
		{
			__COUT__
			    << "Connected to the run_info database to select run condition record"
			    << __E__;
			runInfoDbConnStatus_ = 1;
		}
	}
	else
	{
		__COUT__ << "Connected to the run_info database to select run condition record"
		         << __E__;
		runInfoDbConnStatus_ = 1;
	}

	// select run info from db
	if(runInfoDbConn_ && runInfoDbConnStatus_ == 1)
	{
		PGresult*   res;
		char        buffer[1024];
		std::string row;

		snprintf(buffer,
		         sizeof(buffer),
		         "SELECT global_config.config_data"
		         ", global_config.create_time"
		         " FROM %s.global_config"
		         " WHERE global_config.config_id = \'%ld\';",
		         dbSchema_,
		         conditionID);

		res = PQexec(runInfoDbConn_, buffer);

		if(PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			__SS__ << "getRunRecords() SELECT FROM 'run_condition' DATABASE TABLE "
			          "FAILED!!! PQ ERROR: "
			       << PQresultErrorMessage(res) << __E__;
			PQclear(res);
			__SS_THROW__;
		}

		__COUT__ << "PQntuples(res) " << PQntuples(res) << __E__;
		if(PQntuples(res) >= 1)
		{
			/* first, print out the attribute names */
			int nFields = PQnfields(res);
			conditionRecords.resize(PQntuples(res));

			/* next, print out the rows */
			for(int i = 0; i < PQntuples(res); i++)
			{
				conditionRecords[i].resize(nFields);
				for(int j = 0; j < nFields; j++)
				{
					conditionRecords[i][j] = PQgetvalue(res, i, j);
					row.append(PQgetvalue(res, i, j));
					row.append(" ");
				}
				row.append("\n");
			}
			__COUT__ << "Run condition record retrived" << __E__;
		}
		else
		{
			__SS__ << "getRunConditionByID() RETRIVE RUN CONDITION RECORD FROM "
			          "'run_condition' DATABASE TABLE "
			          "FAILED!!! PQ ERROR: "
			       << PQresultErrorMessage(res) << __E__;
			PQclear(res);
			__SS_THROW__;
		}

		PQclear(res);
	}

	return conditionRecords;
}  //end getRunConditionByID()

DEFINE_OTS_PROCESSOR(DBRunInfo)
