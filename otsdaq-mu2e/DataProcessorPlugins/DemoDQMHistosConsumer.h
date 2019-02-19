#ifndef _ots_DemoDQMHistosConsumer_h_
#define _ots_DemoDQMHistosConsumer_h_

#include "otsdaq-core/Configurable/Configurable.h"
#include "otsdaq-core/DataManager/DQMHistosConsumerBase.h"

#include <string>

namespace ots
{
class ConfigurationManager;
class DemoDQMHistos;

class DemoDQMHistosConsumer : public DQMHistosConsumerBase, public Configurable
{
  public:
	DemoDQMHistosConsumer(std::string              supervisorApplicationUID,
	                      std::string              bufferUID,
	                      std::string              processorUID,
	                      const ConfigurationTree& theXDAQContextConfigTree,
	                      const std::string&       configurationPath);
	virtual ~DemoDQMHistosConsumer(void);

	void startProcessingData(std::string runNumber) override;
	void stopProcessingData(void) override;

  private:
	bool workLoopThread(toolbox::task::WorkLoop* workLoop);
	void fastRead(void);
	void slowRead(void);

	// For fast read
	std::string*                        dataP_;
	std::map<std::string, std::string>* headerP_;
	// For slow read
	std::string                        data_;
	std::map<std::string, std::string> header_;

	bool           saveDQMFile_;  // yes or no
	std::string    DQMFilePath_;
	std::string    DQMFilePrefix_;
	DemoDQMHistos* dqmHistos_;
};
}  // namespace ots

#endif
