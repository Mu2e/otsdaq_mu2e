art_make( BASENAME_ONLY
 LIBRARY_NAME otsdaq-mu2e_ArtModules
 SUBDIRS detail
  NO_PLUGINS
  DICT_DICTIONARIES
  ${ART_PERSISTENCY_PROVENANCE}
  ${CANVAS_PERSISTENCY_PROVENANCE}
 LIB_LIBRARIES 
 otsdaq::NetworkUtilities
#  ${ROOT_CINTEX}
  ${ROOT_RIO}
  ${ROOT_CORE}
#  ${ROOT_NET}
#  ${ROOT_REFLEX}
	  ${ROOT_HIST}
MF_MessageLogger
)

install_headers()
install_source()
