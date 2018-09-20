#ifndef _ots_ControlsOtsInterface_h
#define _ots_ControlsOtsInterface_h

#include "otsdaq-core/NetworkUtilities/ReceiverSocket.h"  // Make sure this is always first because <sys/types.h> (defined in Socket.h) must be first
#include "otsdaq-core/ControlsCore/ControlsVInterface.h"
#include <string>
#include <array>
using namespace ots;
//{
  
  class ControlsOtsInterface : public ControlsVInterface
  {
    
  public:
    ControlsOtsInterface		(const std::string& interfaceUID, const ConfigurationTree& theXDAQContextConfigTree, const std::string& controlsConfigurationPath);
    ~ControlsOtsInterface  		();				

    
    void initialize        	(                                        );
    void destroy           	(                                        );
    
    std::string getList 	(std::string format                      );
    void subscribe      	(std::string Name                        );
    void subscribeJSON  	(std::string List                        );
    void unsubscribe    	(std::string Name                        );
    std::array<std::string, 4>  getCurrentValue (std::string Name	 );
    std::array<std::string, 9>  getSettings     (std::string Name	 );   
  };
  
//}

#endif
