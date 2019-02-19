#ifndef otsdaq_demo_ArtModules_detail_RawEventQueueReader_hh
#define otsdaq_demo_ArtModules_detail_RawEventQueueReader_hh

#include <map>
#include <string>

#include "artdaq/ArtModules/detail/RawEventQueueReader.hh"

namespace ots
{
namespace detail
{
struct RawEventQueueReader : public artdaq::detail::RawEventQueueReader
{
	RawEventQueueReader(RawEventQueueReader const&) = delete;
	RawEventQueueReader& operator=(RawEventQueueReader const&) = delete;

	RawEventQueueReader(fhicl::ParameterSet const&  ps,
	                    art::ProductRegistryHelper& help,
	                    art::SourceHelper const&    pm);

	RawEventQueueReader(fhicl::ParameterSet const&  ps,
	                    art::ProductRegistryHelper& help,
	                    art::SourceHelper const&    pm,
	                    art::MasterProductRegistry&)
	    : RawEventQueueReader(ps, help, pm)
	{
	}
};

}  // namespace detail
}  // namespace ots

// Specialize an art source trait to tell art that we don't care about
// source.fileNames and don't want the files services to be used.
namespace art
{
template<>
struct Source_generator<ots::detail::RawEventQueueReader>
{
	static constexpr bool value = true;
};
}  // namespace art

#endif /* artdaq_demo_ArtModules_detail_RawEventQueueReader_hh */
