#include "otsdaq-demo/Overlays/STIBFragment.hh"

#include "cetlib/exception.h"

std::ostream& ots::operator<<(std::ostream& os, STIBFragment const& f)
{
	os << "STIBFragment_event_size: " << f.hdr_event_size() << "\n";

	return os;
}
