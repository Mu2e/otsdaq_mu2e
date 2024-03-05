

cet_make_exec(NAME DTCFrontEndInterface_TestMain
  LIBRARIES
    otsdaq::FECore
	  otsdaq_mu2e::DTCFrontEndInterface
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
