#include <iostream>
#include "EpicsInterface.h"
#include "alarm.h"  //Holds strings that we can use to access the alarm status, severity, and parameters
//#include "/mu2e/ups/epics/v3_15_4/Linux64bit+2.6-2.12-e10/include/alarm.h"
//#include "alarmString.h"
#include "cadef.h"  //EPICS Channel Access:
// http://www.aps.anl.gov/epics/base/R3-14/12-docs/CAref.html
// Example compile options:
// Compiling:
// Setup epics (See redmine wiki)
// g++ -std=c++11  EpicsCAMonitor.cpp EpicsCAMessage.cpp EpicsWebClient.cpp
// SocketUDP.cpp SocketTCP.cpp -L$EPICS_BASE/lib/linux-x86_64/
// -Wl,-rpath,$EPICS_BASE/lib/linux-x86_64 -lca -lCom -I$EPICS_BASE//include
// -I$EPICS_BASE//include/os/Linux -I$EPICS_BASE/include/compiler/gcc -o
// EpicsWebClient

#define DEBUG false
#define PV_FILE_NAME \
	std::string(getenv("SERVICE_DATA_PATH")) + "/SlowControlsDashboardData/pv_list.dat";

using namespace ots;

EpicsInterface::EpicsInterface()
{
	// this allows for handlers to happen "asynchronously"
	SEVCHK(ca_context_create(ca_enable_preemptive_callback),
	       "EpicsInterface::EpicsInterface() : "
	       "ca_enable_preemptive_callback_init()");
}

EpicsInterface::~EpicsInterface() { destroy(); }

void EpicsInterface::destroy()
{
	// std::cout << "mapOfPVInfo_.size() = " << mapOfPVInfo_.size() << std::endl;
	for(auto it = mapOfPVInfo_.begin(); it != mapOfPVInfo_.end(); it++)
	{
		cancelSubscriptionToChannel(it->first);
		destroyChannel(it->first);
		delete(it->second->parameterPtr);
		delete(it->second);
		mapOfPVInfo_.erase(it);
	}

	// std::cout << "mapOfPVInfo_.size() = " << mapOfPVInfo_.size() << std::endl;
	SEVCHK(ca_poll(), "EpicsInterface::destroy() : ca_poll");
	return;
}

void EpicsInterface::initialize()
{
	destroy();
	loadListOfPVs();

	return;
}
std::string EpicsInterface::getList(std::string format)
{
	std::string pvList;

	if(format == "JSON")
	{
		// pvList = "{\"PVList\" : [";
		pvList = "[";
		for(auto it = mapOfPVInfo_.begin(); it != mapOfPVInfo_.end(); it++)
			pvList += "\"" + it->first + "\", ";

		pvList.resize(pvList.size() - 2);
		pvList += "]";  //}";
		return pvList;
	}
	return pvList;
}

void EpicsInterface::subscribe(std::string pvName)
{
	if(!checkIfPVExists(pvName))
	{
		std::cout << pvName << " doesn't exist!" << std::endl;
		return;
	}
	createChannel(pvName);
	sleep(1);
	subscribeToChannel(pvName, mapOfPVInfo_.find(pvName)->second->channelType);
	SEVCHK(ca_poll(), "EpicsInterface::subscribe() : ca_poll");

	return;
}

//{"PVList" : ["Mu2e_BeamData_IOC/CurrentTime"]}
void EpicsInterface::subscribeJSON(std::string pvList)
{
	// if(DEBUG){std::cout << pvList << std::endl;;}

	std::string JSON = "{\"PVList\" :";
	std::string pvName;
	if(pvList.find(JSON) != std::string::npos)
	{
		pvList = pvList.substr(pvList.find(JSON) + JSON.length(), std::string::npos);
		do
		{
			pvList = pvList.substr(pvList.find("\"") + 1,
			                       std::string::npos);     // eliminate up to the next "
			pvName = pvList.substr(0, pvList.find("\""));  //
			// if(DEBUG){std::cout << "Read PV Name:  " << pvName << std::endl;}
			pvList = pvList.substr(pvList.find("\"") + 1, std::string::npos);
			// if(DEBUG){std::cout << "pvList : " << pvList << std::endl;}

			if(checkIfPVExists(pvName))
			{
				createChannel(pvName);
				subscribeToChannel(pvName,
				                   mapOfPVInfo_.find(pvName)->second->channelType);
				SEVCHK(ca_poll(), "EpicsInterface::subscribeJSON : ca_poll");
			}
			else if(DEBUG)
			{
				std::cout << pvName << " not found in file! Not subscribing!"
				          << std::endl;
			}

		} while(pvList.find(",") != std::string::npos);
	}

	return;
}

