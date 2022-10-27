
cet_make_library(LIBRARY_NAME CFOandDTCCore
SOURCE CFOandDTCCoreVInterface.cc
		 LIBRARIES PUBLIC
		 otsdaq_mu2e::ROCCore
)

install_headers()
install_source()

 
