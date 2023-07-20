
cet_make_library(LIBRARY_NAME CFOandDTCCore
SOURCE CFOandDTCCoreVInterface.cc
		 LIBRARIES PUBLIC
		 otsdaq_mu2e::ROCCore
		 otsdaq::ARTDAQTableBase
)

install_headers()
install_source()

 