void EpicsInterface::unsubscribe(std::string pvName)
{
	if(!checkIfPVExists(pvName))
	{
		std::cout << pvName << " doesn't exist!" << std::endl;
		return;
	}

	cancelSubscriptionToChannel(pvName);
	return;
}
//------------------------------------------------------------------------------------------------------------
//--------------------------------------PRIVATE
// FUNCTION--------------------------------------
//------------------------------------------------------------------------------------------------------------
void EpicsInterface::eventCallback(struct event_handler_args eha)
{
	chid chid = eha.chid;
	if(eha.status == ECA_NORMAL)
	{
		int                  i;
		union db_access_val* pBuf = (union db_access_val*)eha.dbr;
		if(DEBUG)
		{
			printf("channel %s: ", ca_name(eha.chid));
		}

		switch(eha.type)
		{
		case DBR_CTRL_CHAR:
			if(true)
			{
				std::cout << "Response Type: DBR_CTRL_CHAR" << std::endl;
			}
			((EpicsInterface*)eha.usr)
			    ->writePVControlValueToRecord(
			        ca_name(eha.chid),
			        ((struct dbr_ctrl_char*)
			             eha.dbr));  // write the PV's control values to records
			break;
		case DBF_DOUBLE:
			if(DEBUG)
			{
				std::cout << "Response Type: DBR_DOUBLE" << std::endl;
			}
			((EpicsInterface*)eha.usr)
			    ->writePVValueToRecord(
			        ca_name(eha.chid),
			        std::to_string(
			            *((double*)eha.dbr)));  // write the PV's value to records
			break;
		case DBR_STS_STRING:
			if(DEBUG)
			{
				std::cout << "Response Type: DBR_STS_STRING" << std::endl;
			}
			((EpicsInterface*)eha.usr)
			    ->writePVAlertToQueue(ca_name(eha.chid),
			                          epicsAlarmConditionStrings[pBuf->sstrval.status],
			                          epicsAlarmSeverityStrings[pBuf->sstrval.severity]);
			/*if(DEBUG)
{
printf("current %s:\n", eha.count > 1?"values":"value");
for (i = 0; i < eha.count; i++)
{
printf("%s\t", *(&(pBuf->sstrval.value) + i));
if ((i+1)%6 == 0) printf("\n");
}
printf("\n");
}*/
			break;
		case DBR_STS_SHORT:
			if(DEBUG)
			{
				std::cout << "Response Type: DBR_STS_SHORT" << std::endl;
			}
			((EpicsInterface*)eha.usr)
			    ->writePVAlertToQueue(ca_name(eha.chid),
			                          epicsAlarmConditionStrings[pBuf->sshrtval.status],
			                          epicsAlarmSeverityStrings[pBuf->sshrtval.severity]);
			/*if(DEBUG)
{
printf("current %s:\n", eha.count > 1?"values":"value");
for (i = 0; i < eha.count; i++){
printf("%-10d", *(&(pBuf->sshrtval.value) + i));
if ((i+1)%8 == 0) printf("\n");
}
printf("\n");
}*/
			break;
		case DBR_STS_FLOAT:
			if(DEBUG)
			{
				std::cout << "Response Type: DBR_STS_FLOAT" << std::endl;
			}
			((EpicsInterface*)eha.usr)
			    ->writePVAlertToQueue(ca_name(eha.chid),
			                          epicsAlarmConditionStrings[pBuf->sfltval.status],
			                          epicsAlarmSeverityStrings[pBuf->sfltval.severity]);
			/*if(DEBUG)
{
printf("current %s:\n", eha.count > 1?"values":"value");
for (i = 0; i < eha.count; i++){
printf("-10.4f", *(&(pBuf->sfltval.value) + i));
if ((i+1)%8 == 0) printf("\n");
}
printf("\n");
}*/
			break;
		case DBR_STS_ENUM:
			if(DEBUG)
			{
				std::cout << "Response Type: DBR_STS_ENUM" << std::endl;
			}
			((EpicsInterface*)eha.usr)
			    ->writePVAlertToQueue(ca_name(eha.chid),
			                          epicsAlarmConditionStrings[pBuf->senmval.status],
			                          epicsAlarmSeverityStrings[pBuf->senmval.severity]);
			/*if(DEBUG)
{
printf("current %s:\n", eha.count > 1?"values":"value");
for (i = 0; i < eha.count; i++){
printf("%d ", *(&(pBuf->senmval.value) + i));
}
printf("\n");
}*/
			break;
		case DBR_STS_CHAR:
			if(DEBUG)
			{
				std::cout << "Response Type: DBR_STS_CHAR" << std::endl;
			}
			((EpicsInterface*)eha.usr)
			    ->writePVAlertToQueue(ca_name(eha.chid),
			                          epicsAlarmConditionStrings[pBuf->schrval.status],
			                          epicsAlarmSeverityStrings[pBuf->schrval.severity]);
			/*if(DEBUG)
{
printf("current %s:\n", eha.count > 1?"values":"value");
for (i = 0; i < eha.count; i++){
printf("%-5", *(&(pBuf->schrval.value) + i));
if ((i+1)%15 == 0) printf("\n");
}
printf("\n");
}*/
			break;
		case DBR_STS_LONG:
			if(DEBUG)
			{
				std::cout << "Response Type: DBR_STS_LONG" << std::endl;
			}
			((EpicsInterface*)eha.usr)
			    ->writePVAlertToQueue(ca_name(eha.chid),
			                          epicsAlarmConditionStrings[pBuf->slngval.status],
			                          epicsAlarmSeverityStrings[pBuf->slngval.severity]);
			/*if(DEBUG)
{
printf("current %s:\n", eha.count > 1?"values":"value");
for (i = 0; i < eha.count; i++){
printf("%-15d", *(&(pBuf->slngval.value) + i));
if((i+1)%5 == 0) printf("\n");
}
printf("\n");
}*/
			break;
		case DBR_STS_DOUBLE:
			if(DEBUG)
			{
				std::cout << "Response Type: DBR_STS_DOUBLE" << std::endl;
			}
			((EpicsInterface*)eha.usr)
			    ->writePVAlertToQueue(ca_name(eha.chid),
			                          epicsAlarmConditionStrings[pBuf->sdblval.status],
			                          epicsAlarmSeverityStrings[pBuf->sdblval.severity]);
			/*if(DEBUG)
{
printf("current %s:\n", eha.count > 1?"values":"value");
for (i = 0; i < eha.count; i++){
printf("%-15.4f", *(&(pBuf->sdblval.value) + i));
}
printf("\n");
}*/
			break;
		default:
			if(ca_name(eha.chid))
			{
				if(DEBUG)
				{
					std::cout << " EpicsInterface::eventCallback: PV Name = "
					          << ca_name(eha.chid) << std::endl;
					std::cout << (char*)eha.dbr << std::endl;
				}
				((EpicsInterface*)eha.usr)
				    ->writePVValueToRecord(
				        ca_name(eha.chid),
				        (char*)eha.dbr);  // write the PV's value to records
			}

			break;
		}
		/* if get operation failed, print channel name and message */
	}
	else
		printf("channel %s: get operation failed\n", ca_name(eha.chid));

	return;
}

