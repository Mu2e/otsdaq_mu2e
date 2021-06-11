#include_directories(${MPI_INCLUDE_PATH})
#include_directories(/mu2e/ups/pcie_linux_kernel_module/v1_11_02/include/)
#include_directories(${CMAKE_BINARY_DIR}/artdaq)

#cet_make(LIBRARY_NAME Mu2eFEInterfaceComponents
#		EXCLUDE 
#			CFOFrontEndInterface_interface.cc
#		 	DTCFrontEndInterface_interface.cc
#			SendClockAndEventMarkers.cc		
#			my_cntl.cc
			
#		 LIBRARIES         	
		 	
		 
#		 	artdaq-core_Utilities
#		 	${MF_MESSAGELOGGER}
 #        	${MF_UTILITIES}        
  #       	${CETLIB}
	#		Configurable
	#		DTCInterface

#)


simple_plugin(ROCPolarFireCoreInterface "interface"
  	ROCCore
 )
 simple_plugin(ROCDTCHardwareEmulated "interface"
  	ROCCore
 )
    
simple_plugin(DTCFrontEndInterface "interface"
  	FECore
  	CFOandDTCCore
	pcie_linux_kernel_module::DTCInterface  
	ROCCore
	PluginMakers
	${CETLIB_EXCEPT}
	#otsdaq-mu2e_FEInterfaces_ROCPolarFireCoreInterface_interface
 )
 
simple_plugin(CFOFrontEndInterface "interface"
  	FECore
  	CFOandDTCCore
	pcie_linux_kernel_module::CFOInterface
 )
   
   
simple_plugin(CAPTANSignalGenerator "interface"
  DAQHardware
  FECore
  WorkLoopManager
  Configurable
  ConfigurationInterface
  TableCore
  NetworkUtilities
  Macros
  ${CETLIB}
  ${CETLIB_EXCEPT}
  ${MF_MESSAGELOGGER}
  ${XDAQ_BASIC_LIB_LIST}	 
  ${XDAQ_XERCES-C}
)

install_headers()
install_source()

 
