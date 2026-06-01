#include "otsdaq-mu2e/FEInterfaces/ROCPolarFireCoreInterface.h"

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "FE-ROCPolarFireCoreInterface"

//=========================================================================================
ROCPolarFireCoreInterface::ROCPolarFireCoreInterface(
    const std::string&       rocUID,
    const ConfigurationTree& theXDAQContextConfigTree,
    const std::string&       theConfigurationPath)
    : ROCCoreVInterface(rocUID, theXDAQContextConfigTree, theConfigurationPath)
{
	INIT_MF("." /*directory used is USER_DATA/LOG/.*/);

	__COUT__ << "ROCPolarFireCoreInterface instantiated with link: " << linkID_
	         << " and EventWindowDelayOffset = " << delay_ << __E__;

	registerFEMacroFunction("Setup for Pattern Data Taking",
	                        static_cast<FEVInterface::frontEndMacroFunction_t>(
	                            &ROCPolarFireCoreInterface::SetupForPatternDataTaking),
	                        std::vector<std::string>{},  //inputs parameters
	                        std::vector<std::string>{},  //output parameters
	                        1);                          // requiredUserPermissions

	registerFEMacroFunction(
	    "Read SPI Flash Block",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &ROCPolarFireCoreInterface::ReadSPIFlashBlock),
	    std::vector<std::string>{"Start Address", "Number of Bytes"},  //inputs parameters
	    std::vector<std::string>{"Result"},                            //output parameters
	    1);  // requiredUserPermissions

	registerFEMacroFunction(
	    "Write SPI Flash Directory",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &ROCPolarFireCoreInterface::WriteSPIFlashDirectory),
	    std::vector<std::string>{"Path to Directory Map file"},  //inputs parameters
	    std::vector<std::string>{"Result"},                      //output parameters
	    1);  // requiredUserPermissions

	registerFEMacroFunction(
	    "Erase SPI Flash Block",
	    static_cast<FEVInterface::frontEndMacroFunction_t>(
	        &ROCPolarFireCoreInterface::EraseSPIFlashBlock),
	    std::vector<std::string>{"Start Address", "Number of Bytes"},  //inputs parameters
	    std::vector<std::string>{},                                    //output parameters
	    1);  // requiredUserPermissions

	registerFEMacroFunction("Force Clear Action Lock",
	                        static_cast<FEVInterface::frontEndMacroFunction_t>(
	                            &ROCPolarFireCoreInterface::ForceClearActionLock),
	                        std::vector<std::string>{},  //inputs parameters
	                        std::vector<std::string>{},  //output parameters
	                        1);                          // requiredUserPermissions

}  // end constructor()

//==========================================================================================
ROCPolarFireCoreInterface::~ROCPolarFireCoreInterface(void)
{
	// NOTE:: be careful not to call __FE_COUT__ decoration because it uses the
	// tree and it may already be destructed partially
	// Instead use __GEN_COUT__ which decorates using mfSubject_
	__GEN_COUT__ << "Destructed." << __E__;
}  // end destructor()

//==================================================================================================
uint16_t ROCPolarFireCoreInterface::readEmulatorRegister(uint16_t address)
{
	__FE_SS__ << "Deprecated! Use mu2esim in mu2e-pcie-utils" << __E__;
	__FE_SS_THROW__;

	// __FE_COUT__ << "Calling read emulator ROC register: link number " << std::dec
	//             << linkID_ << ", address = " << address << __E__;
	// if(address == 6)
	// 	return 4860;
	// else if(address == 7)
	// 	return delay_;
	// return emulatorRegisters_[address];
}  // end readEmulatorRegister()

//==================================================================================================
void ROCPolarFireCoreInterface::writeEmulatorRegister(uint16_t           address,
                                                      DTCLib::roc_data_t writeData)
{
	__FE_SS__ << "Deprecated! Use mu2esim in mu2e-pcie-utils" << __E__;
	__FE_SS_THROW__;

	// __FE_COUT__ << "Calling write emulator ROC register: link number " << std::dec
	//             << linkID_ << ", address = " << address << ", writeData = " << writeData << __E__;
	// if(address == 6)
	// 	; //4860;
	// else if(address == 7)
	// 	; // delay_;
	// else
	// 	emulatorRegisters_[address] = writeData;
}  // end readEmulatorRegister()

//==================================================================================================
void ROCPolarFireCoreInterface::readEmulatorBlock(std::vector<DTCLib::roc_data_t>& data,
                                                  DTCLib::roc_address_t address,
                                                  uint16_t              numberOfReads,
                                                  bool                  incrementAddress)
{
	__FE_COUT__ << "Calling read emulator block: link number " << std::dec << linkID_
	            << ", address = " << address << ", numberOfReads = " << numberOfReads
	            << ", incrementAddress = " << incrementAddress << __E__;

	for(unsigned int i = 0; i < numberOfReads; ++i)
		data.push_back(address + (incrementAddress ? i : 0));
}  // end readEmulatorBlock()

//==================================================================================================
void ROCPolarFireCoreInterface::GetStatus(__ARGS__)
{
	__SS__ << "TODO";
	__SS_THROW__;
}

//==================================================================================================
std::string ROCPolarFireCoreInterface::getFirmwareVersion()
{
	__SS__ << "TODO";
	__SS_THROW__;
}

//==================================================================================================
int ROCPolarFireCoreInterface::readInjectedPulseTimestamp()
{
	return this->readRegister(12);
}