void EpicsInterface::staticChannelCallbackHandler(struct connection_handler_args cha)
{
	std::cout << "webClientChannelCallbackHandler" << std::endl;

	((PVHandlerParameters*)ca_puser(cha.chid))->webClient->channelCallbackHandler(cha);
	return;
}

void EpicsInterface::channelCallbackHandler(struct connection_handler_args& cha)
{
	std::string pv = ((PVHandlerParameters*)ca_puser(cha.chid))->pvName;
	if(cha.op == CA_OP_CONN_UP)
	{
		std::cout << pv << cha.chid << " connected! " << std::endl;

		mapOfPVInfo_.find(pv)->second->channelType = ca_field_type(cha.chid);
		readPVRecord(pv);

		/*status_ =
ca_array_get_callback(dbf_type_to_DBR_STS(mapOfPVInfo_.find(pv)->second->channelType),
ca_element_count(cha.chid), cha.chid, eventCallback, this); SEVCHK(status_,
"ca_array_get_callback");*/
	}
	else
		std::cout << pv << " disconnected!" << std::endl;

	return;
}

bool EpicsInterface::checkIfPVExists(std::string pvName)
{
	if(DEBUG)
	{
		std::cout << "EpicsInterface::checkIfPVExists(): PV Info Map Length is "
		          << mapOfPVInfo_.size() << std::endl;
	}

	if(mapOfPVInfo_.find(pvName) != mapOfPVInfo_.end())
		return true;

	return false;
}

