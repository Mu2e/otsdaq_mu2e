include_directories(${MPI_INCLUDE_PATH})
include_directories(${CMAKE_BINARY_DIR}/artdaq)


cet_make(LIBRARY_NAME ROCCore
		 LIBRARIES
		  	FECore		
		  	DTCInterface
)

install_headers()
install_source()

 