//==================================================================================================
void ROCPolarFireCoreInterface::writeDelay(uint16_t delay)
{
	this->writeRegister(21, delay);
	return;
}

//==================================================================================================
int ROCPolarFireCoreInterface::readDelay() { return this->readRegister(7); }

//==================================================================================================
int ROCPolarFireCoreInterface::readDTCLinkLossCounter() { return this->readRegister(8); }

//==================================================================================================
void ROCPolarFireCoreInterface::resetDTCLinkLossCounter()
{
	this->writeRegister(24, 0x1);
	return;
}

//==================================================================================================
void ROCPolarFireCoreInterface::configure(void)
try
{
	__FE_COUT_INFO__ << "......... Clear DCS FIFOs" << __E__;
	// this->writeRegister(0,1);
	//this->writeRegister(0,0);  // MT: in DracMonitor, write ANY to addr 0 to issue TOP_SERDES reset. Self-clearing.

	// setup needToResetAlignment using rising edge of register 22
	// (i.e., force synchronization of ROC clock with 40MHz system clock)
	__FE_COUT_INFO__ << "......... setup to synchronize ROC clock with 40 MHz clock edge"
	                 << __E__;
	//this->writeRegister(22, 0);
	//this->writeRegister(22, 1);
	//this->writeRegister(4, 1); // MT: in DracMonitor, DCS_ALIGNMENT is addr 4.  Self-clearing

	this->writeDelay(delay_);

	__FE_COUT_INFO__ << "........."
	                 << " Set delay = " << delay_ << ", readback = " << this->readDelay()
	                 << "... " << __E__;

	__FE_COUT__ << "Debugging ROC-DCS" << __E__;

	unsigned int val;

	// read 6 should read back 0x12fc
	for(int i = 0; i < 1; i++)
	{
		val = this->readRegister(6);

		//__FE_COUT_INFO__ << i << " read register 6 = " << val << __E__;
		if(val != 4860)
		{
			__FE_SS__ << "Bad read not 4860! val = " << val << __E__;
			//__FE_SS_THROW__;   disable for the moment, so we can debug
		}

		val = this->readDelay();
		//__FE_COUT_INFO__ << i << " read register 7 = " << val << __E__;
		if(val != delay_)
		{
			__FE_SS__ << "Bad read not " << delay_ << "! val = " << val << __E__;
			//__FE_SS_THROW__;   disable for the moment, so we can debug
		}
	}

	__FE_COUT_INFO__ << "......... reset DTC link loss counter ... " << __E__;
	resetDTCLinkLossCounter();
}  // end configure()
catch(const std::runtime_error& e)
{
	__FE_COUT__ << "Error caught: " << e.what() << __E__;
	throw;
}
catch(...)
{
	__FE_SS__ << "Unknown error caught. Check printouts!" << __E__;
	try
	{
		throw;
	}  //one more try to printout extra info
	catch(const std::exception& e)
	{
		ss << "Exception message: " << e.what();
	}
	catch(...)
	{
	}
	__FE_SS_THROW__;
}  // end configure() catch

//==============================================================================
void ROCPolarFireCoreInterface::halt(void)
{
	// Call Core Halt() first to stop emulator threads properly
	ROCCoreVInterface::halt();

	// do specifics here:
	// ...
}  // end halt()

//==============================================================================
void ROCPolarFireCoreInterface::pause(void) {}

//==============================================================================
void ROCPolarFireCoreInterface::resume(void) {}

//==============================================================================
void ROCPolarFireCoreInterface::start(std::string)  // runNumber)
{
}

//==============================================================================
void ROCPolarFireCoreInterface::stop(void) {}

//==============================================================================
bool ROCPolarFireCoreInterface::running(void) { return false; }

//==================================================================================================
void ROCPolarFireCoreInterface::SetupForPatternDataTaking(__ARGS__)
{
	__COUT_INFO__ << "SetupForPatternDataTaking()" << __E__;

	//For future, to get link ID of this ROC:
	__FE_COUTV__(getLinkID());

	writeRegister(14, 1);  //ROC reset
	writeRegister(8, 1 << 4);
	writeRegister(30, 0);
	writeRegister(29, 1);

	__COUT_INFO__ << "end SetupForPatternDataTaking()" << __E__;

	// __SET_ARG_OUT__("readValue",GetTemperature(channelnumber));
}  //end SetupForPatternDataTaking()

//==================================================================================================
/// BUSY/DONE from register 128, bit[15]: 0 indicates that the microprocessor is
/// still busy and the function is not done, 1 that the microprocessor is done
bool ROCPolarFireCoreInterface::isActionDone(
    DTCLib::roc_data_t* readStatus /* = nullptr */, bool releaseLockOnDone /* = false */)
{
	DTCLib::roc_data_t readValue = readRegister(ROC_ADDRESS_ACTION_DONE);
	bool               done      = (readValue >> 15) & 1;

	// Detect communication failure: 0xffff means all bits set,
	// which typically indicates the ROC is not responding over DCS.
	if(readValue == 0xffff)
	{
		__FE_COUT_WARN__ << "WARNING: register " << ROC_ADDRESS_ACTION_DONE
		                 << " returned 0xffff - possible ROC communication failure!"
		                 << __E__;
	}

	if(done && readStatus)  //check status also
	{
		*readStatus = readRegister(ROC_ADDRESS_ACTION_STATUS);
	}
	if(done && releaseLockOnDone)
	{
		actionLock_.unlock();
		__FE_COUTT__ << "Released ROC action lock" << __E__;
	}
	return done;
}  //end isActionDone()

