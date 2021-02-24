#add_subdirectory(UserTableCore)
add_subdirectory(FEInterfaces)
add_subdirectory(CFOandDTCCore)
add_subdirectory(ROCCore)
add_subdirectory(TablePlugins)
add_subdirectory(DataProcessorPlugins)

if( $ENV{OTSDAQ_COMPILE_RUNINFO} MATCHES "1" )
	add_subdirectory(RunInfoPlugins)
endif()


#add_subdirectory(DemoRootUtilities)
#add_subdirectory(InterfaceTest)
#add_subdirectory(ArtModules)
#add_subdirectory(Generators)
#add_subdirectory(Overlays)
#add_subdirectory(ControlsInterfacePlugins)
#add_subdirectory(TemplateDirectory)

