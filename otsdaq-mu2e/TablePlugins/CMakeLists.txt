  
include(otsdaq::table)

cet_build_plugin(DTCInterfaceTable otsdaq::table LIBRARIES REG otsdaq::SlowControlsTableBase
  )
    

  
install_headers()
install_source()
