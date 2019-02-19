#ifndef artdaq_ots_Overlays_DataGenFragmentWriter_hh
#define artdaq_ots_Overlays_DataGenFragmentWriter_hh

////////////////////////////////////////////////////////////////////////
// DataGenFragmentWriter
//
// Class derived from DataGenFragment which allows writes to the data (for
// simulation purposes). Note that for this reason it contains
// non-const members which hide the const members in its parent class,
// DataGenFragment, including its reference to the artdaq::Fragment
// object, artdaq_Fragment_, as well as its functions pointing to the
// beginning and end of ADC values in the fragment, dataBegin() and
// dataEnd()
//
////////////////////////////////////////////////////////////////////////

#include "artdaq-core/Data/Fragment.hh"
#include "otsdaq-demo/Overlays/DataGenFragment.hh"

#include <iostream>

namespace ots
{
class DataGenFragmentWriter;
}

class ots::DataGenFragmentWriter : public ots::DataGenFragment
{
  public:
	DataGenFragmentWriter(artdaq::Fragment& f);

	// These functions form overload sets with const functions from
	// ots::DataGenFragment

	DataBlob* dataBegin();
	DataBlob* dataEnd();

	// We'll need to hide the const version of header in DataGenFragment in
	// order to be able to perform writes

	Header* header_()
	{
		assert(artdaq_Fragment_.dataSizeBytes() >= sizeof(Header));
		return reinterpret_cast<Header*>(artdaq_Fragment_.dataBeginBytes());
	}

	void resize(size_t nBytes);

	int asksize() { return artdaq_Fragment_.dataSizeBytes(); }

  private:
	// Note that this non-const reference hides the const reference in the base
	// class
	artdaq::Fragment& artdaq_Fragment_;
};

// The constructor will expect the artdaq::Fragment object it's been
// passed to contain the artdaq::Fragment header + the
// DataGenFragment::Metadata object, otherwise it throws

ots::DataGenFragmentWriter::DataGenFragmentWriter(artdaq::Fragment& f)
    : DataGenFragment(f), artdaq_Fragment_(f)
{
	if(!f.hasMetadata() || f.dataSizeBytes() > 0)
	{
		throw cet::exception(
		    "Error in DataGenFragmentWriter: Raw artdaq::Fragment "
		    "object does not appear to consist of (and only of) "
		    "its own header + the DataGenFragment::Metadata "
		    "object");
	}

	// Allocate space for the header
	artdaq_Fragment_.resizeBytes(sizeof(Header));
}

inline ots::DataGenFragment::DataBlob* ots::DataGenFragmentWriter::dataBegin()
{
	// Make sure there's data past the DataGenFragment header
	assert(artdaq_Fragment_.dataSizeBytes() >=
	       sizeof(Header) + sizeof(artdaq::Fragment::value_type));
	return reinterpret_cast<DataBlob*>(header_() + 1);
}

inline ots::DataGenFragment::DataBlob* ots::DataGenFragmentWriter::dataEnd()
{
	return dataBegin() + artdaq_Fragment_.dataSize() / sizeof(DataBlob);
}

inline void ots::DataGenFragmentWriter::resize(size_t nBytes)
{
	artdaq_Fragment_.resizeBytes(nBytes);
}

#endif /* artdaq_demo_Overlays_DataGenFragmentWriter_hh */
