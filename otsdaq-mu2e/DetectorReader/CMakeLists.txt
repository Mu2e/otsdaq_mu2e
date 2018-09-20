include_directories(${MPI_INCLUDE_PATH})


cet_make(LIBRARY_NAME DetectorReader
		 LIBRARIES
		 ${ARTDAQ_APPLICATION}
		 ${ARTDAQ_APPLICATION_MPI2}		 
		 ${ARTDAQ_BUILDINFO}
		 ${ART_PERSISTENCY_PROVENANCE}		 
		 ${Boost_SYSTEM_LIBRARY}
		 ${ART_UTILITIES}
		 ${CETLIB}
		 ${FHICLCPP}
		 MessageFacility
		 ${MF_UTILITIES}
         	 ${XDAQ_BASIC_LIB_LIST}
	         ${ROOT_CORE}
         	 $ENV{XERCES_C_LIB}/libxerces-c.so
         	 SOAPUtilities
		 ConfigurationDataFormats
		 ConfigurationInterface
		 DataManager
		 DAQHardware
		 EventBuilder
         FiniteStateMachine
         SupervisorInfo
         WorkLoopManager
         XmlUtilities
         FECore
         otsdaq-demo_FEInterfaces_FEOtsUDPFSSRInterface_interface
         otsdaq-demo_UserConfigurationDataFormats_FEROtsUDPHardwareConfiguration_configuration
)

#get_cmake_property(_variableNames VARIABLES)
#foreach( _variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

install_headers()
install_source()
