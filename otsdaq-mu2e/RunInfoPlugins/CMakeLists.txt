include(otsdaq::runInfo)

find_package(nlohmann_json REQUIRED)

include_directories(${PostgreSQL_INCLUDE_DIRS})
cet_build_plugin(DBRunInfo otsdaq::runInfo LIBRARIES REG
     ${PostgreSQL_LIBRARIES}
     nlohmann_json::nlohmann_json
  )

install_headers()
install_source()
