
simple_plugin(ROCPolarFireCoreInterface "interface"
  	ROCCore
 )
 simple_plugin(ROCDTCHardwareEmulated "interface"
  	ROCCore
 )
    
simple_plugin(DTCFrontEndInterface "interface"
      CFOandDTCCore
 )
 
simple_plugin(CFOFrontEndInterface "interface"
      CFOandDTCCore
      pcie_linux_kernel_module::CFOInterface
 )
   
   
simple_plugin(CAPTANSignalGenerator "interface"
  otsdaq::FECore
  otsdaq_components::DAQHardware
)

install_headers()
install_source()

 
