#ifndef _ots_DemoDQMHistos_h_
#define _ots_DemoDQMHistos_h_

#include <map>
#include <queue>
#include <string>

class TH1I;
class TFile;

namespace ots
{
class DemoDQMHistos
{
  public:
	DemoDQMHistos(void);
	virtual ~DemoDQMHistos(void);
	void book(TFile* rootFile);
	void fill(std::string& buffer, std::map<std::string, std::string> header);
	void load(std::string fileName);

  protected:
	TH1I* sequenceNumbers_;
	TH1I* dataNumbers_;
};
}  // namespace ots

#endif