//==================================================================================================
/// This function returns a vector with size equal to the number of words found in the SPI flash at startAddr via a
/// DTC block read of Nbytes/2 from register 384, since each block read word concatenates
/// two bytes read from consecutive addresses.
///
/// Note: The maximum allowed number of bytes to read is 1016
void ROCPolarFireCoreInterface::readSPIFlashBlock(std::vector<uint16_t>& readData,
                                                  uint32_t               startAddress,
                                                  uint16_t               numberOfBytes)
{
	launchSPIFlashBlockRead(startAddress, numberOfBytes);
	collectSPIFlashBlockRead(readData, numberOfBytes);

}  //end readSPIFlashBlock()

//==================================================================================================
void ROCPolarFireCoreInterface::launchSPIFlashBlockRead(uint32_t startAddress,
                                                        uint16_t numberOfBytes)
{
	if(numberOfBytes > 1016)
	{
		__FE_SS__ << "Illegal number of bytes requested for read SPI flash action: "
		          << numberOfBytes << __E__;
		__FE_SS_THROW__;
	}

	__FE_COUTV__(startAddress);
	__FE_COUTV__((int)numberOfBytes);

	std::vector<DTCLib::roc_data_t> commandData = {
	    ROC_ACTION_READ_SPI,
	    //1st command word, is a 32-bit parameter passed via the second (16LSB) and third (16MSB) command words
	    DTCLib::roc_data_t(startAddress),        //LSBs
	    DTCLib::roc_data_t(startAddress >> 16),  //MSBs
	    //2nd command word, is a 32-bit parameter passed via the fourth (16LSB) and fifth (16MSB) command words
	    numberOfBytes,  //LSBs
	    0               //MSBs
	};

	__FE_COUTV__(StringMacros::vectorToString(commandData));

	if(actionLock_.try_lock())
	{
		__FE_COUTT__ << "Have ROC action lock" << __E__;
	}
	else
	{
		__FE_SS__ << "Could not get ROC action lock (is there an incomplete action?)!" << __E__;
		__FE_SS_THROW__;
	}

	try
	{
		writeBlock(commandData, ROC_ADDRESS_ACTION_COMMAND, false /* incrementAddress */);
	}
	catch(const std::exception& e)
	{
		__FE_COUT__ << "Caught exception, releasing action lock..." << __E__;
		actionLock_.unlock();
		throw;
	}

}  //end launchSPIFlashBlockRead()

//==================================================================================================
void ROCPolarFireCoreInterface::collectSPIFlashBlockRead(std::vector<uint16_t>& readData,
                                                         uint16_t               numberOfBytes)
{
	std::vector<uint16_t> tmpReadData;
	try
	{
		// wait for both DONE and the expected TX word count. Register 128 can
		// still contain DONE from the previous action for a short time after
		// writeBlock(), so DONE alone is not a safe completion condition here.
		const size_t expectedReadCount = numberOfBytes / 2 + 4;
		size_t       readCount         = 0;
		size_t       i                 = 0;
		while(true)
		{
			const bool done = isActionDone();
			readCount       = readRegister(ROC_ADDRESS_ACTION_READ_SIZE) &
			            0x7ff;  //only low 11-bits are size (12 is empty, 14 is full)

			if(done && readCount == expectedReadCount)
				break;

			if(i > 5 * 100 /* 5 seconds */)
			{
				auto doneFinal   = readRegister(ROC_ADDRESS_ACTION_DONE);
				auto countFinal  = readRegister(ROC_ADDRESS_ACTION_READ_SIZE);
				auto statusFinal = readRegister(ROC_ADDRESS_ACTION_STATUS);

				__FE_SS__ << "Timeout waiting for SPI flash block read action! Check "
				             "for more info with ROC Read to "
				          << ROC_ADDRESS_ACTION_DONE << ", read count 0x" << std::hex
				          << readCount << " expected 0x" << expectedReadCount
				          << ". Final state: reg128=0x" << doneFinal << " reg129=0x"
				          << countFinal << " reg132=0x" << statusFinal << __E__;
				__FE_SS_THROW__;
			}
			usleep(1000 * 10 /* 10 ms */);
			++i;
		}
		__FE_COUTT__ << "SPI read action done, reading status..." << __E__;

		__FE_COUTV__(readCount);
		if(readCount - 4 != numberOfBytes / 2)
		{
			__FE_SS__ << "Illegal read count received after SPI flash directory read action: 0x"
			          << std::hex << readCount << " expected 0x" << numberOfBytes / 2 + 4
			          << __E__ << "Consider emptying manually by reading 0x" << readCount - 4
			          << " words with Block Read from address 0x" << ROC_ADDRESS_ACTION_COMMAND
			          << __E__;
			__FE_SS_THROW__;
		}

		readBlock(tmpReadData,
		          ROC_ADDRESS_ACTION_COMMAND,
		          readCount - 4,
		          false /* incrementAddress */);
	}
	catch(const std::exception& e)
	{
		__FE_COUT__ << "Caught exception, releasing action lock..." << __E__;
		actionLock_.unlock();
		throw;
	}
	actionLock_.unlock();

	__FE_COUTV__(tmpReadData.size());

	//now append data to input data vector
	readData.insert(readData.end(), tmpReadData.begin(), tmpReadData.end());
	__FE_COUTV__(readData.size());

	size_t readCount = readRegister(ROC_ADDRESS_ACTION_READ_SIZE) &
	                   0x7ff;  //only low 11-bits are size (12 is empty, 14 is full)
	__FE_COUTV__(readCount);

}  //end collectSPIFlashBlockRead()

