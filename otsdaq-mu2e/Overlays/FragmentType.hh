#ifndef artdaq_ots_Overlays_FragmentType_hh
#define artdaq_ots_Overlays_FragmentType_hh
#include "artdaq-core/Data/Fragment.hh"

namespace ots
{
std::vector<std::string> const names{"MISSED", "UDP", "STIB", "DataGen", "UNKNOWN"};

namespace detail
{
enum FragmentType : artdaq::Fragment::type_t
{
	MISSED = artdaq::Fragment::FirstUserFragmentType,
	UDP,
	STIB,
	DataGen,
	INVALID  // Should always be last.
};

// Safety check.
static_assert(artdaq::Fragment::isUserFragmentType(FragmentType::INVALID - 1),
              "Too many user-defined fragments!");
}  // namespace detail

using detail::FragmentType;

FragmentType toFragmentType(std::string t_string);
std::string  fragmentTypeToString(FragmentType val);
}  // namespace ots
#endif /* artdaq_ots_core_Overlays_FragmentType_hh */
