#ifndef artdaq_ots_Overlays_DataGenFragment_hh
#define artdaq_ots_Overlays_DataGenFragment_hh

#include "artdaq-core/Data/Fragment.hh"
#include "cetlib/exception.h"

#include <ostream>
#include <vector>

// Implementation of "DataGenFragment", an artdaq::Fragment overlay class

namespace ots
{
class DataGenFragment;

// Let the "<<" operator dump the DataGenFragment's data to stdout
std::ostream& operator<<(std::ostream&, DataGenFragment const&);
}  // namespace ots

class ots::DataGenFragment
{
  public:
	// The "Metadata" struct is used to store info primarily related to
	// the upstream hardware environment from where the fragment came

	// "data_t" is a typedef of the fundamental unit of data the
	// metadata structure thinks of itself as consisting of; it can give
	// its size via the static "size_words" variable (
	// DataGenFragment::Metadata::size_words )

	struct Metadata
	{
		typedef uint64_t data_t;

		data_t port : 16;
		data_t address : 32;
		data_t unused : 16;

		static size_t const size_words = 1ull;  // Units of Metadata::data_t
	};

	static_assert(sizeof(Metadata) == Metadata::size_words * sizeof(Metadata::data_t),
	              "DataGenFragment::Metadata size changed");

	// The "Header" struct contains "metadata" specific to the fragment
	// which is not hardware-related

	struct Header
	{
		typedef uint8_t data_t;

		data_t type;
		data_t sequence;

		static size_t const size_words = 2ul;  // Units of Header::data_t
	};

	static_assert(sizeof(Header) == Header::size_words * sizeof(Header::data_t),
	              "DataGenFragment::Header size changed");

	struct DataBlob
	{
		double data[10];
	};

	// The constructor simply sets its const private member "artdaq_Fragment_"
	// to refer to the artdaq::Fragment object

	DataGenFragment(artdaq::Fragment const& f) : artdaq_Fragment_(f) {}

	// const getter functions for the data in the header

	// Start of the data, returned as a pointer
	DataBlob const* dataBegin() const
	{
		return reinterpret_cast<DataBlob const*>(header_() + 1);
		// return reinterpret_cast<DataBlob const *>(header_());
	}
	// 0 - type
	// 1 - seq
	// 2-9 - data

	// End of the data, returned as a pointer
	DataBlob const* dataEnd() const
	{
		return dataBegin() + (artdaq_Fragment_.dataSize() / sizeof(DataBlob));
	}

  protected:
	// header_() simply takes the address of the start of this overlay's
	// data (i.e., where the DataGenFragment::Header object begins) and
	// casts it as a pointer to DataGenFragment::Header

	Header const* header_() const
	{
		return reinterpret_cast<DataGenFragment::Header const*>(
		    artdaq_Fragment_.dataBeginBytes());
	}

  private:
	artdaq::Fragment const& artdaq_Fragment_;
};

#endif /* artdaq_ots_core_Overlays_DataGenFragment_hh */
