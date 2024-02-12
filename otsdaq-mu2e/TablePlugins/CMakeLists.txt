  
include(otsdaq::table)

cet_build_plugin(DTCInterfaceTable otsdaq::table LIBRARIES REG 
    otsdaq::SlowControlsTableBase
    otsdaq::XDAQContextTable
  )
    

  
install_headers()
install_source()
