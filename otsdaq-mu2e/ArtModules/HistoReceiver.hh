#include <TDirectory.h>
#include <TGraph.h>
#include <TH1.h>
#include <TObject.h>
#include <string>
#include <vector>
#include "otsdaq/NetworkUtilities/TCPSendClient.h"

namespace ots
{

class HistoReceiver
{
  public:
	enum
	{
		kAdd,
		kReplace
	};
	void addHistogram(TH1* h, TDirectory* subdir, int mode);
	void addGraph(TGraph* g, TDirectory* subdir, int mode);
	void addObject(TObject* readObject, TDirectory* subdir, int mode);
	void readPacket(TDirectory* dir, std::string* buf);
	int  parseMode(std::string mode);
};
}  // namespace ots
