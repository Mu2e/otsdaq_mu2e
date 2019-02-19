#include "otsdaq-mu2e/FEInterfaces/ROCCoreInterface.h"

#include "otsdaq-core/Macros/InterfacePluginMacros.h"

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "FE-ROCCoreInterface"

//=========================================================================================
ROCCoreInterface::ROCCoreInterface(const std::string&       rocUID,
                                   const ConfigurationTree& theXDAQContextConfigTree,
                                   const std::string&       theConfigurationPath)
    : FEVInterface(rocUID, theXDAQContextConfigTree, theConfigurationPath), thisDTC_(0)
{
	INIT_MF("ROCCoreInterface");

	linkID_ =
	    DTCLib::DTC_Link_ID(getSelfNode().getNode("linkID").getValue<unsigned int>());

	delay_ = getSelfNode().getNode("EventWindowDelayOffset").getValue<unsigned int>();

	__MCOUT_INFO__("ROCCoreInterface instantiated with link: "
	               << linkID_ << " and EventWindowDelayOffset = " << delay_ << __E__);
}

//==========================================================================================
ROCCoreInterface::~ROCCoreInterface(void)
{
	// NOTE:: be careful not to call __FE_COUT__ decoration because it uses the
	// tree and it may already be destructed partially
	__COUT__ << FEVInterface::interfaceUID_ << " Destructor" << __E__;
}

//==================================================================================================
void ROCCoreInterface::writeRegister(unsigned address, unsigned data_to_write)
{
	if(emulatorMode_)
	{
		__FE_COUT__ << "Emulator mode write." << __E__;
		return;
	}

	__FE_COUT__ << "Calling write ROC register: link number " << std::dec << linkID_
	            << ", address = " << address << ", write data = " << data_to_write
	            << __E__;

	thisDTC_->WriteROCRegister(linkID_, address, data_to_write);

	return;
}

//==================================================================================================
int ROCCoreInterface::readRegister(unsigned address)
{
	__FE_COUTV__(address);

	if(emulatorMode_)
	{
		__FE_COUT__ << "Emulator mode read." << __E__;
		return -1;
	}

	int read_data = 0;

	try
	{
		__FE_COUT__ << "Calling read ROC register: link number " << std::dec << linkID_
		            << ", address = " << address << __E__;
		read_data = thisDTC_->ReadROCRegister(linkID_, address, 10);
	}
	catch(...)
	{
		__COUT__ << "DTC failed DCS read" << __E__;
		read_data = -999;
	}

	return read_data;
}

//==================================================================================================
int ROCCoreInterface::readTimestamp() { return this->readRegister(12); }

//==================================================================================================
void ROCCoreInterface::writeDelay(unsigned delay)
{
	this->writeRegister(21, delay);
	return;
}

//==================================================================================================
int ROCCoreInterface::readDelay() { return this->readRegister(7); }

//==================================================================================================
int ROCCoreInterface::readDTCLinkLossCounter() { return this->readRegister(8); }

//==================================================================================================
void ROCCoreInterface::resetDTCLinkLossCounter()
{
	this->writeRegister(24, 0x1);
	return;
}

//==================================================================================================
void ROCCoreInterface::highRateCheck(void)
{
	__FE_MCOUT__("Starting the high rate check... " << __E__);

	std::thread([](ROCCoreInterface* roc) { ROCCoreInterface::highRateCheckThread(roc); },
	            this)
	    .detach();

	__FE_MCOUT__("Thread launched..." << __E__);
}

//==================================================================================================
void ROCCoreInterface::highRateCheckThread(ROCCoreInterface* roc) try
{
	__MCOUT__(roc->interfaceUID_ << "Starting the high rate check... " << __E__);
	srand(time(NULL));

	int          r;
	unsigned int val;
	int          loops  = 10 * 1000;
	int          cnt    = 0;
	int          cnts[] = {0, 0};

	unsigned int correct[] = {4860, 10};

	for(int i = 0; i < loops; i++)
		for(int j = 0; j < 2; j++)
		{
			r = rand() % 100;
			__MCOUT__(roc->interfaceUID_ << i << "\t of " << loops << "\tx " << r
			                             << " :\t read register " << 6 + j << __E__);

			for(int rr = 0; rr < r; rr++)
			{
				++cnt;
				++cnts[j];
				val = roc->readRegister(6 + j);
				if(val != correct[j])
				{
					__SS__ << roc->interfaceUID_ << i << "\tx " << r << " :\t "
					       << "read register " << 6 + j << ". Mismatch on read " << val
					       << " vs " << correct[j] << ". Read failed on read number "
					       << cnt << __E__;
					__MOUT__ << ss.str();
					__SS_THROW__;
				}
			}
		}

	__MCOUT__(roc->interfaceUID_ << "Completed high rate check. Number of reads: " << cnt
	                             << ", reg6cnt=" << cnts[0] << ", reg7cnt=" << cnts[1]
	                             << __E__);
}
catch(...)
{
	__SS__ << roc->interfaceUID_ << "Error caught. Check printouts!" << __E__;
	__MCOUT__(ss.str());
	//__FE_SS_THROW__;
}

//==================================================================================================
void ROCCoreInterface::configure(void) try
{
	// __MCOUT_INFO__("......... Clear DCS FIFOs" << __E__);
	// this->writeRegister(0,1);
	// this->writeRegister(0,0);

	// setup needToResetAlignment using rising edge of register 22
	// (i.e., force synchronization of ROC clock with 40MHz system clock)
	__MCOUT_INFO__("......... setup to synchronize ROC clock with 40 MHz clock edge"
	               << __E__);
	this->writeRegister(22, 0);
	this->writeRegister(22, 1);

	__MCOUT_INFO__("........."
	               << " Set delay = " << delay_ << ", readback = " << this->readDelay()
	               << " ... ");

	this->writeDelay(delay_);

	__FE_COUT__ << "Debugging ROC-DCS" << __E__;

	unsigned int val;

	// read 6 should read back 0x12fc
	for(int i = 0; i < 1; i++)
	{
		val = this->readRegister(6);

		//__MCOUT_INFO__(i << " read register 6 = " << val << __E__);
		if(val != 4860)
		{
			__FE_SS__ << "Bad read not 4860! val = " << val << __E__;
			__FE_SS_THROW__;
		}

		val = this->readDelay();
		//__MCOUT_INFO__(i << " read register 7 = " << val << __E__);
		if(val != delay_)
		{
			__FE_SS__ << "Bad read not " << delay_ << "! val = " << val << __E__;
			__FE_SS_THROW__;
		}
	}

	if(0)  // random intense check
	{
		highRateCheck();
	}

	__MCOUT_INFO__("......... reset DTC link loss counter ... ");
	resetDTCLinkLossCounter();
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

//========================================================================================================================
void ROCCoreInterface::halt(void) {}

//========================================================================================================================
void ROCCoreInterface::pause(void) {}

//========================================================================================================================
void ROCCoreInterface::resume(void) {}

//========================================================================================================================
void ROCCoreInterface::start(std::string)  // runNumber)
{
}

//========================================================================================================================
void ROCCoreInterface::stop(void) {}

//========================================================================================================================
bool ROCCoreInterface::running(void) { return false; }

DEFINE_OTS_INTERFACE(ROCCoreInterface)
