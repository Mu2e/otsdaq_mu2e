
#simple_plugin(STIBReceiver "generator"
#  artdaq-ots_Overlays
#  otsdaq-demo_Overlays
#  artdaq-ots_Generators_UDPReceiver_generator
#  artdaq_Application
#  artdaq_DAQdata
#  artdaq-core_Utilities
#  artdaq-core_Data
#  ${ART_UTILITIES}
#  ${FHICLCPP}
#  ${CETLIB}
#  )

simple_plugin(DataGenReceiver "generator"
  DataManager
  WorkLoopManager
  artdaq-ots_Overlays
  otsdaq-demo_Overlays
#  artdaq-ots_Generators_UDPReceiver_generator
  artdaq_Application
  artdaq_DAQdata
  artdaq-core_Utilities
  artdaq-core_Data
  ${ART_UTILITIES}
  ${MF_MESSAGELOGGER}
  ${FHICLCPP}
  ${CETLIB}
  )
  
