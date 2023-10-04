#include "otsdaq-mu2e/ROCCore/ROCCoreVInterface.h"
#include "otsdaq/Macros/InterfacePluginMacros.h"

using namespace ots;

//=========================================================================================
ROCCoreVInterface::ROCCoreVInterface(const std::string&       rocUID,
                                     const ConfigurationTree& theXDAQContextConfigTree,
                                     const std::string&       theConfigurationPath)
    : FEVInterface(rocUID, theXDAQContextConfigTree, theConfigurationPath)
    , thisDTC_(0)
    , delay_(getSelfNode().getNode("EventWindowDelayOffset").getValue<unsigned int>())
    , tmo_ms_(100)
    , emulatorWorkLoopPeriod_(1 * 1000 * 1000 /*1 sec in microseconds*/)
    , emulatorWorkLoopExit_(false)
    , emulatorWorkLoopRunning_(false)
{
	__FE_COUT__ << "Constructing..." << __E__;

	INIT_MF("." /*directory used is USER_DATA/LOG/.*/);

	linkID_ =
	    DTCLib::DTC_Link_ID(getSelfNode().getNode("linkID").getValue<unsigned int>());

	__FE_COUT_INFO__ << "ROCCoreVInterface instantiated with link: " << linkID_
	                 << " and EventWindowDelayOffset = " << delay_ << __E__;

	__FE_COUT__ << "Constructed." << __E__;
}  // end constructor()

//==========================================================================================
ROCCoreVInterface::~ROCCoreVInterface(void)
{
	// NOTE:: be careful not to call __FE_COUT__ decoration because it uses the
	// tree and it may already be destructed partially
	// Instead use __GEN_COUT__ which decorates using mfSubject_
	__GEN_COUT__ << "Destructing..." << __E__;

	while(emulatorWorkLoopRunning_)
	{
		__GEN_COUT__ << "Attempting to exit thread..." << __E__;
		emulatorWorkLoopExit_ = true;
		sleep(1);
	}

	__GEN_COUT__ << "Work Loop thread is not running." << __E__;

	__GEN_COUT__ << "Destructed." << __E__;
}  // end destructor()

//==================================================================================================
void ROCCoreVInterface::writeRegister(DTCLib::roc_address_t address,
                                      DTCLib::roc_data_t    writeData)
{
	__FE_COUT__ << "Calling write ROC register: link number " << std::dec << linkID_
	            << ", address = " << address << ", write data = " << writeData << __E__;

	if(emulatorMode_)
	{
		__FE_COUT__ << "Emulator mode write." << __E__;
		std::lock_guard<std::mutex> lock(workLoopMutex_);
		return writeEmulatorRegister(address, writeData);
	}
	else
		return writeROCRegister(address, writeData);

}  // end writeRegister()

//==================================================================================================
DTCLib::roc_data_t ROCCoreVInterface::readRegister(DTCLib::roc_address_t address) try
{
	__FE_COUT__ << "Calling read ROC register: link number = " << std::dec << linkID_
	            << ", address = " << address << __E__;

	if(emulatorMode_)
	{
		__FE_COUT__ << "Emulator mode read." << __E__;
		std::lock_guard<std::mutex> lock(workLoopMutex_);
		return readEmulatorRegister(address);
	}
	else
		return readROCRegister(address);

}  // end readRegister()
catch(...)
{
	__SS__ << "read exception caught: \n\n" << StringMacros::stackTrace() << __E__;
	__FE_COUT_ERR__ << ss.str();
	throw;
}
//
void ROCCoreVInterface::readBlock(std::vector<DTCLib::roc_data_t>& data,
                                  DTCLib::roc_address_t            address,
                                  uint16_t                         wordCount,
                                  bool                             incrementAddress)
{
	__FE_COUT__ << "Calling read ROC block: link number " << std::dec << linkID_
	            << ", address = " << address << ", wordCount = " << wordCount
	            << ", incrementAddress = " << incrementAddress << __E__;

	if(emulatorMode_)
	{
		__FE_COUT__ << "Emulator mode read." << __E__;
		std::lock_guard<std::mutex> lock(workLoopMutex_);
		return readEmulatorBlock(data, address, wordCount, incrementAddress);
	}
	else
		return readROCBlock(data, address, wordCount, incrementAddress);

}  // end readBlock()

