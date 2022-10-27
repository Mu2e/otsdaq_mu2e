include(otsdaq::runInfo)

include_directories(${PostgreSQL_INCLUDE_DIRS})
cet_build_plugin(DBRunInfo otsdaq::runInfo LIBRARIES REG ${PostgreSQL_LIBRARIES}
  )
  
install_headers()
install_source()
