
include(otsdaq::FEInterface)

cet_build_plugin(ROCPolarFireCoreInterface otsdaq::FEInterface LIBRARIES REG otsdaq_mu2e::ROCCore
 )
 cet_build_plugin(ROCDTCHardwareEmulated otsdaq::FEInterface LIBRARIES REG otsdaq_mu2e::ROCCore
 )
    
cet_build_plugin(DTCFrontEndInterface otsdaq::FEInterface LIBRARIES REG otsdaq_mu2e::CFOandDTCCore
 )
 
cet_build_plugin(CFOFrontEndInterface otsdaq::FEInterface LIBRARIES REG otsdaq_mu2e::CFOandDTCCore pcie_linux_kernel_module::CFOInterface
 )
   
   
cet_build_plugin(CAPTANSignalGenerator otsdaq::FEInterface LIBRARIES REG otsdaq_components::DAQHardware
)

install_headers()
install_source()

 
