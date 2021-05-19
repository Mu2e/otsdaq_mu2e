  simple_plugin(DQMMu2eHistoConsumer "processor"
    otsdaq-mu2e_ArtModules
    art::Framework_Services_Registry
    artdaq_core::artdaq-core_Utilities
    otsdaq::ConfigurationInterface
    otsdaq::DataManager
    otsdaq::RootUtilities
    art_root_io::TFileService_service
 
)

install_headers()
install_source()