////==================================================================================================
// int ROCCoreVInterface::readTimestamp() { return this->readRegister(12); }
//
////==================================================================================================
// void ROCCoreVInterface::writeDelay(unsigned delay)
//{
//	this->writeRegister(21, delay);
//	return;
//}
//
////==================================================================================================
// int ROCCoreVInterface::readDelay() { return this->readRegister(7); }
//
////==================================================================================================
// int ROCCoreVInterface::readDTCLinkLossCounter() { return this->readRegister(8); }
//
////==================================================================================================
// void ROCCoreVInterface::resetDTCLinkLossCounter()
//{
//	this->writeRegister(24, 0x1);
//	return;
//}

//==================================================================================================
void ROCCoreVInterface::highRateCheck(unsigned int loops,
                                      unsigned int baseAddress,
                                      unsigned int correctRegisterValue0,
                                      unsigned int correctRegisterValue1)
{
	__FE_MCOUT__("Starting the high rate check... " << __E__);

	std::thread(
	    [](ROCCoreVInterface* roc,
	       unsigned int       loops,
	       unsigned int       baseAddress,
	       int                correctRegisterValue0,
	       int                correctRegisterValue1) {
		    ROCCoreVInterface::highRateCheckThread(
		        roc, loops, baseAddress, correctRegisterValue0, correctRegisterValue1);
	    },
	    this,
	    loops,
	    baseAddress,
	    correctRegisterValue0,
	    correctRegisterValue1)
	    .detach();

	__FE_MCOUT__("Thread launched..." << __E__);
}

//==================================================================================================
void ROCCoreVInterface::highRateCheckThread(ROCCoreVInterface* roc,
                                            unsigned int       loops,
                                            unsigned int       baseAddress,
                                            unsigned int       correctRegisterValue0,
                                            unsigned int       correctRegisterValue1) try
{
	__MCOUT__(roc->interfaceUID_ << "Starting the high rate check... " << __E__);
	srand(time(NULL));

	int          r;
	unsigned int val;
	// int          loops  = loops;//10 * 1000;
	int cnt    = 0;
	int cnts[] = {0, 0};

	unsigned int correct[] = {correctRegisterValue0,
	                          correctRegisterValue1};  //{4860, 10};

	for(unsigned int i = 0; i < loops; i++)
		for(unsigned int j = 0; j < 2; j++)
		{
			r = rand() % 100;
			__MCOUT__(roc->interfaceUID_ << i << "\t of " << loops << "\tx " << r
			                             << " :\t read register " << baseAddress + j
			                             << __E__);

			for(int rr = 0; rr < r; rr++)
			{
				++cnt;
				++cnts[j];
				val = roc->readRegister(baseAddress + j);
				if(val != correct[j])
				{
					__SS__ << roc->interfaceUID_ << i << "\tx " << r << " :\t "
					       << "read register " << baseAddress + j << ". Mismatch on read "
					       << val << " vs " << correct[j]
					       << ". Read failed on read number " << cnt << __E__;
					__MOUT__ << ss.str();
					__SS_THROW__;
				}
			}
		}

	__MCOUT__(roc->interfaceUID_ << "Completed high rate check. Number of reads: " << cnt
	                             << ", firstRegCnt=" << cnts[0]
	                             << ", secondRegcnt=" << cnts[1] << __E__);
}  // end highRateCheckThread()
catch(...)
{
	__SS__ << roc->interfaceUID_ << "Error caught. Check printouts!" << __E__;
	__MCOUT__(ss.str());
}  // end highRateCheckThread() catch