void EpicsInterface::loadListOfPVs()
{
	// Initialize Channel Access
	status_ = ca_task_initialize();
	SEVCHK(status_, "EpicsInterface::loadListOfPVs() : Unable to initialize");
	if(status_ != ECA_NORMAL)
		exit(-1);

	// read file
	// for each line in file
	std::string pv_list_file = PV_FILE_NAME;
	std::cout << pv_list_file << std::endl;
	std::ifstream infile(pv_list_file);
	std::cout << "Reading file" << std::endl;

	// make map of pvname -> PVInfo
	for(std::string line; getline(infile, line);)
	{
		std::cout << line << std::endl;
		mapOfPVInfo_[line] = new PVInfo(DBR_STRING);
	}

	// subscribe for each pv
	for(auto pv : mapOfPVInfo_)
	{
		subscribe(pv.first);
	}

	// channels are subscribed to by here.

	// get parameters (e.g. HIHI("upper alarm") HI("upper warning") LOLO("lower
	// alarm")) for each pv
	for(auto pv : mapOfPVInfo_)
	{
		getControlValues(pv.first);
	}

	std::cout << "Finished reading file and subscribing to pvs!" << std::endl;
	return;
}

void EpicsInterface::getControlValues(std::string pvName)
{
	if(true)
	{
		std::cout << "EpicsInterface::getControlValues(" << pvName << ")" << std::endl;
	}
	if(!checkIfPVExists(pvName))
	{
		std::cout << pvName << " doesn't exist!" << std::endl;
		return;
	}

	SEVCHK(ca_array_get_callback(DBR_CTRL_CHAR,
	                             0,
	                             mapOfPVInfo_.find(pvName)->second->channelID,
	                             eventCallback,
	                             this),
	       "ca_array_get_callback");
	SEVCHK(ca_poll(), "EpicsInterface::getControlValues() : ca_poll");
	return;
}

void EpicsInterface::createChannel(std::string pvName)
{
	if(!checkIfPVExists(pvName))
	{
		std::cout << pvName << " doesn't exist!" << std::endl;
		return;
	}
	std::cout << "Trying to create channel to " << pvName << ":"
	          << mapOfPVInfo_.find(pvName)->second->channelID << std::endl;

	if(mapOfPVInfo_.find(pvName)->second != NULL)  // Check to see if the pvName
	                                               // maps to a null pointer so we
	                                               // don't have any errors
		if(mapOfPVInfo_.find(pvName)->second->channelID !=
		   NULL)  // channel might exist, subscription doesn't so create a
		          // subscription
		{
			// if state of channel is connected then done, use it
			if(ca_state(mapOfPVInfo_.find(pvName)->second->channelID) == cs_conn)
			{
				if(DEBUG)
				{
					std::cout << "Channel to " << pvName << " already exists!"
					          << std::endl;
				}
				return;
			}
			if(DEBUG)
			{
				std::cout << "Channel to " << pvName
				          << " exists, but is not connected! Destroying current channel."
				          << std::endl;
			}
			destroyChannel(pvName);
		}

	// create pvs handler
	if(mapOfPVInfo_.find(pvName)->second->parameterPtr == NULL)
	{
		mapOfPVInfo_.find(pvName)->second->parameterPtr =
		    new PVHandlerParameters(pvName, this);
	}

	// at this point, make a new channel

	SEVCHK(ca_create_channel(pvName.c_str(),
	                         staticChannelCallbackHandler,
	                         mapOfPVInfo_.find(pvName)->second->parameterPtr,
	                         0,
	                         &(mapOfPVInfo_.find(pvName)->second->channelID)),
	       "EpicsInterface::createChannel() : ca_create_channel");
	std::cout << "channelID: " << pvName << mapOfPVInfo_.find(pvName)->second->channelID
	          << std::endl;
	SEVCHK(ca_poll(), "EpicsInterface::createChannel() : ca_poll");

	return;
}
void EpicsInterface::destroyChannel(std::string pvName)
{
	if(mapOfPVInfo_.find(pvName)->second != NULL)
	{
		if(mapOfPVInfo_.find(pvName)->second->channelID != NULL)
		{
			status_ = ca_clear_channel(mapOfPVInfo_.find(pvName)->second->channelID);
			SEVCHK(status_, "EpicsInterface::destroyChannel() : ca_clear_channel");
			if(status_ == ECA_NORMAL)
			{
				mapOfPVInfo_.find(pvName)->second->channelID = NULL;
				if(DEBUG)
				{
					std::cout << "Killed channel to " << pvName << std::endl;
				}
			}
			SEVCHK(ca_poll(), "EpicsInterface::destroyChannel() : ca_poll");
		}
		else
		{
			if(DEBUG)
			{
				std::cout << "No channel to " << pvName << " exists" << std::endl;
			}
		}
	}
	return;
}