//==================================================================================================
/// 32 programming words = each pair of programming words contain a programming image
/// index to be written to increasing SPI address starting from 0x0 (ie image 0 descriptor
/// pointer to address 0, image 1 descriptor pointer to address 4…)
///
///
/// The RETURN_STATUS register will return a fail count, ie how many times the image index
/// read back from the SPI is not equal to what was written.
void ROCPolarFireCoreInterface::writeSPIFlashDirectory(
    const std::vector<uint32_t>& imageAddresses, bool waitForDone /* = true */)
{
	__FE_COUTV__(imageAddresses.size());
	if(imageAddresses.size() > 16)
	{
		__FE_SS__
		    << "Illegal number of image address requested for SPI flash directory write: "
		    << imageAddresses.size() << __E__;
		__FE_SS_THROW__;
	}

	std::vector<DTCLib::roc_data_t> commandData = {
	    ROC_ACTION_WRITE_DIR,
	    //1st command word, is a 32-bit parameter passed via the second (16LSB) and third (16MSB) command words
	    DTCLib::roc_data_t(imageAddresses.size()),  //LSBs
	    0,                                          //MSBs
	    //2nd command word, is a 32-bit parameter passed via the fourth (16LSB) and fifth (16MSB) command words
	    0,  //LSBs
	    0   //MSBs
	};
	size_t i = 0;
	for(; i < imageAddresses.size(); ++i)
	{
		commandData.push_back(DTCLib::roc_data_t(imageAddresses[i]));
		commandData.push_back(DTCLib::roc_data_t(imageAddresses[i] >> 16));
	}
	//enforce always 32/2 entries
	for(; i < 16; ++i)
	{
		commandData.push_back(0);  //LSBs
		commandData.push_back(0);  //MSBs
	}
	__FE_COUTV__(StringMacros::vectorToString(commandData));

	if(TTEST(1))
	{
		std::stringstream outss;
		for(auto& val : commandData)
			outss << " 0x" << std::hex << std::setw(4) << std::setfill('0') << val;
		__COUTTV__(outss.str());
	}
	if(!waitForDone)
	{
		if(actionLock_.try_lock())
		{
			__FE_COUTT__ << "Have ROC action lock" << __E__;
			writeBlock(
			    commandData, ROC_ADDRESS_ACTION_COMMAND, false /* incrementAddress */);
			return;
		}
		else
		{
			__FE_SS__ << "Could not get ROC action lock (is there an incomplete action?)!"
			          << __E__;
			__FE_SS_THROW__;
		}
	}

	DTCLib::roc_data_t readStatus;
	{  //start action lock
		std::lock_guard<std::mutex> lock(
		    actionLock_);  // protect/lock this link/ROC from starting more than one action
		__FE_COUTT__ << "Have ROC action lock" << __E__;
		// getDevice()->begin_dcs_transaction(); //block other transactions while getting status
		writeBlock(commandData, ROC_ADDRESS_ACTION_COMMAND, false /* incrementAddress */);

		size_t acceptPolls = 0;
		while(isActionDone())
		{
			if(acceptPolls > 30 * 100 /* 30 seconds */)
			{
				auto reg128 = readRegister(ROC_ADDRESS_ACTION_DONE);
				auto reg132 = readRegister(ROC_ADDRESS_ACTION_STATUS);
				__FE_SS__ << "SPI DIRECTORY WRITE TIMEOUT: phase=command-accepted "
				             "timeout=30s (DONE stuck high) reg128=0x"
				          << std::hex << reg128 << " reg132=0x" << reg132 << __E__;
				__FE_SS_THROW__;
			}
			usleep(1000 * 10 /* 10 ms */);
			++acceptPolls;
		}

		size_t donePolls = 0;
		while(!isActionDone())
		{
			if(donePolls > 30 * 100 /* 30 seconds */)
			{
				auto reg128 = readRegister(ROC_ADDRESS_ACTION_DONE);
				auto reg132 = readRegister(ROC_ADDRESS_ACTION_STATUS);
				__FE_SS__ << "SPI DIRECTORY WRITE TIMEOUT: phase=complete "
				             "timeout=30s reg128=0x"
				          << std::hex << reg128 << " reg132=0x" << reg132 << __E__;
				__FE_SS_THROW__;
			}
			usleep(1000 * 10 /* 10 ms */);
			++donePolls;
		}
		readStatus = readRegister(ROC_ADDRESS_ACTION_STATUS);
		// getDevice()->end_dcs_transaction(); //re-allow other transactions
	}  //end action lock

	if(readStatus)
	{
		__FE_SS__ << "Non-zero status received after SPI flash directory write action: 0x"
		          << std::hex << readStatus << __E__;
		__FE_SS_THROW__;
	}

}  //end writeSPIFlashDirectory()