//==================================================================================================
void ROCCoreVInterface::highRateBlockCheck(unsigned int loops,
                                           unsigned int baseAddress,
                                           unsigned int correctRegisterValue0,
                                           unsigned int correctRegisterValue1)
{
	__FE_MCOUT__("Starting the high rate block check... " << __E__);

	std::thread(
	    [](ROCCoreVInterface* roc,
	       unsigned int       loops,
	       unsigned int       baseAddress,
	       int                correctRegisterValue0,
	       int                correctRegisterValue1) {
		    ROCCoreVInterface::highRateBlockCheckThread(
		        roc, loops, baseAddress, correctRegisterValue0, correctRegisterValue1);
	    },
	    this,
	    loops,
	    baseAddress,
	    correctRegisterValue0,
	    correctRegisterValue1)
	    .detach();

	__FE_MCOUT__("Thread launched..." << __E__);
}

//==================================================================================================
void ROCCoreVInterface::highRateBlockCheckThread(ROCCoreVInterface* roc,
                                                 unsigned int       loops,
                                                 unsigned int       baseAddress,
                                                 unsigned int       correctRegisterValue0,
                                                 unsigned int correctRegisterValue1) try
{
	__MCOUT__(roc->interfaceUID_ << "Starting the high rate block check... " << __E__);
	srand(time(NULL));

	int                   r;
	std::vector<uint16_t> val;
	// int          loops  = loops;//10 * 1000;
	int cnt    = 0;
	int cnts[] = {0, 0};

	unsigned int correct[] = {correctRegisterValue0,
	                          correctRegisterValue1};  //{4860, 10};

	for(unsigned int i = 0; i < loops; i++)
		for(unsigned int j = 0; j < 2; j++)
		{
			r = rand() % 100;
			__MCOUT__(roc->interfaceUID_ << i << "\t of " << loops << "\tx " << r
			                             << " :\t read register " << baseAddress + j
			                             << __E__);

			roc->readBlock(val, baseAddress + j, r, 0);

			if(val.size() != 0)
			{
				for(size_t rr = 0; rr < val.size(); rr++)
				{
					++cnt;
					++cnts[j];

					if(val[rr] != correct[j])
					{
						__SS__ << roc->interfaceUID_ << i << "\tx " << r << " :\t "
						       << "read register " << baseAddress + j
						       << ". Mismatch on read " << val[rr] << " vs " << correct[j]
						       << ". Read failed on read number " << cnt << __E__;
						__MOUT__ << ss.str();
						__SS_THROW__;
					}
				}
			}
			else
			{
				__MCOUT__(roc->interfaceUID_ << i << " buffer size 0! " << __E__);
			}
		}

	__MCOUT__(roc->interfaceUID_
	          << "Completed high rate block check. Number of reads: " << cnt
	          << ", firstRegCnt=" << cnts[0] << ", secondRegcnt=" << cnts[1] << __E__);
}  // end highRateBlockCheckThread()
catch(...)
{
	__SS__ << roc->interfaceUID_ << "Error caught. Check printouts!" << __E__;
	__MCOUT__(ss.str());
}  // end highRateBlockCheckThread() catch

