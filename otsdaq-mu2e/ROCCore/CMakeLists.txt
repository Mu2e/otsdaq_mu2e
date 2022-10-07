
cet_make_library(LIBRARY_NAME ROCCore
SOURCE ROCCoreVInterface.cc
		 LIBRARIES PUBLIC
		  	otsdaq::FECore
			  pcie_linux_kernel_module::DTCInterface
)

install_headers()
install_source()

 