//==================================================================================================
/// The programming words contain 1kB of data from the bitstream file. Multiple
/// “load of image” commands are needed to pass the full bitstream file, whose
/// name is passes via an OTSDAQ Macromaker function. It is up to the DTC to manage
/// the start writing address for each block. The last block may have Nwords < 512
/// and less programming words.
///
///
/// The RETURN_STATUS register will return a “chunk” fail mask. The 1 kB of data is
/// passed to the Flash SPI in 8 chunks of 128 bit each. After each chunk, the SPI is
/// read back and compared to the written data. The fail mask will contain 1 in bit(i)
/// is the i-th chunk verification failed. It is up to the DTC to decide whether to
/// resend the whole 1 kB data or 1kB starting from the address where the first failure
/// happened.
///
/// Note: should be called in thread because it could take a long time
void ROCPolarFireCoreInterface::writeSPIFlashBlock(const std::vector<uint16_t>& writeData,
                                                   uint32_t startAddress,
                                                   bool     waitForDone /* = true */)
{
	__FE_COUTV__(writeData.size());
	if(writeData.size() > 512)
	{
		__FE_SS__ << "Illegal number of write words requested for SPI flash write: "
		          << writeData.size() << __E__;
		__FE_SS_THROW__;
	}

	__FE_COUT__ << "startAddress " << startAddress << " 0x" << std::hex << std::setw(8)
	            << std::setfill('0') << startAddress << __E__;

	std::vector<DTCLib::roc_data_t> commandData = {
	    ROC_ACTION_WRITE_SPI,
	    //1st command word, is a 32-bit parameter passed via the second (16LSB) and third (16MSB) command words
	    DTCLib::roc_data_t(startAddress),        //LSBs
	    DTCLib::roc_data_t(startAddress >> 16),  //MSBs
	    //2nd command word, is a 32-bit parameter passed via the fourth (16LSB) and fifth (16MSB) command words
	    DTCLib::roc_data_t(writeData.size()),  //LSBs
	    0                                      //MSBs
	};

	for(size_t i = 0; i < writeData.size(); ++i)
		commandData.push_back(DTCLib::roc_data_t(writeData[i]));

	__FE_COUTTV__(StringMacros::vectorToString(commandData));
	if(TTEST(1))
	{
		std::stringstream outss;
		for(auto& val : commandData)
			outss << " 0x" << std::hex << std::setw(4) << std::setfill('0') << val;
		outss << __E__;
		__FE_COUTTV__(outss.str());
		for(size_t i = commandData.size() - 1; i > 0; --i)
			outss << " 0x" << std::hex << std::setw(4) << std::setfill('0')
			      << commandData[i];
		outss << __E__;
		__FE_COUTTV__(outss.str());
	}
	__FE_COUTV__(commandData.size());

	if(!waitForDone)
	{
		if(actionLock_.try_lock())
		{
			__FE_COUTT__ << "Have ROC action lock" << __E__;
			writeBlock(
			    commandData, ROC_ADDRESS_ACTION_COMMAND, false /* incrementAddress */);
			return;
		}
		else
		{
			__FE_SS__ << "Could not get ROC action lock (is there an incomplete action?)!"
			          << __E__;
			__FE_SS_THROW__;
		}
	}

	DTCLib::roc_data_t readStatus;
	{  //start action lock
		std::lock_guard<std::mutex> lock(
		    actionLock_);  // protect/lock this link/ROC from starting more than one action
		__FE_COUTT__ << "Have ROC action lock" << __E__;
		writeBlock(commandData, ROC_ADDRESS_ACTION_COMMAND, false /* incrementAddress */);

		// Wait for DONE transition: 1 -> 0 -> 1
		// After writeBlock(), reg128 may still be 0x8000 (DONE) from the
		// previous action. We must first wait for DONE to go LOW (command
		// accepted by the processor), then wait for DONE to go HIGH again
		// (command completed).

		// Phase 1: wait for DONE to clear (command accepted)
		{
			size_t i = 0;
			while(isActionDone())
			{
				if(i > 5 * 100 /* 5 seconds */)
				{
					auto reg128 = readRegister(ROC_ADDRESS_ACTION_DONE);
					auto reg132 = readRegister(ROC_ADDRESS_ACTION_STATUS);
					__FE_SS__ << "SPI WRITE TIMEOUT: phase=command-accepted timeout=5s "
					             "(DONE stuck high) reg128=0x"
					          << std::hex << reg128 << " reg132=0x" << reg132 << __E__;
					__FE_SS_THROW__;
				}
				usleep(1000 * 10 /* 10 ms */);
				++i;
			}
		}

		// Phase 2: wait for DONE to set (command completed)
		{
			size_t i = 0;
			while(!isActionDone())
			{
				if(i > 5 * 100 /* 5 seconds */)
				{
					auto reg128 = readRegister(ROC_ADDRESS_ACTION_DONE);
					auto reg132 = readRegister(ROC_ADDRESS_ACTION_STATUS);
					__FE_SS__ << "SPI WRITE TIMEOUT: phase=complete timeout=5s reg128=0x"
					          << std::hex << reg128 << " reg132=0x" << reg132 << __E__;
					__FE_SS_THROW__;
				}
				usleep(1000 * 10 /* 10 ms */);
				++i;
			}
		}

		readStatus = readRegister(ROC_ADDRESS_ACTION_STATUS);
	}  //end action lock

	if(readStatus)
	{
		__FE_SS__ << "Non-zero status received after SPI flash write action: 0x"
		          << std::hex << readStatus << __E__;
		__FE_SS_THROW__;
	}

}  //end writeSPIFlashBlock()

