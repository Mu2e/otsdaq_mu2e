art_make( BASENAME_ONLY
 LIBRARY_NAME otsdaq-demo_ArtModules
 SUBDIRS detail
  NO_PLUGINS
  DICT_DICTIONARIES
  ${ART_PERSISTENCY_PROVENANCE}
  ${CANVAS_PERSISTENCY_PROVENANCE}
 LIB_LIBRARIES 
 otsdaq-demo_Overlays 
 artdaq_ArtModules
  art_Framework_Core
  art_Framework_IO_Sources
  art_Framework_Principal
  canvas_Persistency_Common
  art_Persistency_Provenance
  canvas_Persistency_Provenance
 canvas_Utilities
  MF_MessageLogger
  ${ROOT_CINTEX}
  ${ROOT_RIO}
  ${ROOT_CORE}
  ${ROOT_NET}
  ${ROOT_REFLEX}
  cetlib
)


simple_plugin(WFViewer "module" 
	otsdaq-demo_Overlays 
	${ROOT_GPAD}
	${ROOT_HIST} 
	${ROOT_GUI} 
	${ROOT_RIO}
	MessageFacility
	${MF_MESSAGELOGGER}
	${MF_UTILITIES}
)

simple_plugin(OtsInput "source"
  otsdaq-demo_ArtModules
  artdaq_ArtModules
  art_Framework_Core
  art_Framework_IO_Sources
  art_Framework_Principal
  art_Persistency_Provenance
  art_Utilities
  MF_MessageLogger
)

