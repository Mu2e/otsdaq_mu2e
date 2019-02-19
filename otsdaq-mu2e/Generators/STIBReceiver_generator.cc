#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-demo/Generators/STIBReceiver.hh"

//#include "art/Utilities/Exception.h"
#include "artdaq-core/Utilities/SimpleLookupPolicy.h"
#include "artdaq/Application/GeneratorMacros.hh"
#include "cetlib/exception.h"
#include "fhiclcpp/ParameterSet.h"
#include "otsdaq-demo/Overlays/FragmentType.hh"
#include "otsdaq-demo/Overlays/STIBFragmentWriter.hh"

#include <sys/poll.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>

ots::STIBReceiver::STIBReceiver(fhicl::ParameterSet const& ps) : UDPReceiver(ps) {}

void ots::STIBReceiver::ProcessData_(artdaq::FragmentPtrs& frags)
{
	ots::STIBFragment::Metadata metadata;
	metadata.port    = dataport_;
	metadata.address = si_data_.sin_addr.s_addr;

	bool                  inData         = false;
	uint64_t              bunch_counter  = 0;
	uint64_t              triggerCounter = 0;
	uint8_t               triggerInput0  = 0;
	uint8_t               triggerInput1  = 0;
	std::vector<uint32_t> data;

	std::ofstream output;
	if(rawOutput_)
	{
		std::string outputPath =
		    rawPath_ + "/STIBReceiver-" + ip_ + ":" + std::to_string(dataport_) + ".bin";
		output.open(outputPath, std::ios::out | std::ios::app | std::ios::binary);
	}

	std::cout << __COUT_HDR_FL__ << "Starting STIBReceiver Packet Processing Loop"
	          << std::endl;
	for(auto packet = packetBuffers_.begin(); packet != packetBuffers_.end(); ++packet)
	{
		for(size_t word = 0; word < (*packet)->size(); word += 4)
		{
			uint8_t byte3 = (*packet)->at(word);
			uint8_t byte2 = (*packet)->at(word + 1);
			uint8_t byte1 = (*packet)->at(word + 2);
			uint8_t byte0 = (*packet)->at(word + 3);

			if(rawOutput_)
			{
				output.write((char*)&(byte3), sizeof(uint32_t));
			}

			if((byte0 & 0x8) == 0)
			{
				if(inData)  // We've reached the end of a data block, write it out
				{
					std::size_t initial_payload_size = 0;

					frags.emplace_back(
					    artdaq::Fragment::FragmentBytes(initial_payload_size,
					                                    ev_counter(),
					                                    fragment_id(),
					                                    ots::detail::FragmentType::STIB,
					                                    metadata));
					// We now have a fragment to contain this event:
					ev_counter_inc();
					ots::STIBFragmentWriter thisFrag(*frags.back());
					thisFrag.resize(4 * data.size());  // 4 bytes per 32-bit word
					thisFrag.set_hdr_bunch_counter(bunch_counter);
					thisFrag.set_hdr_trigger_counter(triggerCounter);
					thisFrag.set_hdr_trigger_input0(triggerInput0);
					thisFrag.set_hdr_trigger_input1(triggerInput1);
					inData         = false;
					bunch_counter  = 0;
					triggerCounter = 0;
					triggerInput0  = 0;
					triggerInput1  = 0;
					for(size_t ii = 0; ii < data.size(); ++ii)
					{
						*(thisFrag.dataBegin() + ii) = data[ii];
					}
					data.clear();
				}

				if((byte0 & 0x10) == 0x10)
				{  // Bunch Counter Low
					bunch_counter = (bunch_counter & 0xFFFFFF000000) + byte1 +
					                (byte2 << 8) + (byte3 << 16);
				}
				else if((byte0 & 0x20) == 0x20)
				{  // Bunch Counter High
					bunch_counter = (bunch_counter & 0xFFFFFF) + ((uint64_t)byte1 << 24) +
					                ((uint64_t)byte2 << 32) + ((uint64_t)byte3 << 40);
				}
				else if((byte0 & 0xb0) == 0xb0)
				{  // Trigger Counter High
					triggerCounter = (triggerCounter & 0xFFFF) + ((uint64_t)byte1 << 16) +
					                 ((uint64_t)byte2 << 24) + ((uint64_t)byte3 << 32);
				}
				else if((byte0 & 0xa0) == 0xa0)
				{  // Trigger Counter Low
					bunch_counter = (bunch_counter & 0xFFFFFFFFFF00) + byte1;
					triggerCounter =
					    (triggerCounter & 0xFFFFFF0000) + byte2 + (byte3 << 8);
				}
				else if(((byte0 & 0xc0) == 0xc0) ||
				        ((byte0 & 0xd0) == 0xd0) ||  // Trigger Input
				        ((byte0 & 0xe0) == 0xe0) || ((byte0 & 0xf0) == 0xf0))
				{
					bunch_counter = (bunch_counter & 0xFFFFFFFFFF00) + byte1;
					triggerInput0 = byte2;
					triggerInput1 = byte3;
				}
			}
			else if((byte0 & 1) == 1)
			{
				inData = true;
				data.push_back((uint32_t)byte0 + ((uint32_t)byte1 << 8) +
				               ((uint64_t)byte2 << 16) + ((uint64_t)byte3 << 24));
			}
		}
	}
	return;
}

// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(ots::STIBReceiver)
