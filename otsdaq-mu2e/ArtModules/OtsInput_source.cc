#include "art/Framework/Core/InputSourceMacros.h"
#include "art/Framework/IO/Sources/Source.h"
#include "otsdaq-demo/ArtModules/detail/RawEventQueueReader.hh"

#include <string>
using std::string;

namespace ots
{
typedef art::Source<detail::RawEventQueueReader> OtsInput;
}

DEFINE_ART_INPUT_SOURCE(ots::OtsInput)
