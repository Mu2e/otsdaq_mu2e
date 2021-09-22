basic_plugin(DBRunInfo "runinfo"
    Configurable	
	Macros
	$ENV{POSTGRESQL_LIBRARIES}/libpq.so
    ConfigurationInterface
    TableCore
	${ROOT_BASIC_LIB_LIST}
	${MF_MESSAGELOGGER}
  )
  
install_headers()
install_source()
