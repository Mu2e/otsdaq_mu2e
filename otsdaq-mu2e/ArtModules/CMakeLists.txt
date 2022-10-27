cet_make_library( 
SOURCE HistoReceiver.cc HistoSender.cc
LIBRARIES PUBLIC
otsdaq::NetworkUtilities
ROOT::Core
otsdaq::Macros
ROOT::RIO
ROOT::Gui
)

install_headers()
install_source()
