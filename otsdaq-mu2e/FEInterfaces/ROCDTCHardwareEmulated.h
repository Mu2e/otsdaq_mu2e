#ifndef _ots_ROCDTCHardwareEmulated_h_
#define _ots_ROCDTCHardwareEmulated_h_

#include <sstream>
#include <string>
#include "otsdaq-mu2e/ROCCore/ROCCoreVInterface.h"

namespace ots
{
class ROCDTCHardwareEmulated : public ROCCoreVInterface
{
	// clang-format off
  public:
	ROCDTCHardwareEmulated(
		const std::string&       rocUID,
		const ConfigurationTree& theXDAQContextConfigTree,
		const std::string&       interfaceConfigurationPath);

	~ROCDTCHardwareEmulated(void);

	// state machine
	//----------------
	void 									configure				(void) override;
	void 									halt					(void) override;
	void 									pause					(void) override;
	void 									resume					(void) override;
	void 									start					(std::string runNumber) override;
	void 									stop					(void) override;
	bool 									running					(void) override;

	// write and read to registers
	virtual void 							writeROCRegister		(uint16_t address, uint16_t data_to_write) override;
	virtual int  							readROCRegister			(uint16_t address) override;
	virtual void 							writeEmulatorRegister	(uint16_t address, uint16_t data_to_write) override { }
	virtual int 							readEmulatorRegister	(uint16_t address) override;

	virtual void 							readROCBlock			(std::vector<uint16_t>& data, uint16_t address, uint16_t numberOfReads, bool incrementAddress) override { }
	virtual void 							readEmulatorBlock		(std::vector<uint16_t>& data, uint16_t address, uint16_t numberOfReads, bool incrementAddress) override { }


	// specific ROC functions
	virtual int  							readTimestamp			(void) override;
	virtual void 							writeDelay				(uint16_t delay) override;  // 5ns steps
	virtual int  							readDelay				(void) override;            	// 5ns steps

	virtual int  							readDTCLinkLossCounter	(void) override;
	virtual void 							resetDTCLinkLossCounter	(void) override;

	// clang-format on
};

}  // namespace ots

#endif
