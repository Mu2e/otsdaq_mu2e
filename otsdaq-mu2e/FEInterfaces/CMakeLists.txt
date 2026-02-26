include(otsdaq::FEInterface)

cet_make_library(LIBRARY_NAME ROCPolarFireCoreInterface SOURCE ROCPolarFireCoreInterfaceImpl.cc LIBRARIES PUBLIC otsdaq-mu2e::ROCCore )
cet_build_plugin(ROCPolarFireCoreInterface otsdaq::FEInterface LIBRARIES REG otsdaq-mu2e::ROCPolarFireCoreInterface )
cet_build_plugin(ROCDTCHardwareEmulated otsdaq::FEInterface LIBRARIES REG otsdaq-mu2e::ROCCore )

cet_make_library(LIBRARY_NAME DTCFrontEndInterface SOURCE DTCFrontEndInterfaceImpl.cc
  LIBRARIES PUBLIC
  otsdaq-mu2e::CFOandDTCCore
  ROOT::Hist
  ROOT::Core
  ROOT::RIO
)

cet_build_plugin(DTCFrontEndInterface otsdaq::FEInterface LIBRARIES REG
  otsdaq-mu2e::DTCFrontEndInterface
  ROOT::Hist
  ROOT::Core
  ROOT::RIO
)

cet_make_library(LIBRARY_NAME CFOFrontEndInterface SOURCE CFOFrontEndInterfaceImpl.cc
  LIBRARIES PUBLIC
  otsdaq-mu2e::CFOandDTCCore
  ROOT::Hist
  ROOT::Core
  ROOT::RIO
  ROOT::Tree
)

cet_build_plugin(CFOFrontEndInterface otsdaq::FEInterface LIBRARIES REG otsdaq-mu2e::CFOFrontEndInterface )

cet_build_plugin(CAPTANSignalGenerator otsdaq::FEInterface LIBRARIES REG otsdaq-components::DAQHardware )

install_headers()
install_source()
