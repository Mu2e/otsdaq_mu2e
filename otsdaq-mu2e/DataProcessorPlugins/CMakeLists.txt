  simple_plugin(DQMMu2eHistoConsumer "processor"
    otsdaq-mu2e_ArtModules
    Configurable
    DataManager
    RootUtilities
    WorkLoopManager
    TableCore
    Macros
    ConfigurationInterface
    ${ROOT_CORE}
    ${ROOT_RIO}
    ${ROOT_HIST}
    ${CETLIB}
    ${ROOT_TREE}
    MF_MessageLogger
    art_Utilities
    art_root_io
    art_root_io_tfile_support
    art_root_io_TFileService_service
 
)

install_headers()
install_source()