void EpicsInterface::subscribeToChannel(std::string pvName, chtype subscriptionType)
{
	if(!checkIfPVExists(pvName))
	{
		std::cout << pvName << " doesn't exist!" << std::endl;
		return;
	}
	if(DEBUG)
	{
		std::cout << "Trying to subscribe to " << pvName << ":"
		          << mapOfPVInfo_.find(pvName)->second->channelID << std::endl;
	}

	if(mapOfPVInfo_.find(pvName)->second != NULL)  // Check to see if the pvName
	                                               // maps to a null pointer so we
	                                               // don't have any errors
	{
		if(mapOfPVInfo_.find(pvName)->second->eventID !=
		   NULL)  // subscription already exists
		{
			if(DEBUG)
			{
				std::cout << "Already subscribed to " << pvName << "!" << std::endl;
			}
			// FIXME No way to check if the event ID is valid
			// Just cancel the subscription if it already exists?
		}
	}

	//	int i=0;
	//	while(ca_state(mapOfPVInfo_.find(pvName)->second->channelID) == cs_conn
	//&& i<2) 		Sleep(1); 	if(i==2)
	//		{__SS__;throw std::runtime_error(ss.str() + "Channel failed for " +
	// pvName);}

	SEVCHK(ca_create_subscription(
	           dbf_type_to_DBR(mapOfPVInfo_.find(pvName)->second->channelType),
	           1,
	           mapOfPVInfo_.find(pvName)->second->channelID,
	           DBE_VALUE | DBE_ALARM | DBE_PROPERTY,
	           eventCallback,
	           this,
	           &(mapOfPVInfo_.find(pvName)->second->eventID)),
	       "EpicsInterface::subscribeToChannel() : ca_create_subscription");
	if(DEBUG)
	{
		std::cout << "EpicsInterface::subscribeToChannel: Created Subscription to "
		          << mapOfPVInfo_.find(pvName)->first << "!\n"
		          << std::endl;
	}
	SEVCHK(ca_poll(), "EpicsInterface::subscribeToChannel() : ca_poll");

	return;
}

void EpicsInterface::cancelSubscriptionToChannel(std::string pvName)
{
	if(mapOfPVInfo_.find(pvName)->second != NULL)
		if(mapOfPVInfo_.find(pvName)->second->eventID != NULL)
		{
			status_ = ca_clear_subscription(mapOfPVInfo_.find(pvName)->second->eventID);
			SEVCHK(status_,
			       "EpicsInterface::cancelSubscriptionToChannel() : "
			       "ca_clear_subscription");
			if(status_ == ECA_NORMAL)
			{
				mapOfPVInfo_.find(pvName)->second->eventID = NULL;
				if(DEBUG)
				{
					std::cout << "Killed subscription to " << pvName << std::endl;
				}
			}
			SEVCHK(ca_poll(), "EpicsInterface::cancelSubscriptionToChannel() : ca_poll");
		}
		else
		{
			if(DEBUG)
			{
				std::cout << pvName << "does not have a subscription!" << std::endl;
			}
		}
	else
	{
		// std::cout << pvName << "does not have a subscription!" << std::endl;
	}
	//  SEVCHK(ca_flush_io(),"ca_flush_io");
	return;
}

