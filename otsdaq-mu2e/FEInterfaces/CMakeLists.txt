
include(otsdaq::FEInterface)

cet_make_library(LIBRARY_NAME ROCPolarFireCoreInterface SOURCE ROCPolarFireCoreInterfaceImpl.cc LIBRARIES PUBLIC otsdaq_mu2e::ROCCore )
cet_build_plugin(ROCPolarFireCoreInterface otsdaq::FEInterface LIBRARIES REG otsdaq_mu2e::ROCPolarFireCoreInterface )
 cet_build_plugin(ROCDTCHardwareEmulated otsdaq::FEInterface LIBRARIES REG otsdaq_mu2e::ROCCore
 )
    
cet_build_plugin(DTCFrontEndInterface otsdaq::FEInterface LIBRARIES REG otsdaq_mu2e::CFOandDTCCore
 )
 
cet_build_plugin(CFOFrontEndInterface otsdaq::FEInterface LIBRARIES REG otsdaq_mu2e::CFOandDTCCore mu2e_pcie_utils::CFOInterface
 )
   
   
cet_build_plugin(CAPTANSignalGenerator otsdaq::FEInterface LIBRARIES REG otsdaq_components::DAQHardware
)

install_headers()
install_source()

 
