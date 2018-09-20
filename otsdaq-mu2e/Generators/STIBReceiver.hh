#ifndef artdaq_ots_Generators_STIBReceiver_hh
#define artdaq_ots_Generators_STIBReceiver_hh


// Some C++ conventions used:

// -Append a "_" to every private member function and variable

#include "fhiclcpp/fwd.h"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-ots/Generators/UDPReceiver.hh"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <list>
#include <queue>
#include <atomic>

namespace ots {    

  class STIBReceiver : public ots::UDPReceiver {
  public:
    explicit STIBReceiver(fhicl::ParameterSet const & ps);

  private:
    void ProcessData_(artdaq::FragmentPtrs & frags) override;
  };
}

#endif /* artdaq_demo_Generators_ToySimulator_hh */