//==================================================================================================
void ROCCoreVInterface::configure(void) try
{
	__FE_COUT__ << "ROCCoreVInterface::configure called " << __E__;
	configureROCDCS(); // needs to be added to derived classes
	//	// this->writeRegister(0,1);
	//	// this->writeRegister(0,0);
	//
	//	// setup needToResetAlignment using rising edge of register 22
	//	// (i.e., force synchronization of ROC clock with 40MHz system clock)
	//	__MCOUT_INFO__("......... setup to synchronize ROC clock with 40 MHz clock edge"
	//	               << __E__);
	//	this->writeRegister(22, 0);
	//	this->writeRegister(22, 1);
	//
	//	__MCOUT_INFO__("........."
	//	               << " Set delay = " << delay_ << ", readback = " <<
	// this->readDelay()
	//	               << " ... ");
	//
	//	this->writeDelay(delay_);
	//
	//	__FE_COUT__ << "Debugging ROC-DCS" << __E__;
	//
	//	unsigned int val;
	//
	//	// read 6 should read back 0x12fc
	//	for(int i = 0; i < 1; i++)
	//	{
	//		val = this->readRegister(6);
	//
	//		//__MCOUT_INFO__(i << " read register 6 = " << val << __E__);
	//		if(val != 4860)
	//		{
	//			__FE_SS__ << "Bad read not 4860! val = " << val << __E__;
	//			__FE_SS_THROW__;
	//		}
	//
	//		val = this->readDelay();
	//		//__MCOUT_INFO__(i << " read register 7 = " << val << __E__);
	//		if(val != delay_)
	//		{
	//			__FE_SS__ << "Bad read not " << delay_ << "! val = " << val << __E__;
	//			__FE_SS_THROW__;
	//		}
	//	}
	//
	//	if(0)  // random intense check
	//	{
	//		highRateCheck();
	//	}
	//
	//	__MCOUT_INFO__("......... reset DTC link loss counter ... ");
	//	resetDTCLinkLossCounter();
}
catch(const std::runtime_error& e)
{
	__FE_MOUT__ << "Error caught: " << e.what() << __E__;
	throw;
}
catch(...)
{
	__FE_SS__ << "Unknown error caught. Check printouts!" << __E__;
	__FE_MOUT__ << ss.str();
	__FE_SS_THROW__;
}

//==================================================================================================
void ROCCoreVInterface::configureROCDCS(void) 
{
	__FE_COUT__ << "configure ROC DCS timeout time " << __E__;

	// check if setting "ROC_DCSTimeoutMs" is present in linked groups in ROCTypeLinkTable->LinkToSubsystemGroupedParametersTable
    bool found = false;
	auto ROCTypeConfig = getSelfNode().getNode("ROCTypeLinkTable");
	if(!ROCTypeConfig.isDisconnected()) 
	{
		if(!ROCTypeConfig.getNode("LinkToSubsystemGroupedParametersTable").isDisconnected())
		{
			auto settingPairs = ROCTypeConfig.getNode("LinkToSubsystemGroupedParametersTable").getChildren();
			for(auto& setting : settingPairs) 
			{
				if(setting.second.getNode("Name").getValue<std::string>() == "ROC_DCSTimeoutMs") 
				{
					if(setting.second.getNode("Status").getValue<bool>())
					{
						tmo_ms_ = setting.second.getNode("Value").getValue<unsigned int>();
						__FE_COUT__ << "DEBUG FOUND 'ROC_DCSTimeoutMs' (" << setting.second.getNode("Name").getValue<std::string>() << ":" << setting.second.getNode("CommentDescription") << ") = " << tmo_ms_ << " " << setting.second.getNode("Value").getValue<unsigned int>() << __E__;
						found = true;
						break;
					}
				}
			} 
		}
	}

	if(found)
		__FE_COUT__ << "configure ROC DCS timeout from settings tmo_ms = " << tmo_ms_ << __E__;
	else
		__FE_COUT__ << "Setting 'ROC_DCSTimeoutMs' not found,  configure ROC DCS timeout with default value tmo_ms = " << tmo_ms_ << __E__;
}


//==============================================================================
void ROCCoreVInterface::halt(void)
{
	if(emulatorWorkLoopRunning_)
	{
		__FE_COUT__ << "Halting and attempting to exit emulator workloop..." << __E__;
		ROCCoreVInterface::emulatorWorkLoopExit_ = true;
	}
}  // end halt()

//==============================================================================
void ROCCoreVInterface::pause(void) {}

//==============================================================================
void ROCCoreVInterface::resume(void) {}

//==============================================================================
void ROCCoreVInterface::start(std::string)  // runNumber)
{
}

//==============================================================================
void ROCCoreVInterface::stop(void) {}

//==============================================================================
bool ROCCoreVInterface::running(void) { return false; }
