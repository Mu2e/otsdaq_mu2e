#include "otsdaq-demo/Generators/DataGenReceiver.hh"

//#include "art/Utilities/Exception.h"
#include "artdaq/Application/GeneratorMacros.hh"
#include "cetlib/exception.h"
#include "fhiclcpp/ParameterSet.h"
#include "artdaq-core/Utilities/SimpleLookupPolicy.h"
#include "otsdaq-demo/Overlays/DataGenFragmentWriter.hh"
#include "otsdaq-demo/Overlays/FragmentType.hh"

#include <fstream>
#include <iomanip>
#include <iterator>
#include <iostream>
#include <sys/poll.h>

//========================================================================================================================
ots::DataGenReceiver::DataGenReceiver(fhicl::ParameterSet const & ps)
: WorkLoop                  ("DataGenReceiver")
, DataConsumer              (ps.get<std::string>("SupervisorApplicationUID", "ARTDAQDataManager")
		, ps.get<std::string>("BufferUID", "ARTDAQBuffer")
		, ps.get<std::string>("ProcessorUID", "DataGenReceiver"), HighConsumerPriority)
, CommandableFragmentGenerator(ps)
, rawOutput_                (ps.get<bool>("raw_output_enabled",false))
, rawPath_                  (ps.get<std::string>("raw_output_path", "/tmp"))
, dataport_                 (ps.get<int>("port",6343))
, ip_                       (ps.get<std::string>("ip","127.0.0.1"))
, expectedPacketNumber_     (0)
, sendCommands_             (ps.get<bool>("send_OtsUDP_commands",false))
, fragmentWindow_           (ps.get<double>("fragment_time_window_ms", 1000))
, lastFrag_                 (std::chrono::high_resolution_clock::now())
{
	std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << std::endl;
	mf::LogInfo("DataGenReceiver") << "MY TRIGGER MODE IS: " << ps.get<std::string>("trigger_mode","UNDEFINED!!!");
	std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "registering to buffer!"<< std::endl;
	registerToBuffer();
}

//========================================================================================================================
bool ots::DataGenReceiver::getNext_(artdaq::FragmentPtrs & output)
{
	//__COUT__ << "READING DATA!" << std::endl;
	if (should_stop())
		return false;

	//unsigned long block;
	if(read<std::string, std::map<std::string,std::string>>(buffer_) < 0)
		usleep(10000);
	else
	{
		//std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << name_ << " Buffer: " << buffer << std::endl;
		unsigned long long value;
		memcpy((void *)&value, (void *) buffer_.substr(2).data(),8); //make data counter
		std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << std::hex << value << std::dec << std::endl;
		ProcessData_(output);
	}
	return true;
}

//========================================================================================================================
void ots::DataGenReceiver::ProcessData_(artdaq::FragmentPtrs & frags) {

	std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << std::endl;
	unsigned long long value;
	memcpy((void *)&value, (void *) buffer_.substr(2).data(),8); //make data counter
	std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << std::hex << value << std::dec << std::endl;

	ots::DataGenFragment::Metadata metadata;
	//metadata.port = dataport_;
	//metadata.address = si_data_.sin_addr.s_addr;
	metadata.port = 2000;
	metadata.address = 0xc0aabb11;


	std::ofstream output;
	//std::cout << __COUT_HDR_FL__ << "SAVING FILE??? " << rawOutput_ << std::endl;
	if(rawOutput_) {
		std::string outputPath = rawPath_ + "/DataGenReceiver-"+ ip_ + ":" + std::to_string(dataport_) + ".bin";
		//std::cout << __COUT_HDR_FL__ << "FILE DATA PATH: " << outputPath << std::endl;
		output.open(outputPath, std::ios::out | std::ios::app | std::ios::binary );
	}

	mf::LogInfo("DataGenReceiver") << "Starting DataGenReceiver Packet Processing Loop" << std::endl;
	//	for( auto packet = packetBuffers_.begin(); packet != packetBuffers_.end(); ++packet ) {

	std::size_t initial_payload_size = 0;

	frags.emplace_back( artdaq::Fragment::FragmentBytes(initial_payload_size,
			ev_counter(), fragment_id(),
			ots::detail::FragmentType::DataGen, metadata) );
	ev_counter_inc();

	//******* WE BUILD THE EVENT *************
	// We now have a fragment to contain this event:
	ots::DataGenFragmentWriter thisFrag(*frags.back());
	thisFrag.resize(sizeof(uint8_t) * 64050);
	std::cout << __COUT_HDR_FL__ << "DJN2 after, datasizebytes=" << thisFrag.asksize() <<std::endl << std::endl;

	memcpy(thisFrag.dataBegin(), (&buffer_[2]), sizeof(DataGenFragment::DataBlob));

	//******* DONE BUILDING THE EVENT *************
	if(rawOutput_) {
		output.write(&buffer_[0],sizeof(DataGenFragment::DataBlob));
	}
	//	}
	mf::LogInfo("DataGenReceiver") <<"Done with DataGenReceiver Packet Processing Loop. Deleting PacketBuffers" << std::endl;
	//	packetBuffers_.clear();
	return;
}

//========================================================================================================================
void ots::DataGenReceiver::start() {
 #pragma message "Using default implementation of DataGenReceiver::start_()"
}

//========================================================================================================================
void ots::DataGenReceiver::stop()
{
#pragma message "Using default implementation of DataGenReceiver::stop()"
}

//========================================================================================================================
void ots::DataGenReceiver::stopNoMutex()
{
#pragma message "Using default implementation of DataGenReceiver::stopNoMutex()"
}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(ots::DataGenReceiver)
