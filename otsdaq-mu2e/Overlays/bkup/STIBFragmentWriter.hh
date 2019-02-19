#ifndef artdaq_ots_Overlays_STIBFragmentWriter_hh
#define artdaq_ots_Overlays_STIBFragmentWriter_hh

////////////////////////////////////////////////////////////////////////
// STIBFragmentWriter
//
// Class derived from STIBFragment which allows writes to the data (for
// simulation purposes). Note that for this reason it contains
// non-const members which hide the const members in its parent class,
// STIBFragment, including its reference to the artdaq::Fragment
// object, artdaq_Fragment_, as well as its functions pointing to the
// beginning and end of ADC values in the fragment, dataBegin() and
// dataEnd()
//
////////////////////////////////////////////////////////////////////////

#include "artdaq-core/Data/Fragment.hh"
#include "otsdaq-demo/Overlays/STIBFragment.hh"

#include <iostream>

namespace ots
{
class STIBFragmentWriter;
}

class ots::STIBFragmentWriter : public ots::STIBFragment
{
  public:
	STIBFragmentWriter(artdaq::Fragment& f);

	// These functions form overload sets with const functions from
	// ots::STIBFragment

	uint8_t* dataBegin();
	uint8_t* dataEnd();

	// We'll need to hide the const version of header in STIBFragment in
	// order to be able to perform writes

	Header* header_()
	{
		assert(artdaq_Fragment_.dataSizeBytes() >= sizeof(Header));
		return reinterpret_cast<Header*>(artdaq_Fragment_.dataBeginBytes());
	}

	void set_hdr_bunch_counter(Header::counter_t counter)
	{
		header_()->bunch_counter = counter;
	}

	void set_hdr_trigger_counter(Header::counter_t counter)
	{
		header_()->trigger_counter = counter;
	}

	void set_hdr_trigger_input0(Header::counter_t input)
	{
		header_()->trigger_input0 = input;
	}

	void set_hdr_trigger_input1(Header::counter_t input)
	{
		header_()->trigger_input1 = input;
	}

	void resize(size_t nBytes);

  private:
	size_t calc_event_size_words_(size_t nBytes);

	static size_t bytes_to_words_(size_t nBytes);

	// Note that this non-const reference hides the const reference in the base
	// class
	artdaq::Fragment& artdaq_Fragment_;
};

// The constructor will expect the artdaq::Fragment object it's been
// passed to contain the artdaq::Fragment header + the
// STIBFragment::Metadata object, otherwise it throws

ots::STIBFragmentWriter::STIBFragmentWriter(artdaq::Fragment& f)
    : STIBFragment(f), artdaq_Fragment_(f)
{
	if(!f.hasMetadata() || f.dataSizeBytes() > 0)
	{
		throw cet::exception(
		    "Error in STIBFragmentWriter: Raw artdaq::Fragment "
		    "object does not appear to consist of (and only of) "
		    "its own header + the STIBFragment::Metadata object");
	}

	// Allocate space for the header
	artdaq_Fragment_.resizeBytes(sizeof(Header));
}

inline uint8_t* ots::STIBFragmentWriter::dataBegin()
{
	// Make sure there's data past the STIBFragment header
	assert(artdaq_Fragment_.dataSizeBytes() >=
	       sizeof(Header) + sizeof(artdaq::Fragment::value_type));
	return reinterpret_cast<uint8_t*>(header_() + 1);
}

inline uint8_t* ots::STIBFragmentWriter::dataEnd()
{
	return dataBegin() + stib_data_words();
}

inline void ots::STIBFragmentWriter::resize(size_t nBytes)
{
	artdaq_Fragment_.resizeBytes(sizeof(Header::data_t) * calc_event_size_words_(nBytes));
	header_()->event_size = calc_event_size_words_(nBytes);
}

inline size_t ots::STIBFragmentWriter::calc_event_size_words_(size_t nBytes)
{
	return bytes_to_words_(nBytes) + hdr_size_words();
}

inline size_t ots::STIBFragmentWriter::bytes_to_words_(size_t nBytes)
{
	auto mod(nBytes % bytes_per_word_());
	return (mod == 0) ? nBytes / bytes_per_word_() : nBytes / bytes_per_word_() + 1;
}

#endif /* artdaq_demo_Overlays_STIBFragmentWriter_hh */
