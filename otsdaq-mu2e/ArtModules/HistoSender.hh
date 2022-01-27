#include "otsdaq/NetworkUtilities/TCPSendClient.h"
#include <TH1.h>
#include <string>
#include <vector>
#include "TBufferFile.h"

namespace ots {

  class HistoSender {
  public:
    HistoSender(std::string serverHost, int serverPort);
    ~HistoSender(void);

    void sendHistogram (std::string directoryName, TH1* hist);
    void sendHistograms(std::string directoryName, std::vector<TH1*> &hists);
    void sendHistograms(std::map<std::string, std::vector<TH1*>>& hists);
  
  private:
    TCPSendClient    sender_;
  };
}