//==================================================================================================
///	Erases in blocks of 64KB (automatically in hardware calculates ceiling)
/// Note: erase does not give status
void ROCPolarFireCoreInterface::eraseSPIFlashBlock(uint32_t eraseSize,
                                                   uint32_t startAddress,
                                                   bool     waitForDone /* = true */)
{
	__FE_COUTV__(eraseSize);
	__FE_COUTV__(startAddress);

	std::vector<DTCLib::roc_data_t> commandData = {
	    ROC_ACTION_ERASE_ADDR,
	    //1st command word, is a 32-bit parameter passed via the second (16LSB) and third (16MSB) command words
	    DTCLib::roc_data_t(startAddress),        //LSBs
	    DTCLib::roc_data_t(startAddress >> 16),  //MSBs
	    //2nd command word, is a 32-bit parameter passed via the fourth (16LSB) and fifth (16MSB) command words
	    DTCLib::roc_data_t(eraseSize),        //LSBs
	    DTCLib::roc_data_t(eraseSize >> 16),  //MSBs
	};

	__FE_COUTTV__(StringMacros::vectorToString(commandData));
	if(TTEST(1))
	{
		std::stringstream outss;
		for(auto& val : commandData)
			outss << " 0x" << std::hex << std::setw(4) << std::setfill('0') << val;
		outss << __E__;
		__FE_COUTTV__(outss.str());
	}
	if(!waitForDone)
	{
		if(actionLock_.try_lock())
		{
			__FE_COUTT__ << "Have ROC action lock" << __E__;
			writeBlock(
			    commandData, ROC_ADDRESS_ACTION_COMMAND, false /* incrementAddress */);
			return;
		}
		else
		{
			__FE_SS__ << "Could not get ROC action lock (is there an incomplete action?)!"
			          << __E__;
			__FE_SS_THROW__;
		}
	}

	// DTCLib::roc_data_t readStatus;
	{  //start action lock
		std::lock_guard<std::mutex> lock(
		    actionLock_);  // protect/lock this link/ROC from starting more than one action
		__FE_COUTT__ << "Have ROC action lock" << __E__;
		writeBlock(commandData, ROC_ADDRESS_ACTION_COMMAND, false /* incrementAddress */);

		// Phase 1: wait for DONE to clear (command accepted)
		{
			size_t i = 0;
			while(isActionDone())
			{
				if(i > 5 * 100 /* 5 seconds */)
				{
					auto reg128 = readRegister(ROC_ADDRESS_ACTION_DONE);
					auto reg132 = readRegister(ROC_ADDRESS_ACTION_STATUS);
					__FE_SS__ << "SPI ERASE TIMEOUT: phase=command-accepted timeout=5s "
					             "(DONE stuck high) reg128=0x"
					          << std::hex << reg128 << " reg132=0x" << reg132 << __E__;
					__FE_SS_THROW__;
				}
				usleep(1000 * 10 /* 10 ms */);
				++i;
			}
		}

		// Phase 2: wait for DONE to set (erase completed)
		{
			size_t i = 0;
			while(!isActionDone())
			{
				if(i > 180 * 100 /* 180 seconds */)
				{
					auto reg128 = readRegister(ROC_ADDRESS_ACTION_DONE);
					auto reg132 = readRegister(ROC_ADDRESS_ACTION_STATUS);
					__FE_SS__
					    << "SPI ERASE TIMEOUT: phase=complete timeout=180s reg128=0x"
					    << std::hex << reg128 << " reg132=0x" << reg132 << __E__;
					__FE_SS_THROW__;
				}
				usleep(1000 * 10 /* 10 ms */);
				++i;
			}
		}

		// readStatus = readRegister(ROC_ADDRESS_ACTION_STATUS);
	}  //end action lock

	// __FE_COUTV__(readStatus);
	// if(readStatus)
	// {
	// 	__FE_SS__ << "Non-zero status received after SPI flash erase action: 0x" << std::hex << readStatus << __E__;
	// 	__FE_SS_THROW__;
	// }

}  //end eraseSPIFlashBlock()

//==================================================================================================
/// The RETURN_STATUS register contain the error returned by the IAP programming.
/// It is 0x0 if no error.
void ROCPolarFireCoreInterface::programFromSPIByIndex(uint8_t index,
                                                      bool    waitForDone /* = true */)
{
	std::vector<DTCLib::roc_data_t> commandData = {
	    ROC_ACTION_PROG_INDEX,
	    //1st command word, is a 32-bit parameter passed via the second (16LSB) and third (16MSB) command words
	    DTCLib::roc_data_t(index),  //LSBs
	    0,                          //MSBs
	    //2nd command word, is a 32-bit parameter passed via the fourth (16LSB) and fifth (16MSB) command words
	    0,  //LSBs
	    0   //MSBs
	};
	__FE_COUTV__(StringMacros::vectorToString(commandData));

	if(!waitForDone)
	{
		if(actionLock_.try_lock())
		{
			__FE_COUTT__ << "Have ROC action lock" << __E__;
			writeBlock(
			    commandData, ROC_ADDRESS_ACTION_COMMAND, false /* incrementAddress */);
			return;
		}
		else
		{
			__FE_SS__ << "Could not get ROC action lock (is there an incomplete action?)!"
			          << __E__;
			__FE_SS_THROW__;
		}
	}

	DTCLib::roc_data_t readStatus;
	{  //start action lock
		std::lock_guard<std::mutex> lock(
		    actionLock_);  // protect/lock this link/ROC from starting more than one action
		__FE_COUTT__ << "Have ROC action lock" << __E__;
		writeBlock(commandData, ROC_ADDRESS_ACTION_COMMAND, false /* incrementAddress */);

		// Phase 1: wait for DONE to clear (command accepted)
		{
			size_t i = 0;
			while(isActionDone())
			{
				if(i > 5 * 100 /* 5 seconds */)
				{
					__FE_SS__
					    << "Timeout waiting for programFromSPIByIndex command to be "
					       "accepted (DONE stuck high)!"
					    << __E__;
					__FE_SS_THROW__;
				}
				usleep(1000 * 10 /* 10 ms */);
				++i;
			}
		}

		// Phase 2: wait for DONE to set (command completed)
		{
			size_t i = 0;
			while(!isActionDone())
			{
				if(i > 5 * 100 /* 5 seconds */)
				{
					__FE_SS__
					    << "Timeout waiting for action to program from SPI flash by "
					       "index! Check for more info with ROC Read to "
					    << ROC_ADDRESS_ACTION_DONE << __E__;
					__FE_SS_THROW__;
				}
				usleep(1000 * 10 /* 10 ms */);
				++i;
			}
		}
		readStatus = readRegister(ROC_ADDRESS_ACTION_STATUS);
	}  //end action lock

	if(readStatus)
	{
		__FE_SS__ << "Non-zero status received after action to program from SPI flash by "
		             "index: 0x"
		          << std::hex << readStatus << __E__;
		__FE_SS_THROW__;
	}
}  //end programFromSPIByIndex()