void EpicsInterface::readValueFromPV(std::string pvName)
{
	// SEVCHK(ca_get(DBR_String, 0, mapOfPVInfo_.find(pvName)->second->channelID,
	// &(mapOfPVInfo_.find(pvName)->second->pvValue), eventCallback,
	// &(mapOfPVInfo_.find(pvName)->second->callbackPtr)), "ca_get");

	return;
}
void EpicsInterface::writePVControlValueToRecord(std::string           pvName,
                                                 struct dbr_ctrl_char* pdata)
{
	if(DEBUG)
	{
		std::cout << "Reading Control Values from " << pvName << "!" << std::endl;
	}

	if(!checkIfPVExists(pvName))
	{
		std::cout << pvName << " doesn't exist!" << std::endl;
		return;
	}
	mapOfPVInfo_.find(pvName)->second->settings = *pdata;

	if(true)
	{
		std::cout << "status: " << pdata->status << std::endl;
		std::cout << "severity: " << pdata->severity << std::endl;
		std::cout << "units: " << pdata->units << std::endl;
		std::cout << "upper disp limit: " << pdata->upper_disp_limit << std::endl;
		std::cout << "lower disp limit: " << pdata->lower_disp_limit << std::endl;
		std::cout << "upper alarm limit: " << pdata->upper_alarm_limit << std::endl;
		std::cout << "upper warning limit: " << pdata->upper_warning_limit << std::endl;
		std::cout << "lower warning limit: " << pdata->lower_warning_limit << std::endl;
		std::cout << "lower alarm limit: " << pdata->lower_alarm_limit << std::endl;
		std::cout << "upper control limit: " << pdata->upper_ctrl_limit << std::endl;
		std::cout << "lower control limit: " << pdata->lower_ctrl_limit << std::endl;
		std::cout << "RISC_pad: " << pdata->RISC_pad << std::endl;
		std::cout << "Value: " << pdata->value << std::endl;
	}
	return;
}
// Enforces the circular buffer
void EpicsInterface::writePVValueToRecord(std::string pvName, std::string pdata)
{
	std::pair<time_t, std::string> currentRecord(time(0), pdata);

	if(!checkIfPVExists(pvName))
	{
		std::cout << pvName << " doesn't exist!" << std::endl;
		return;
	}
	// std::cout << pdata << std::endl;

	PVInfo* pvInfo = mapOfPVInfo_.find(pvName)->second;

	if(pvInfo->mostRecentBufferIndex != pvInfo->dataCache.size() - 1 &&
	   pvInfo->mostRecentBufferIndex != (unsigned int)(-1))
	{
		if(pvInfo->dataCache[pvInfo->mostRecentBufferIndex].first == currentRecord.first)
		{
			pvInfo->valueChange = true;  // false;
		}
		else
		{
			pvInfo->valueChange = true;
		}

		++pvInfo->mostRecentBufferIndex;
		pvInfo->dataCache[pvInfo->mostRecentBufferIndex] = currentRecord;
	}
	else
	{
		pvInfo->dataCache[0]          = currentRecord;
		pvInfo->mostRecentBufferIndex = 0;
	}
	// debugConsole(pvName);
	// debugConsole(pvName);

	return;
}
void EpicsInterface::writePVAlertToQueue(std::string pvName,
                                         const char* status,
                                         const char* severity)
{
	if(!checkIfPVExists(pvName))
	{
		std::cout << pvName << " doesn't exist!" << std::endl;
		return;
	}
	PVAlerts alert(time(0), status, severity);
	mapOfPVInfo_.find(pvName)->second->alerts.push(alert);

	// debugConsole(pvName);

	return;
}
void EpicsInterface::readPVRecord(std::string pvName)
{
	status_ = ca_array_get_callback(
	    dbf_type_to_DBR_STS(mapOfPVInfo_.find(pvName)->second->channelType),
	    ca_element_count(mapOfPVInfo_.find(pvName)->second->channelID),
	    mapOfPVInfo_.find(pvName)->second->channelID,
	    eventCallback,
	    this);
	SEVCHK(status_, "EpicsInterface::readPVRecord(): ca_array_get_callback");
	return;
}

