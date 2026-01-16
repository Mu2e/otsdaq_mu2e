

cet_make_exec(NAME DTCFrontEndInterface_TestMain
  LIBRARIES
	otsdaq::FECore
    otsdaq::Macros
	otsdaq-mu2e::DTCFrontEndInterface
	${XDAQ_BASIC_LIB_LIST}
	config # From XDAQ
	b2innub # From XDAQ
	i2outils # From XDAQ
	executive # From XDAQ
	b2inutils # From XDAQ
	pthttp # From XDAQ
	ptfifo # From XDAQ
	xrelay # From XDAQ
	hyperdaq # From XDAQ
)

cet_make_exec(NAME CFOFrontEndInterface_TestMain
  LIBRARIES
	otsdaq::FECore
    otsdaq::Macros
	  otsdaq-mu2e::CFOFrontEndInterface
	${XDAQ_BASIC_LIB_LIST}
	config # From XDAQ
	b2innub # From XDAQ
	i2outils # From XDAQ
	executive # From XDAQ
	b2inutils # From XDAQ
	pthttp # From XDAQ
	ptfifo # From XDAQ
	xrelay # From XDAQ
	hyperdaq # From XDAQ
)

cet_make_exec(NAME DTCFrontEndInterface_ExtCFOMain
  LIBRARIES
	otsdaq::FECore
    otsdaq::Macros
	  otsdaq-mu2e::DTCFrontEndInterface
	${XDAQ_BASIC_LIB_LIST}
	config # From XDAQ
	b2innub # From XDAQ
	i2outils # From XDAQ
	executive # From XDAQ
	b2inutils # From XDAQ
	pthttp # From XDAQ
	ptfifo # From XDAQ
	xrelay # From XDAQ
	hyperdaq # From XDAQ
)


cet_make_exec(NAME DTCFrontEndInterface_HWEventBuilding
  LIBRARIES
	otsdaq::FECore
    otsdaq::Macros
	  otsdaq-mu2e::DTCFrontEndInterface
	${XDAQ_BASIC_LIB_LIST}
	config # From XDAQ
	b2innub # From XDAQ
	i2outils # From XDAQ
	executive # From XDAQ
	b2inutils # From XDAQ
	pthttp # From XDAQ
	ptfifo # From XDAQ
	xrelay # From XDAQ
	hyperdaq # From XDAQ
)

cet_script(ALWAYS_COPY test_DCS.sh) #wrapper around DCS_test to hide printouts
cet_make_exec(NAME DCS_test
  LIBRARIES
	otsdaq::FECore
    otsdaq::Macros
	  otsdaq-mu2e::DTCFrontEndInterface
	${XDAQ_BASIC_LIB_LIST}
	config # From XDAQ
	b2innub # From XDAQ
	i2outils # From XDAQ
	executive # From XDAQ
	b2inutils # From XDAQ
	pthttp # From XDAQ
	ptfifo # From XDAQ
	xrelay # From XDAQ
	hyperdaq # From XDAQ
)

install_headers()
install_source()
