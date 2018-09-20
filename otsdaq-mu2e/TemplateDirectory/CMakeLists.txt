cet_make(LIBRARY_NAME TemplateDirectory
		 LIBRARIES
		 ${XDAQ_BASIC_LIB_LIST}
		 SOAPUtilities
         $ENV{XERCES_C_LIB}/libxerces-c.so
	 MessageFacility
)
