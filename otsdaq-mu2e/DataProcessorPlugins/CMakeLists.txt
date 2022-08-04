

include(otsdaq::dataProcessor)

  cet_build_plugin(DQMMu2eHistoConsumer otsdaq::dataProcessor LIBRARIES REG
    otsdaq_mu2e::otsdaq-mu2e_ArtModules 
    art::Framework_Services_Registry
    art_root_io::TFileService_service
    otsdaq::RootUtilities
)

install_headers()
install_source()
