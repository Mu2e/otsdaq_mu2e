#ifndef _ots_CFOFrontEndInterface_h_
#define _ots_CFOFrontEndInterface_h_

#include "otsdaq-core/FECore/FEVInterface.h"
#include <string>
#include <map>

#include "dtcInterfaceLib/DTC.h"

#include "mu2e_driver/mu2e_mmap_ioctl.h"	// m_ioc_cmd_t

namespace ots
{

  //class FrontEndHardwareTemplate;
  //class FrontEndFirmwareTemplate;

  class CFOFrontEndInterface: public FEVInterface
  {
  public:
    CFOFrontEndInterface (const std::string& interfaceUID, const ConfigurationTree& theXDAQContextConfigTree, const std::string& interfaceConfigurationPath);

    virtual ~CFOFrontEndInterface(void);

    // state machine
    //----------------
    void configure        (void);
    void halt             (void);
    void pause            (void);
    void resume           (void);
    void start            (std::string runNumber);
    void stop             (void);
    bool running   	  (void);

    // more complicated functions
    //----------------
    float MeasureLoopback(int linkToLoopback);

    // hardware access
    //----------------
    int  universalRead	  (char* address, char* readValue ) override;
    void universalWrite	  (char* address, char* writeValue) override;
    int  registerRead     (int address  );
    int  registerWrite    (int address, int dataToWrite); //return read value after having written dataToWrite

    void readStatus(void);
    int getLinkStatus();

    float delay[8][6][8];

  protected:
    //FrontEndHardwareTemplate* theFrontEndHardware_;
    //FrontEndFirmwareTemplate* theFrontEndFirmware_;

  private:
    char devfile_[11];
    int fd_;
    int dtc_ = -1;
    DTCLib::DTC* thisCFO_;

    m_ioc_reg_access_t reg_access_; 

    int loopback_distribution_[10000];
    unsigned int min_distribution_;
    unsigned int max_distribution_;
    float average_loopback_;
    float rms_loopback_;

  };

}

#endif
