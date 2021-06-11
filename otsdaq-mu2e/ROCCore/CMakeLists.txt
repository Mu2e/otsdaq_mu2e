include_directories(${MPI_INCLUDE_PATH})
include_directories(${CMAKE_BINARY_DIR}/artdaq)


cet_make(LIBRARY_NAME ROCCore
		 LIBRARIES
		  	FECore		
		  	pcie_linux_kernel_module::DTCInterface
			Configurable
			ConfigurationInterface
			TableCore
			artdaq-core_Utilities
			Macros
			WorkLoopManager
			${XDAQ_BASIC_LIB_LIST}
			${XDAQ_XERCES-C}
			${MF_MESSAGELOGGER}	
)

install_headers()
install_source()

 
