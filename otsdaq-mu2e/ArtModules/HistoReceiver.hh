#include "otsdaq/NetworkUtilities/TCPSendClient.h"
#include <TDirectory.h>
#include <string>
#include <vector>

namespace ots {

  class HistoReceiver {
  public:
    void addHistogram(TObject*readObject, TDirectory*subdir);
    void readPacket(TDirectory *dir, std::string* buf);

  };
}



