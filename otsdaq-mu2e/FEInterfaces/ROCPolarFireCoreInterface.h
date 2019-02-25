#ifndef _ots_ROCPolarFireCoreInterface_h_
#define _ots_ROCPolarFireCoreInterface_h_

#include <sstream>
#include <string>
#include "otsdaq-mu2e/ROCCore/ROCCoreVInterface.h"

namespace ots
{
class ROCPolarFireCoreInterface : public ROCCoreVInterface
{
  public:
	ROCPolarFireCoreInterface(const std::string&       rocUID,
	                          const ConfigurationTree& theXDAQContextConfigTree,
	                          const std::string&       interfaceConfigurationPath);

	~ROCPolarFireCoreInterface(void);

	// state machine
	//----------------
	void configure(void) override;
	void halt(void) override;
	void pause(void) override;
	void resume(void) override;
	void start(std::string runNumber) override;
	void stop(void) override;
	bool running(void) override;

	// write and read to registers
	virtual void writeROCRegister(unsigned address, unsigned data_to_write) override;
	virtual int  readROCRegister(unsigned address) override;
	virtual void writeEmulatorRegister(unsigned address, unsigned data_to_write) override
	{
	}
	virtual int readEmulatorRegister(unsigned address) override { return -1; }

	// specific ROC functions
	int  readTimestamp();
	void writeDelay(unsigned delay);  // 5ns steps
	int  readDelay();                 // 5ns steps

	int  readDTCLinkLossCounter();
	void resetDTCLinkLossCounter();

	void        highRateCheck(void);
	static void highRateCheckThread(ROCPolarFireCoreInterface* roc);
};

}  // namespace ots

#endif
