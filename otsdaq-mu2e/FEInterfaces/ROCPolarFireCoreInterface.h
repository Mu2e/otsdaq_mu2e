#ifndef _ots_ROCPolarFireCoreInterface_h_
#define _ots_ROCPolarFireCoreInterface_h_

#include <sstream>
#include <string>
#include "otsdaq-mu2e/ROCCore/ROCCoreVInterface.h"

namespace ots
{
class ROCPolarFireCoreInterface : public ROCCoreVInterface
{
	// clang-format off
  public:
	ROCPolarFireCoreInterface(const std::string&       rocUID,
	                          const ConfigurationTree& theXDAQContextConfigTree,
	                          const std::string&       interfaceConfigurationPath);

	~ROCPolarFireCoreInterface(void);

	enum CaloTrkRegisters
	{
		ROC_ADDRESS_ACTION_DONE             = 128,
		ROC_ADDRESS_ACTION_READ_SIZE       	= 129,
		ROC_ADDRESS_ACTION_STATUS         	= 132,
		ROC_ADDRESS_ACTION_COMMAND	      	= 384,
	};

	enum CaloTrkActions
	{
		ROC_ACTION_READ_SPI             	= 7,
		ROC_ACTION_WRITE_SPI             	= 8,
		ROC_ACTION_WRITE_DIR             	= 9,

		ROC_ACTION_ERASE_ADDR             	= 3,
		ROC_ACTION_PROG_INDEX             	= 4,
		ROC_ACTION_PROG_ADDR             	= 5,
		ROC_ACTION_PROG_AUTO             	= 6,
	};

	std::mutex 								actionLock_; /// protect/lock this link/ROC from starting more than one action

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
	virtual void 							writeEmulatorRegister	(DTCLib::roc_address_t address, DTCLib::roc_data_t data_to_write) override;
	virtual uint16_t						readEmulatorRegister	(DTCLib::roc_address_t address) override;

	virtual void 							readEmulatorBlock		(std::vector<DTCLib::roc_data_t>& data, DTCLib::roc_address_t address, uint16_t numberOfReads, bool incrementAddress) override;

	// specific ROC functions
	virtual int  							readInjectedPulseTimestamp					(void) override;
	virtual void 							writeDelay									(uint16_t delay) override;  // 5ns steps
	virtual int  							readDelay									(void) override;            	// 5ns steps

	virtual int  							readDTCLinkLossCounter						(void) override;
	virtual void 							resetDTCLinkLossCounter						(void) override;


	virtual void  							GetStatus									(__ARGS__) override;
	virtual std::string						getFirmwareVersion							(void) override;
	void 									SetupForPatternDataTaking					(__ARGS__);

	bool									isActionDone								(DTCLib::roc_data_t* readStatus = nullptr, bool releaseLockOnDone = false) override; /// consider using actionLock_ to protect/lock this link/ROC from starting more than one action
	void 									readSPIFlashBlock							(std::vector<uint16_t>& readData, uint32_t startAddress, uint8_t numberOfBytes) override;
	void 									writeSPIFlashDirectory						(const std::vector<uint32_t>& imageAddresses, bool waitForDone = true) override;
	void 									writeSPIFlashBlock							(const std::vector<uint16_t>& writeData, uint32_t startAddress, bool waitForDone = true) override;
	void 									eraseSPIFlashBlock							(uint32_t eraseSize, uint32_t startAddress, bool waitForDone = true) override;
	void 									programFromSPIByIndex						(uint8_t index, bool waitForDone = true) override;
	void 									programFromSPIByAddress						(uint32_t startAddress, bool waitForDone = true) override;
	void 									autoProgramFromSPI							(bool waitForDone = true) override;
	void 									forceClearActionLock						(void) override;

	void 									ReadSPIFlashBlock							(__ARGS__);
	void 									WriteSPIFlashDirectory						(__ARGS__);
	void 									EraseSPIFlashBlock							(__ARGS__);
	void 									ForceClearActionLock						(__ARGS__);

	// clang-format on
};

}  // namespace ots

#endif
