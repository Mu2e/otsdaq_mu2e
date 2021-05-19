#ifndef _ProtoTypeHistos_h_
#define _ProtoTypeHistos_h_

#include <TH1F.h>
#include <string>
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art_root_io/TFileDirectory.h"
#include "art_root_io/TFileService.h"
#include "otsdaq/NetworkUtilities/TCPPublishServer.h"

namespace ots
{
class HistoContainer
{
  public:
	HistoContainer(){};
	virtual ~HistoContainer(void){};
	struct summaryInfoHist_
	{
		TH1F* _Hist;
		int   station;
		int   plane;
		int   panel;
		int   straw;
		summaryInfoHist_() { _Hist = NULL; }
	};

	std::vector<summaryInfoHist_> histograms;

	void BookHistos(
	    TDirectory* dir, std::string Title, int station, int plane, int panel, int straw)
	{
		dir->mkdir("TestingHistos", "TestingHistos");
		histograms.push_back(summaryInfoHist_());
		this->histograms[histograms.size() - 1]._Hist =
		    new TH1F(Title.c_str(), Title.c_str(), 1000, 0, 4000);
		this->histograms[histograms.size() - 1].station = station;
		this->histograms[histograms.size() - 1].plane   = plane;
		this->histograms[histograms.size() - 1].panel   = panel;
		this->histograms[histograms.size() - 1].straw   = straw;
	}

	void BookHistos(art::ServiceHandle<art::TFileService> tfs,
	                std::string                           Title,
	                int                                   station,
	                int                                   plane,
	                int                                   panel,
	                int                                   straw)
	{
		histograms.push_back(summaryInfoHist_());
		art::TFileDirectory TestDir = tfs->mkdir("TestingHistos");
		this->histograms[histograms.size() - 1]._Hist =
		    TestDir.make<TH1F>(Title.c_str(), Title.c_str(), 1000, 0, 4000);
		this->histograms[histograms.size() - 1].station = station;
		this->histograms[histograms.size() - 1].plane   = plane;
		this->histograms[histograms.size() - 1].panel   = panel;
		this->histograms[histograms.size() - 1].straw   = straw;
	}

	void BookHistos(TDirectory* dir, std::string Title)
	{
		dir->mkdir("TestingHistos", "TestingHistos");
		histograms.push_back(summaryInfoHist_());
		this->histograms[histograms.size() - 1]._Hist =
		    new TH1F(Title.c_str(), Title.c_str(), 1000, 0, 4000);
	}
};

}  // namespace ots

#endif