void EpicsInterface::debugConsole(std::string pvName)
{
	std::cout << "==============================================================="
	             "==============="
	          << std::endl;
	for(unsigned int it = 0; it < mapOfPVInfo_.find(pvName)->second->dataCache.size() - 1;
	    it++)
	{
		if(it == mapOfPVInfo_.find(pvName)->second->mostRecentBufferIndex)
		{
			std::cout << "-----------------------------------------------------------"
			             "----------"
			          << std::endl;
		}
		std::cout << "Iteration: " << it << " | "
		          << mapOfPVInfo_.find(pvName)->second->mostRecentBufferIndex << " | "
		          << mapOfPVInfo_.find(pvName)->second->dataCache[it].second << std::endl;
		if(it == mapOfPVInfo_.find(pvName)->second->mostRecentBufferIndex)
		{
			std::cout << "-----------------------------------------------------------"
			             "----------"
			          << std::endl;
		}
	}
	std::cout << "==============================================================="
	             "==============="
	          << std::endl;
	std::cout << "Status:     "
	          << " | " << mapOfPVInfo_.find(pvName)->second->alerts.size() << " | "
	          << mapOfPVInfo_.find(pvName)->second->alerts.front().status << std::endl;
	std::cout << "Severity:   "
	          << " | " << mapOfPVInfo_.find(pvName)->second->alerts.size() << " | "
	          << mapOfPVInfo_.find(pvName)->second->alerts.front().severity << std::endl;
	std::cout << "==============================================================="
	             "==============="
	          << std::endl;

	return;
}
void EpicsInterface::popQueue(std::string pvName)
{
	if(DEBUG)
	{
		std::cout << "EpicsInterface::popQueue() " << std::endl;
	}
	mapOfPVInfo_.find(pvName)->second->alerts.pop();

	if(mapOfPVInfo_.find(pvName)->second->alerts.empty())
	{
		readPVRecord(pvName);
		SEVCHK(ca_poll(), "EpicsInterface::popQueue() : ca_poll");
	}
	return;
}

std::array<std::string, 4> EpicsInterface::getCurrentValue(std::string pvName)
{
	std::cout << "void EpicsInterface::getCurrentValue() reached" << std::endl;

	if(mapOfPVInfo_.find(pvName) != mapOfPVInfo_.end())
	{
		PVInfo*     pv = mapOfPVInfo_.find(pvName)->second;
		std::string time, value, status, severity;

		int index = pv->mostRecentBufferIndex;

		std::cout << pv << index << std::endl;

		if(0 <= index && index < pv->circularBufferSize)
		{
			time     = std::to_string(pv->dataCache[index].first);
			value    = pv->dataCache[index].second;
			status   = pv->alerts.front().status;
			severity = pv->alerts.front().severity;
		}
		else if(index == -1)
		{
			time     = "N/a";
			value    = "N/a";
			status   = "DC";
			severity = "DC";
		}
		else
		{
			time     = "N/a";
			value    = "N/a";
			status   = "UDF";
			severity = "INVALID";
		}
		// Time, Value, Status, Severity

		std::cout << "Index:    " << index << std::endl;
		std::cout << "Time:     " << time << std::endl;
		std::cout << "Value:    " << value << std::endl;
		std::cout << "Status:   " << status << std::endl;
		std::cout << "Severity: " << severity << std::endl;

		if(pv->valueChange)
		{
			pv->valueChange = false;
		}
		else
		{
			std::cout << pvName << " has no change" << std::endl;
			time     = "NO_CHANGE";
			value    = "";
			status   = "";
			severity = "";
		}

		std::array<std::string, 4> currentValues = {time, value, status, severity};

		return currentValues;
	}
	else
	{
		std::cout << pvName << " was not found!" << std::endl;
		std::cout << "Trying to resubscribe to " << pvName << std::endl;
		// subscribe(pvName);
	}
	std::array<std::string, 4> currentValues = {"PV Not Found", "NF", "N/a", "N/a"};
	// std::string currentValues [4] = {"N/a", "N/a", "N/a", "N/a"};
	return currentValues;
}

