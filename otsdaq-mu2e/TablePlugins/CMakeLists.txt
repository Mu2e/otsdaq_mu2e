#cet_make(LIBRARY_NAME TablePluginDataFormat
#		 LIBRARIES
#		 TableCore
#         ${CETLIB}
#)
  
simple_plugin(DTCInterfaceTable "table"
  TableCore
  ConfigurationInterface
  Macros
  ${CETLIB}
  ${MF_MESSAGELOGGER}
  )
    

  
install_headers()
install_source()
