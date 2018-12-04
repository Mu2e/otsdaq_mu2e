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


simple_plugin(ROCCoreInterface "interface"
  	FECore		
  	DTCInterface
 )
 
simple_plugin(ROCCoreVEmulator "interface"
	otsdaq-mu2e_FEInterfaces_ROCCoreInterface_interface
 )
    
simple_plugin(DTCFrontEndInterface "interface"
  	FECore
	DTCInterface  
	otsdaq-mu2e_FEInterfaces_ROCCoreInterface_interface
	otsdaq-mu2e_FEInterfaces_ROCCoreVEmulator_interface
 )
 
simple_plugin(CFOFrontEndInterface "interface"
  	FECore
	DTCInterface
 )
   

install_headers()
install_source()

 
