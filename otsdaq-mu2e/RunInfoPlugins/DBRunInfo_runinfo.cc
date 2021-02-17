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
		__COUT__ << "Unable to connect to prototype_run_info database!\n" << __E__;
		PQfinish(runInfoDbConn);
	}
	else
	{
		__COUT__ << "Connected to prototype_run_info database!\n" << __E__;
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
				"note");

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

} //end updateRunInfo()


DEFINE_OTS_PROCESSOR(DBRunInfo)