//==================================================================================================
/// The RETURN_STATUS register contain the error returned by the IAP programming.
/// It is 0x0 if no error.
void ROCPolarFireCoreInterface::programFromSPIByAddress(uint32_t startAddress,
                                                        bool     waitForDone /* = true */)
{
	std::vector<DTCLib::roc_data_t> commandData = {
	    ROC_ACTION_PROG_ADDR,
	    //1st command word, is a 32-bit parameter passed via the second (16LSB) and third (16MSB) command words
	    DTCLib::roc_data_t(startAddress),        //LSBs
	    DTCLib::roc_data_t(startAddress >> 16),  //MSBs
	    //2nd command word, is a 32-bit parameter passed via the fourth (16LSB) and fifth (16MSB) command words
	    0,  //LSBs
	    0   //MSBs
	};
	__FE_COUTV__(StringMacros::vectorToString(commandData));

	if(!waitForDone)
	{
		if(actionLock_.try_lock())
		{
			__FE_COUTT__ << "Have ROC action lock" << __E__;
			writeBlock(
			    commandData, ROC_ADDRESS_ACTION_COMMAND, false /* incrementAddress */);
			return;
		}
		else
		{
			__FE_SS__ << "Could not get ROC action lock (is there an incomplete action?)!"
			          << __E__;
			__FE_SS_THROW__;
		}
	}

	DTCLib::roc_data_t readStatus;
	{  //start action lock
		std::lock_guard<std::mutex> lock(
		    actionLock_);  // protect/lock this link/ROC from starting more than one action
		__FE_COUTT__ << "Have ROC action lock" << __E__;
		writeBlock(commandData, ROC_ADDRESS_ACTION_COMMAND, false /* incrementAddress */);

		// Phase 1: wait for DONE to clear (command accepted)
		{
			size_t i = 0;
			while(isActionDone())
			{
				if(i > 5 * 100 /* 5 seconds */)
				{
					__FE_SS__
					    << "Timeout waiting for programFromSPIByAddress command to be "
					       "accepted (DONE stuck high)!"
					    << __E__;
					__FE_SS_THROW__;
				}
				usleep(1000 * 10 /* 10 ms */);
				++i;
			}
		}

		// Phase 2: wait for DONE to set (command completed)
		{
			size_t i = 0;
			while(!isActionDone())
			{
				if(i > 5 * 100 /* 5 seconds */)
				{
					__FE_SS__
					    << "Timeout waiting for action to program from SPI flash by "
					       "address! Check for more info with ROC Read to "
					    << ROC_ADDRESS_ACTION_DONE << __E__;
					__FE_SS_THROW__;
				}
				usleep(1000 * 10 /* 10 ms */);
				++i;
			}
		}
		readStatus = readRegister(ROC_ADDRESS_ACTION_STATUS);
	}  //end action lock

	if(readStatus)
	{
		__FE_SS__ << "Non-zero status received after action to program from SPI flash by "
		             "address: 0x"
		          << std::hex << readStatus << __E__;
		__FE_SS_THROW__;
	}
}  //end programFromSPIByAddress()

//==================================================================================================
/// The RETURN_STATUS register contain the error returned by the IAP programming.
/// It is 0x0 if no error.
void ROCPolarFireCoreInterface::autoProgramFromSPI(bool waitForDone /* = true */)
{
	std::vector<DTCLib::roc_data_t> commandData = {
	    ROC_ACTION_PROG_ADDR,
	    //1st command word, is a 32-bit parameter passed via the second (16LSB) and third (16MSB) command words
	    0,  //LSBs
	    0,  //MSBs
	    //2nd command word, is a 32-bit parameter passed via the fourth (16LSB) and fifth (16MSB) command words
	    0,  //LSBs
	    0   //MSBs
	};
	__FE_COUTV__(StringMacros::vectorToString(commandData));

	if(!waitForDone)
	{
		if(actionLock_.try_lock())
		{
			__FE_COUTT__ << "Have ROC action lock" << __E__;
			writeBlock(
			    commandData, ROC_ADDRESS_ACTION_COMMAND, false /* incrementAddress */);
			return;
		}
		else
		{
			__FE_SS__ << "Could not get ROC action lock (is there an incomplete action?)!"
			          << __E__;
			__FE_SS_THROW__;
		}
	}

	DTCLib::roc_data_t readStatus;
	{  //start action lock
		std::lock_guard<std::mutex> lock(
		    actionLock_);  // protect/lock this link/ROC from starting more than one action
		__FE_COUTT__ << "Have ROC action lock" << __E__;
		// getDevice()->begin_dcs_transaction(); //block other transactions while getting status
		writeBlock(commandData, ROC_ADDRESS_ACTION_COMMAND, false /* incrementAddress */);

		//wait for action to complete
		size_t i = 0;
		while(!isActionDone())
		{
			if(i > 5 * 100 /* 5 seconds */)
			{
				// getDevice()->end_dcs_transaction(true /* force */); //re-allow other transactions
				__FE_SS__ << "Timeout waiting for action to program from SPI flash by "
				             "index! Check for more info with ROC Read to "
				          << ROC_ADDRESS_ACTION_DONE << __E__;
				__FE_SS_THROW__;
			}
			usleep(1000 * 10 /* 10 ms */);
		}
		readStatus = readRegister(ROC_ADDRESS_ACTION_STATUS);
		// getDevice()->end_dcs_transaction(); //re-allow other transactions
	}  //end action lock

	if(readStatus)
	{
		__FE_SS__ << "Non-zero status received after action to program from SPI flash by "
		             "index: 0x"
		          << std::hex << readStatus << __E__;
		__FE_SS_THROW__;
	}
}  //end autoProgramFromSPI()

