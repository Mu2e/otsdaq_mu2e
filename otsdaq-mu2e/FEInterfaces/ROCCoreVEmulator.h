#ifndef _ots_ROCCoreVEmulator_h_
#define _ots_ROCCoreVEmulator_h_

#include "otsdaq-mu2e/FEInterfaces/ROCCoreInterface.h"


namespace ots
{


class ROCCoreVEmulator : public ROCCoreInterface
{

public:
	ROCCoreVEmulator (const std::string& rocUID,
			const ConfigurationTree& theXDAQContextConfigTree,
			const std::string& interfaceConfigurationPath);

	~ROCCoreVEmulator(void);


	//return false when done with workloop
	virtual bool 					emulatorWorkLoop	(void) { __SS__ << "This is an empty emulator! this function should be overridden by the derived class." << __E__; __SS_THROW__; return false;}

	static void 					emulatorThread		(ROCCoreVEmulator* roc)
	{
		roc->workloopRunning_ = true;

		bool stillWorking = true;
		while(!roc->workloopExit_ && stillWorking)
		{
			__COUT__ << "Calling emulator WorkLoop..." << __E__;

			//lockout member variables for the remainder of the scope
			//this guarantees the emulator thread can safely access the members
			//	Note: other functions (e.g. write and read) must also lock for this to work!
			std::lock_guard<std::mutex> lock(roc->workloopMutex_);
			stillWorking = roc->emulatorWorkLoop();
		}
		__COUT__ << "Exited emulator WorkLoop." << __E__;

		roc->workloopRunning_ = false;
	} //end emulatorThread()

protected:
	volatile bool 					workloopExit_;
	volatile bool 					workloopRunning_;

	std::mutex						workloopMutex_;
};

}

#endif
