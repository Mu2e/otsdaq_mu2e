#include "otsdaq-mu2e/RunInfoPlugins/DBRunInfo.h"
#include "otsdaq/Macros/CoutMacros.h"
#include "otsdaq/Macros/RunInfoPluginMacros.h"
#include "otsdaq/MessageFacility/MessageFacility.h"

#include <libpq-fe.h> /* for PGconn */

using namespace ots;

//==============================================================================
DBRunInfo::DBRunInfo(
    std::string              interfaceUID)
	// ,
    // const ConfigurationTree& theXDAQContextConfigTree,
    // const std::string&       configurationPath)
    : RunInfoVInterface(interfaceUID)//, theXDAQContextConfigTree, configurationPath)  
{
}

//==============================================================================
DBRunInfo::~DBRunInfo(void) { ; }

//==============================================================================
unsigned int DBRunInfo::claimNextRunNumber(void)
{
	unsigned int runNumber = (unsigned int)-1;
	__COUT__ << "claiming next Run Number" << __E__;

	int runInfoDbConnStatus_ = 0;
	char* dbname_ = const_cast < char *> (getenv("OTSDAQ_RUNINFO_DATABASE")? getenv("OTSDAQ_RUNINFO_DATABASE") : "prototype_run_info");
	char* dbhost_ = const_cast < char *> (getenv("OTSDAQ_RUNINFO_DATABASE_HOST")? getenv("OTSDAQ_RUNINFO_DATABASE_HOST") : "");
	char* dbport_ = const_cast < char *> (getenv("OTSDAQ_RUNINFO_DATABASE_PORT")? getenv("OTSDAQ_RUNINFO_DATABASE_PORT") : "");
	char* dbuser_ = const_cast < char *> (getenv("OTSDAQ_RUNINFO_DATABASE_USER")? getenv("OTSDAQ_RUNINFO_DATABASE_USER") : "");
	char* dbpwd_  = const_cast < char *> (getenv("OTSDAQ_RUNINFO_DATABASE_PWD")? getenv("OTSDAQ_RUNINFO_DATABASE_PWD") : "");

	//open db connection
	char runInfoDbConnInfo [1024];
	sprintf(runInfoDbConnInfo, "dbname=%s host=%s port=%s  \
		user=%s password=%s", dbname_, dbhost_, dbport_, dbuser_, dbpwd_);
	PGconn* runInfoDbConn = PQconnectdb(runInfoDbConnInfo);

	if(PQstatus(runInfoDbConn) == CONNECTION_BAD)
	{
		__COUT__ << "Unable to connect to prototype_run_info database for insert new run number and info!\n" << __E__;
		PQfinish(runInfoDbConn);
	}
	else
	{
		__COUT__ << "Connected to prototype_run_info database for insert new run number and info!\n" << __E__;
		runInfoDbConnStatus_ = 1;
	}

	// write run info into db
	if(runInfoDbConnStatus_ == 1)
	{
		PGresult* res;
		char      buffer[1024];
		__COUT__ << "Insert new run info in the run_info Database table" << __E__;
		snprintf(buffer,
				sizeof(buffer),
				"INSERT INTO public.run_info(				\
											run_type		\
											, user_name		\
											, host_name		\
											, start_time	\
											, note)			\
											VALUES ('%s','%s','%s',TO_TIMESTAMP(%ld),'%s');",
				"T",
				__ENV__("MU2E_OWNER"),
				__ENV__("HOSTNAME"),
				time(NULL),
				"some note");

		res = PQexec(runInfoDbConn, buffer);

		if(PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			__SS__ << "RUN INFO INSERT INTO DATABASE TABLE FAILED!!! PQ ERROR: " << PQresultErrorMessage(res)
				<< __E__;
			PQclear(res);
			__SS_THROW__;
		}
		PQclear(res);

		res = PQexec(runInfoDbConn, "select max(run_number) from public.run_info;");

		if(PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			__SS__ << "RUN INFO SELECT FROM DATABASE TABLE FAILED!!! PQ ERROR: " << PQresultErrorMessage(res) << __E__;
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
			__SS__ << "RETRIVE RUN NUMBER FROM DATABASE TABLE FAILED!!! PQ ERROR: " << PQresultErrorMessage(res) << __E__;
			PQclear(res);
			__SS_THROW__;
		}

		PQclear(res);
	}

	//close db connection
	if(PQstatus(runInfoDbConn) == CONNECTION_OK)
	{
		PQfinish(runInfoDbConn);
		__COUT__ << "prototype_run_info DB CONNECTION CLOSED\n" << __E__;
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
	__COUT__ << "Updating run info for run number " << runNumber << " " <<
		(runStopType == RunInfoVInterface::RunStopType::HALT?"HALT":
		(runStopType == RunInfoVInterface::RunStopType::STOP?"STOP":"ERROR")
		) << __E__;

	int runInfoDbConnStatus_ = 0;
	char* dbname_ = const_cast < char *> (getenv("OTSDAQ_RUNINFO_DATABASE")? getenv("OTSDAQ_RUNINFO_DATABASE") : "prototype_run_info");
	char* dbhost_ = const_cast < char *> (getenv("OTSDAQ_RUNINFO_DATABASE_HOST")? getenv("OTSDAQ_RUNINFO_DATABASE_HOST") : "");
	char* dbport_ = const_cast < char *> (getenv("OTSDAQ_RUNINFO_DATABASE_PORT")? getenv("OTSDAQ_RUNINFO_DATABASE_PORT") : "");
	char* dbuser_ = const_cast < char *> (getenv("OTSDAQ_RUNINFO_DATABASE_USER")? getenv("OTSDAQ_RUNINFO_DATABASE_USER") : "");
	char* dbpwd_  = const_cast < char *> (getenv("OTSDAQ_RUNINFO_DATABASE_PWD")? getenv("OTSDAQ_RUNINFO_DATABASE_PWD") : "");

	//open db connection
	char runInfoDbConnInfo [1024];
	sprintf(runInfoDbConnInfo, "dbname=%s host=%s port=%s  \
		user=%s password=%s", dbname_, dbhost_, dbport_, dbuser_, dbpwd_);
	PGconn* runInfoDbConn = PQconnectdb(runInfoDbConnInfo);

	if(PQstatus(runInfoDbConn) == CONNECTION_BAD)
	{
		__COUT__ << "Unable to connect to prototype_run_info database for update!\n" << __E__;
		PQfinish(runInfoDbConn);
	}
	else
	{
		__COUT__ << "Connected to prototype_run_info database for update!\n" << __E__;
		runInfoDbConnStatus_ = 1;
	}

	// Update run info pause into db
	if(runInfoDbConnStatus_ == 1 && runStopType == RunInfoVInterface::RunStopType::PAUSE)
	{
		PGresult* res;
		char      buffer[1024];
		std::string pause = "";

		snprintf(buffer, sizeof(buffer),  "select pause_time from public.run_info where run_number=%d ;", runNumber);
		res = PQexec(runInfoDbConn, buffer);

		if(PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			__SS__ << "RUN INFO PAUSE SELECT FROM DATABASE TABLE FAILED!!! PQ ERROR: " << PQresultErrorMessage(res) << __E__;
			PQclear(res);
			__SS_THROW__;
		}

		if(PQntuples(res) == 1)
		{
			pause = PQgetvalue(res, 0, 0);
			if (pause=="")
				pause.append(std::to_string(time(NULL)));
			else
				pause.append(";" + std::to_string(time(NULL)));

			__COUTV__(pause);
		}
		else
		{
			__SS__ << "RETRIVE PAUSE FROM RUN_INFO DATABASE TABLE FAILED!!! PQ ERROR: " << PQresultErrorMessage(res) << __E__;
			PQclear(res);
			__SS_THROW__;
		}

		PQclear(res);

		__COUT__ << "Update run info pause in the run_info Database table" << __E__;
		snprintf(buffer,
				sizeof(buffer),
				"UPDATE public.run_info SET pause_time='%s' WHERE run_number=%d;", pause.c_str(), runNumber);

		res = PQexec(runInfoDbConn, buffer);

		if(PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			__SS__ << "RUN INFO PAUSE UPDATE INTO DATABASE TABLE FAILED!!! PQ ERROR: " << PQresultErrorMessage(res)
				<< __E__;
			PQclear(res);
			__SS_THROW__;
		}
		PQclear(res);
	}

	// Update run info stop into db
	if(runInfoDbConnStatus_ == 1
		&& (runStopType == RunInfoVInterface::RunStopType::HALT
		|| runStopType == RunInfoVInterface::RunStopType::STOP))
	{
		PGresult* res;
		char      buffer[1024];
		std::string note = "";

		snprintf(buffer, sizeof(buffer),  "select note from public.run_info where run_number=%d ;", runNumber);
		res = PQexec(runInfoDbConn, buffer);

		if(PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			__SS__ << "RUN INFO NOTE SELECT FROM DATABASE TABLE FAILED!!! PQ ERROR: " << PQresultErrorMessage(res) << __E__;
			PQclear(res);
			__SS_THROW__;
		}

		if(PQntuples(res) == 1)
		{
			note = PQgetvalue(res, 0, 0);
			note.append(
				(runStopType == RunInfoVInterface::RunStopType::HALT?"\nRUN end from RUNNING to HALT":
				(runStopType == RunInfoVInterface::RunStopType::STOP?"\nRUN end from RUNNING to STOP":"\nRUN end for an ERROR"))
			);
			__COUTV__(note);
		}
		else
		{
			__SS__ << "RETRIVE NOTE FROM RUN_INFO DATABASE TABLE FAILED!!! PQ ERROR: " << PQresultErrorMessage(res) << __E__;
			PQclear(res);
			__SS_THROW__;
		}

		PQclear(res);

		__COUT__ << "Update run info stop_time and note in the run_info Database table" << __E__;
		snprintf(buffer,
				sizeof(buffer),
				"UPDATE public.run_info SET stop_time=TO_TIMESTAMP(%ld), note='%s' WHERE run_number=%d;", time(NULL), note.c_str(), runNumber);

		res = PQexec(runInfoDbConn, buffer);

		if(PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			__SS__ << "RUN INFO STOP UPDATE INTO DATABASE TABLE FAILED!!! PQ ERROR: " << PQresultErrorMessage(res)
				<< __E__;
			PQclear(res);
			__SS_THROW__;
		}
		PQclear(res);
	}

	//close db connection
	if(PQstatus(runInfoDbConn) == CONNECTION_OK)
	{
		PQfinish(runInfoDbConn);
		__COUT__ << "prototype_run_info DB CONNECTION CLOSED\n" << __E__;
	}

	if(runNumber == (unsigned int)-1)
	{
		__SS__ << "Impossible run number not defined by run info plugin!" << __E__;
		__SS_THROW__;
	}
	
} //end updateRunInfo()


DEFINE_OTS_PROCESSOR(DBRunInfo)
