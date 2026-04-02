

cet_make_exec(NAME DTCFrontEndInterface_TestMain
  LIBRARIES
	otsdaq::FECore
    otsdaq::Macros
	otsdaq-mu2e::DTCFrontEndInterface
)

cet_make_exec(NAME CFOFrontEndInterface_TestMain
  LIBRARIES
	otsdaq::FECore
    otsdaq::Macros
	  otsdaq-mu2e::CFOFrontEndInterface
)

cet_make_exec(NAME DTCFrontEndInterface_ExtCFOMain
  LIBRARIES
	otsdaq::FECore
    otsdaq::Macros
	  otsdaq-mu2e::DTCFrontEndInterface
)


cet_make_exec(NAME DTCFrontEndInterface_HWEventBuilding
  LIBRARIES
	otsdaq::FECore
    otsdaq::Macros
	  otsdaq-mu2e::DTCFrontEndInterface
)

cet_script(ALWAYS_COPY test_DCS.sh) #wrapper around DCS_test to hide printouts
cet_make_exec(NAME DCS_test
  LIBRARIES
	otsdaq::FECore
    otsdaq::Macros
	  otsdaq-mu2e::DTCFrontEndInterface
)

install_headers()
install_source()
