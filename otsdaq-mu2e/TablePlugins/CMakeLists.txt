#cet_make(LIBRARY_NAME TablePluginDataFormat
#		 LIBRARIES
#		 TableCore
#         ${CETLIB}
#)
  
simple_plugin(DTCInterfaceTable "table"
  TableCore
  ConfigurationInterface
  ${CETLIB}
  )
    

  
install_headers()
install_source()
