#include "otsdaq-mu2e/ArtModules/HistoSender.hh"


namespace ots{
  HistoSender::HistoSender(std::string serverHost, int serverPort):sender_(serverHost,serverPort),
    buffer_(TBufferFile(TBufferFile::kWrite)){

    sender_.connect();
  }

  HistoSender::~HistoSender(void){
    //sender_.disconnect();
  }

  void HistoSender::sendHistogram (std::string directoryName, TH1* hist){
    buffer_.SetWriteMode();
    buffer_.WriteStdString(directoryName);
    buffer_.WriteObject(hist);
    sender_.sendPacket(buffer_.Buffer(), buffer_.Length());
    buffer_.Reset();
  }

  void HistoSender::sendHistograms(std::string directoryName, std::vector<TH1*> &hists){
    buffer_.SetWriteMode();
    buffer_.WriteStdString(directoryName);
    for (size_t i=0; i<hists.size(); ++i){
      buffer_.WriteObject(hists[i]);
    }
    sender_.sendPacket(buffer_.Buffer(), buffer_.Length());
    buffer_.Reset();
  }
}
