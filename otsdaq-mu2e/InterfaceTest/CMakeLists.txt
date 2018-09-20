link_directories($ENV{XDAQ_LIB})

cet_make_exec(DemoInterfaceTestMain
  LIBRARIES
		ConfigurationInterface
		DAQHardware
		FECore
		ConfigurationDataFormats
        	WorkLoopManager
		NetworkUtilities
		otsdaq-components_FEInterfaces_FEOtsUDPTemplateInterface_interface
		$ENV{XDAQ_LIB}/libxdaq.so
		$ENV{XDAQ_LIB}/libconfig.so
		$ENV{XDAQ_LIB}/libpeer.so
		$ENV{XDAQ_LIB}/libtoolbox.so
		$ENV{XDAQ_LIB}/liblog4cplus.so
		$ENV{XDAQ_LIB}/libcgicc.so
		$ENV{XDAQ_LIB}/libxcept.so
		$ENV{XDAQ_LIB}/libxoap.so
		$ENV{XDAQ_LIB}/libxdata.so
		$ENV{XDAQ_LIB}/libxgi.so
		$ENV{XDAQ_LIB}/liblogudpappender.so
		$ENV{XDAQ_LIB}/liblogxmlappender.so
		$ENV{XDAQ_LIB}/libmimetic.so
		$ENV{XDAQ_LIB}/libasyncresolv
		$ENV{XDAQ_LIB}/libb2innub.so
		/usr/lib64/libuuid.so

)

install_headers()
install_source()
