include_directories(${MPI_INCLUDE_PATH})
include_directories(${CMAKE_BINARY_DIR}/artdaq)


cet_make(LIBRARY_NAME CFOandDTCCore
		 LIBRARIES
		  	FECore		
		  	DTCInterface
			Configurable
			ConfigurationInterface
			TableCore
			Macros
			WorkLoopManager
			${XDAQ_BASIC_LIB_LIST}
			$ENV{XERCES_C_LIB}/libxerces-c.so
)

install_headers()
install_source()

 
