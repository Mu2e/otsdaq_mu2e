
cet_make_library(LIBRARY_NAME ROCCore
SOURCE ROCCoreVInterface.cc
		 LIBRARIES PUBLIC
		  	otsdaq::FECore
			  mu2e_pcie_utils::DTCInterface
)

install_headers()
install_source()

 
