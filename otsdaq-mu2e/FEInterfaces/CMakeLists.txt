
basic_plugin(ROCPolarFireCoreInterface "interface"
  	ROCCore
 )
 basic_plugin(ROCDTCHardwareEmulated "interface"
  	ROCCore
 )
    
basic_plugin(DTCFrontEndInterface "interface"
      CFOandDTCCore
 )
 
basic_plugin(CFOFrontEndInterface "interface"
      CFOandDTCCore
      pcie_linux_kernel_module::CFOInterface
 )
   
   
basic_plugin(CAPTANSignalGenerator "interface"
  otsdaq::FECore
  otsdaq_components::DAQHardware
)

install_headers()
install_source()

 