//==================================================================================================
void ROCPolarFireCoreInterface::ReadSPIFlashBlock(__ARGS__)
{
	__FE_COUT__ << "ReadSPIFlashBlock()" << __E__;

	uint32_t startAddress  = __GET_ARG_IN__("Start Address", uint32_t);
	uint32_t numberOfBytes = __GET_ARG_IN__("Number of Bytes", uint32_t);

	__FE_COUTV__(startAddress);
	__FE_COUTV__(numberOfBytes);

	std::vector<uint16_t> readData;
	readSPIFlashBlock(readData, startAddress, numberOfBytes);

	std::stringstream outss;
	outss << "\nRead " << readData.size() * 2 << " bytes (requested " << numberOfBytes
	      << " bytes) from address 0x" << std::hex << startAddress << ":" << __E__;
	for(size_t i = 0; i < readData.size(); i += 2)
	{
		if(i % 16 == 0)
		{
			outss << "0x" << std::hex << std::setw(8) << std::setfill('0')
			      << startAddress + i;
			outss << ": 0x";
		}
		if(i % 2 == 0)
			outss << " ";
		if(i % 4 == 0)
			outss << "    ";
		outss << std::hex << std::setw(4) << std::setfill('0') << readData[i + 1];
		outss << std::hex << std::setw(4) << std::setfill('0') << readData[i];
		if(i % 16 == 14)
			outss << "\n";
	}
	__FE_COUTTV__(StringMacros::vectorToString(readData));

	__SET_ARG_OUT__("Result", outss.str());
	__FE_COUT__ << "end ReadSPIFlashBlock()" << __E__;
}  //end ReadSPIFlashBlock()

//==================================================================================================
void ROCPolarFireCoreInterface::WriteSPIFlashDirectory(__ARGS__)
{
	__FE_COUT__ << "WriteSPIFlashDirectory()" << __E__;
	std::string fullpath = __GET_ARG_IN__("Path to Directory Map file", std::string);
	__FE_COUTV__(fullpath);

	std::vector<uint32_t> writeData;
	{
		__COUTV__(fullpath);

		char       line[100];
		std::FILE* fp = std::fopen(fullpath.c_str(), "r");
		if(!fp)
		{
			__FE_SS__ << "Could not open file at " << fullpath << ". Error: " << errno
			          << " - " << strerror(errno) << __E__;
			__FE_SS_THROW__;
		}

		//each line is 32-bit address
		while(fgets(line, 100, fp))
		{
			uint32_t value = std::stoi(line, nullptr, 16);
			__FE_COUT__ << value << " 0x" << std::hex << value << __E__;
			writeData.push_back(value);
		}
		fclose(fp);
	}

	__FE_COUTV__(StringMacros::vectorToString(writeData));
	writeSPIFlashDirectory(writeData);

	__FE_COUT__ << "end WriteSPIFlashDirectory()" << __E__;
}  //end WriteSPIFlashDirectory()

//==================================================================================================
void ROCPolarFireCoreInterface::EraseSPIFlashBlock(__ARGS__)
{
	__FE_COUT__ << "EraseSPIFlashBlock()" << __E__;

	uint32_t startAddress  = __GET_ARG_IN__("Start Address", uint32_t);
	uint32_t numberOfBytes = __GET_ARG_IN__("Number of Bytes", uint32_t);

	__FE_COUTV__(startAddress);
	__FE_COUTV__(numberOfBytes);

	std::vector<uint16_t> readData;
	eraseSPIFlashBlock(numberOfBytes, startAddress);

	__FE_COUT__ << "end eraseSPIFlashBlock()" << __E__;
}  //end EraseSPIFlashBlock()

//==================================================================================================
void ROCPolarFireCoreInterface::ForceClearActionLock(__ARGS__)
{
	__FE_COUT__ << "ForceClearActionLock()" << __E__;

	actionLock_.unlock();

	__FE_COUT__ << "end ForceClearActionLock()" << __E__;
}  //end EraseSPIFlashBlock()

//==================================================================================================
void ROCPolarFireCoreInterface::forceClearActionLock()
{
	__FE_COUT__ << "forceClearActionLock()" << __E__;

	actionLock_.unlock();

	__FE_COUT__ << "end forceClearActionLock()" << __E__;
}  //end EraseSPIFlashBlock()
