#include <TDirectory.h>
#include <string>
#include <vector>
#include "otsdaq/NetworkUtilities/TCPSendClient.h"

namespace ots
{

class HistoReceiver
{
  public:
	void addHistogram(TObject* readObject, TDirectory* subdir);
	void readPacket(TDirectory* dir, std::string* buf);
};
}  // namespace ots
