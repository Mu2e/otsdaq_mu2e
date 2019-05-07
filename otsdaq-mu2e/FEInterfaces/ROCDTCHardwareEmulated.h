#ifndef _ots_ROCDTCHardwareEmulated_h_
#define _ots_ROCDTCHardwareEmulated_h_

#include <sstream>
#include <string>
#include "otsdaq-mu2e/ROCCore/ROCCoreVInterface.h"

namespace ots
{
class ROCDTCHardwareEmulated : public ROCCoreVInterface
{
  public:
	ROCDTCHardwareEmulated(const std::string&       rocUID,
	                          const ConfigurationTree& theXDAQContextConfigTree,
	                          const std::string&       interfaceConfigurationPath);

	~ROCDTCHardwareEmulated(void);

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
	virtual int readEmulatorRegister(unsigned address) override;

	virtual void readROCBlock(std::vector<uint16_t>& data, unsigned address,unsigned numberOfReads,unsigned incrementAddress) override {	}
	virtual void readEmulatorBlock(std::vector<uint16_t>& data, unsigned address,unsigned numberOfReads,unsigned incrementAddress) override {	}


	// specific ROC functions
	virtual int  readTimestamp() override;
	virtual void writeDelay(unsigned delay) override;  // 5ns steps
	virtual int  readDelay() override;                 // 5ns steps

	virtual int  readDTCLinkLossCounter() override;
	virtual void resetDTCLinkLossCounter() override;
};

}  // namespace ots

#endif
