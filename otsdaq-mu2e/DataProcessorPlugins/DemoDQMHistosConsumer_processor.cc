#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/Macros/ProcessorPluginMacros.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-demo/DataProcessorPlugins/DemoDQMHistosConsumer.h"
#include "otsdaq-demo/DemoRootUtilities/DemoDQMHistos.h"

using namespace ots;

//========================================================================================================================
DemoDQMHistosConsumer::DemoDQMHistosConsumer(
    std::string              supervisorApplicationUID,
    std::string              bufferUID,
    std::string              processorUID,
    const ConfigurationTree& theXDAQContextConfigTree,
    const std::string&       configurationPath)
    : WorkLoop(processorUID)
    , DQMHistosConsumerBase(
          supervisorApplicationUID, bufferUID, processorUID, LowConsumerPriority)
    , Configurable(theXDAQContextConfigTree, configurationPath)
    , saveDQMFile_(theXDAQContextConfigTree.getNode(configurationPath)
                       .getNode("SaveDQMFile")
                       .getValue<bool>())
    , DQMFilePath_(theXDAQContextConfigTree.getNode(configurationPath)
                       .getNode("DQMFilePath")
                       .getValue<std::string>())
    , DQMFilePrefix_(theXDAQContextConfigTree.getNode(configurationPath)
                         .getNode("DQMFileNamePrefix")
                         .getValue<std::string>())
    , dqmHistos_(new DemoDQMHistos())

{
}

//========================================================================================================================
DemoDQMHistosConsumer::~DemoDQMHistosConsumer(void) { closeFile(); }

//========================================================================================================================
void DemoDQMHistosConsumer::startProcessingData(std::string runNumber)
{
	// IMPORTANT
	// The file must be always opened because even the LIVE DQM uses the pointer
	// to it
	DQMHistosBase::openFile(DQMFilePath_ + "/" + DQMFilePrefix_ + "_Run" + runNumber +
	                        ".root");

	dqmHistos_->book(DQMHistosBase::theFile_);
	DataConsumer::startProcessingData(runNumber);
}

//========================================================================================================================
void DemoDQMHistosConsumer::stopProcessingData(void)
{
	DataConsumer::stopProcessingData();
	if(saveDQMFile_)
	{
		save();
	}
	closeFile();
}

//========================================================================================================================
bool DemoDQMHistosConsumer::workLoopThread(toolbox::task::WorkLoop* workLoop)
{
	//__COUT__ << DataProcessor::processorUID_ << " running, because workloop: "
	//<< 	WorkLoop::continueWorkLoop_ << std::endl;
	fastRead();
	return WorkLoop::continueWorkLoop_;
}

//========================================================================================================================
void DemoDQMHistosConsumer::fastRead(void)
{
	//__COUT__ << processorUID_ << " running!" << std::endl;
	// This is making a copy!!!
	if(DataConsumer::read(dataP_, headerP_) < 0)
	{
		usleep(100);
		return;
	}
	//__COUT__ << DataProcessor::processorUID_ << " UID: " <<
	// supervisorApplicationUID_ << std::endl;

	// HW emulator
	//	 Burst Type | Sequence | 8B data
	//__COUT__ << "Size fill: " << dataP_->length() << std::endl;
	dqmHistos_->fill(*dataP_, *headerP_);

	DataConsumer::setReadSubBuffer<std::string, std::map<std::string, std::string>>();
}

//========================================================================================================================
void DemoDQMHistosConsumer::slowRead(void)
{
	//__COUT__ << DataProcessor::processorUID_ << " running!" << std::endl;
	// This is making a copy!!!
	if(DataConsumer::read(data_, header_) < 0)
	{
		usleep(1000);
		return;
	}
	//__COUT__ << DataProcessor::processorUID_ << " UID: " <<
	// supervisorApplicationUID_ << std::endl;  DQMHistos::fill(data_,header_);
}

DEFINE_OTS_PROCESSOR(DemoDQMHistosConsumer)
