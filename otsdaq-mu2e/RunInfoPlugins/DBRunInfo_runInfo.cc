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
	dbSchema_  = const_cast < char *> (getenv("OTSDAQ_RUNINFO_DATABASE_SCHEMA")? getenv("OTSDAQ_RUNINFO_DATABASE_SCHEMA") : "public");

	//open db connection
	openDbConnection();
}

//==============================================================================
DBRunInfo::~DBRunInfo(void) { ; }

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
unsigned int DBRunInfo::claimNextRunNumber(const std::string& runInfoConditions)
{
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

		//extract configuraiton name and version from runInfoConditions
		std::string runConfiguration = runInfoConditions.substr(runInfoConditions.find("Configuration := ") + sizeof("Configuration := ") - 1);
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
											, host_name				\
											, artdaq_partition		\
											, configuration_name	\
											, configuration_version	\
											, commit_time)			\
											VALUES ('%s','%s','%d','%s','%s',CURRENT_TIMESTAMP);",
				dbSchema_,
				runType,
				hostName,
				std::stoi(artadqPartition),
				runConfiguration.c_str(),
				runConfigurationVersion.c_str());

		res = PQexec(runInfoDbConn_, buffer);

		if(PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			__SS__ << "INSERT INTO 'run_configuration' DATABASE TABLE FAILED!!! PQ ERROR: " << PQresultErrorMessage(res)
				<< __E__;
			PQclear(res);
			__SS_THROW__;
		}

		PQclear(res);

		snprintf(buffer,
				sizeof(buffer),
				"select max(run_number) from %s.run_configuration;",
				dbSchema_);

		res = PQexec(runInfoDbConn_, buffer);

		if(PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			__SS__ << "SELECT FROM 'run_configuration' DATABASE TABLE FAILED!!! PQ ERROR: " << PQresultErrorMessage(res) << __E__;
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
		__COUT__ << "Insert new start run transition in the run_transition database table" << __E__;

		int transitionType = 5; // START SAME TYPE IN THE DB

		snprintf(buffer,
				sizeof(buffer),
				"INSERT INTO %s.run_transition(					\
											  run_number		\
											, transition_type	\
											, transition_time)	\
											VALUES (%ld,'%d',CURRENT_TIMESTAMP);",
				dbSchema_,
				boost::numeric_cast<long int>(runNumber),
				boost::numeric_cast<int>(transitionType));

		res = PQexec(runInfoDbConn_, buffer);

		if(PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			__SS__ << "INSERT INTO 'run_transition' DATABASE TABLE FAILED!!! PQ ERROR: " << PQresultErrorMessage(res)
				<< __E__;
			PQclear(res);
			__SS_THROW__;
		}

		PQclear(res);

		//insert a new row in the run_condition table if it pk(runConfiguration,runConfigurationVersion, runContext,runContextVersion) doesn't exist yet
		snprintf(buffer,
				sizeof(buffer),
				"SELECT configuration_name, configuration_version, context_name, context_version \
				 FROM %s.run_condition WHERE 		\
				 configuration_name 	= '%s' 	AND \
				 configuration_version	= '%s'	AND \
				 context_name			= '%s'	AND \
				 context_version		= '%s';",
				dbSchema_,
				runConfiguration.c_str(),
				runConfigurationVersion.c_str(),
				runContext.c_str(),
				runContextVersion.c_str());

		res = PQexec(runInfoDbConn_, buffer);

		if(PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			__SS__ << "SELECT FROM 'run_condition' DATABASE TABLE FAILED!!! PQ ERROR: " << PQresultErrorMessage(res) << __E__;
			PQclear(res);
			__SS_THROW__;
		}

		//get primary key in the run_condiotion table
		if(PQntuples(res) != 1)
		{
			std::string pk;
		    int nFields = PQnfields(res);
			for (int i = 0; i < PQntuples(res); i++)
    		{
        		for (int j = 0; j < nFields; j++)
					pk += PQgetvalue(res, i, j);
   			}

			__COUTV__(pk);
			std::string pkRun = runConfiguration + runConfigurationVersion + runContext + runContextVersion;
			if (strcmp(pk.c_str(), pkRun.c_str()))
			{
				PQclear(res);
				char buffer2[4194304];

				snprintf(buffer2,
				sizeof(buffer2),
				"INSERT INTO %s.run_condition(						\
											  configuration_name	\
											, configuration_version	\
											, context_name			\
											, context_version		\
											, condition				\
											, commit_time)			\
											VALUES ('%s','%s','%s','%s','%s',CURRENT_TIMESTAMP);",
				dbSchema_,
				runConfiguration.c_str(),
				runConfigurationVersion.c_str(),
				runContext.c_str(),
				runContextVersion.c_str(),
				runInfoConditions.c_str());

				res = PQexec(runInfoDbConn_, buffer2);

				if(PQresultStatus(res) != PGRES_COMMAND_OK)
				{
					__SS__ << "INSERT INTO 'run_condition' DATABASE TABLE FAILED!!! PQ ERROR: " << PQresultErrorMessage(res)
						<< __E__;
					PQclear(res);
					__SS_THROW__;
				}
			}
		}
		else
		{
			__COUT__ << "PRIMARY KEY FROM 'run_condition' DATABASE TABLE EXIST. NOTHING TO DO. NEW CONFIGURATION KEY HAS NOT BEEN REGISTERD" << __E__;
		}

		PQclear(res);
	}

	//close db connection
	// if(PQstatus(runInfoDbConn_) == CONNECTION_OK)
	// {
	// 	PQfinish(runInfoDbConn_);
	// 	__COUT__ << "run_info DB CONNECTION CLOSED\n" << __E__;
	// }

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

	//close db connection
	// if(PQstatus(runInfoDbConn_) == CONNECTION_OK)
	// {
	// 	PQfinish(runInfoDbConn_);
	// 	__COUT__ << "run_info DB CONNECTION CLOSED\n" << __E__;
	// }

	if(runNumber == (unsigned int)-1)
	{
		__SS__ << "Impossible run number not defined by run info plugin!" << __E__;
		__SS_THROW__;
	}
	
} //end updateRunInfo()


DEFINE_OTS_PROCESSOR(DBRunInfo)