std::array<std::string, 9> EpicsInterface::getSettings(std::string pvName)
{
	std::cout << "void EpicsInterface::getPVSettings() reached" << std::endl;

	if(mapOfPVInfo_.find(pvName) != mapOfPVInfo_.end())
	{
		if(mapOfPVInfo_.find(pvName)->second != NULL)  // Check to see if the pvName
		                                               // maps to a null pointer so
		                                               // we don't have any errors
			if(mapOfPVInfo_.find(pvName)->second->channelID !=
			   NULL)  // channel might exist, subscription doesn't so create a
			          // subscription
			{
				dbr_ctrl_char* set = &mapOfPVInfo_.find(pvName)->second->settings;
				std::string units, upperDisplayLimit, lowerDisplayLimit, upperAlarmLimit,
				    upperWarningLimit, lowerWarningLimit, lowerAlarmLimit,
				    upperControlLimit, lowerControlLimit;
				// sprintf(&units[0],"%d",set->units);
				//			    	units = set->units;
				//					sprintf(&upperDisplayLimit[0],"%u",set->upper_disp_limit);
				//					sprintf(&lowerDisplayLimit[0],"%u",set->lower_disp_limit
				//);
				//					sprintf(&lowerDisplayLimit[0],"%u",set->lower_disp_limit
				//); 					sprintf(
				//&upperAlarmLimit[0],"%u",set->upper_alarm_limit  );
				//					sprintf(&upperWarningLimit[0],"%u",set->upper_warning_limit);
				//					sprintf(&lowerWarningLimit[0],"%u",set->lower_warning_limit);
				//					sprintf(
				//&lowerAlarmLimit[0],"%u",set->lower_alarm_limit  );
				//					sprintf(&upperControlLimit[0],"%u",set->upper_ctrl_limit);
				//					sprintf(&lowerControlLimit[0],"%u",set->lower_ctrl_limit);

				//					std::string units             =
				// set->units;
				//					std::string upperDisplayLimit
				//(reinterpret_cast<char*>(set->upper_disp_limit   ));
				//					std::string lowerDisplayLimit
				//(reinterpret_cast<char*>(set->lower_disp_limit   ));
				//					std::string upperAlarmLimit
				//(reinterpret_cast<char*>(set->upper_alarm_limit  ));
				//					std::string upperWarningLimit
				//(reinterpret_cast<char*>(set->upper_warning_limit));
				//					std::string lowerWarningLimit
				//(reinterpret_cast<char*>(set->lower_warning_limit));
				//					std::string lowerAlarmLimit
				//(reinterpret_cast<char*>(set->lower_alarm_limit  ));
				//					std::string upperControlLimit
				//(reinterpret_cast<char*>(set->upper_ctrl_limit   ));
				//					std::string lowerControlLimit
				//(reinterpret_cast<char*>(set->lower_ctrl_limit   ));
				if(DEBUG)
				{
					std::cout << "Units              :    " << units << std::endl;
					std::cout << "Upper Display Limit:    " << upperDisplayLimit
					          << std::endl;
					std::cout << "Lower Display Limit:    " << lowerDisplayLimit
					          << std::endl;
					std::cout << "Upper Alarm Limit  :    " << upperAlarmLimit
					          << std::endl;
					std::cout << "Upper Warning Limit:    " << upperWarningLimit
					          << std::endl;
					std::cout << "Lower Warning Limit:    " << lowerWarningLimit
					          << std::endl;
					std::cout << "Lower Alarm Limit  :    " << lowerAlarmLimit
					          << std::endl;
					std::cout << "Upper Control Limit:    " << upperControlLimit
					          << std::endl;
					std::cout << "Lower Control Limit:    " << lowerControlLimit
					          << std::endl;
				}
			}

		std::array<std::string, 9> s = {
		    "DC'd", "DC'd", "DC'd", "DC'd", "DC'd", "DC'd", "DC'd", "DC'd", "DC'd"};

		// std::array<std::string, 9> s = {units, upperDisplayLimit,
		// lowerDisplayLimit, upperAlarmLimit, upperWarningLimit, lowerWarningLimit,
		// lowerAlarmLimit, upperControlLimit, lowerControlLimit};

		return s;
	}
	else
	{
		std::cout << pvName << " was not found!" << std::endl;
		std::cout << "Trying to resubscribe to " << pvName << std::endl;
		subscribe(pvName);
	}
	std::array<std::string, 9> s = {
	    "DC'd", "DC'd", "DC'd", "DC'd", "DC'd", "DC'd", "DC'd", "DC'd", "DC'd"};
	return s;
}

DEFINE_OTS_MONITOR(EpicsInterface)
