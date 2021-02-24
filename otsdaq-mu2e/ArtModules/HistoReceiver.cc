#include "otsdaq-mu2e/ArtModules/HistoReceiver.hh"
#include "otsdaq/Macros/CoutMacros.h"
#include "TBufferFile.h"
#include "TH1.h"

namespace ots {
  void HistoReceiver::readPacket(TDirectory *dir, std::string* buf){
  
    TBufferFile   message(TBuffer::kWrite);         // prepare message
    message.WriteBuf(buf->data(), buf->size()); // copy buffer
    message.SetReadMode();
    message.SetBufferOffset(0); // move pointer

    std::string directoryNameStdString;
    message.ReadStdString(directoryNameStdString);
    TString directoryName(directoryNameStdString);

    __MOUT__ << "[HistoReceiver::readPacket] Moving in dir: "<< directoryNameStdString << std::endl;

    auto subdir = dir->mkdir(directoryName, "", kTRUE );

    //now let's add the histograms
    TObject *readObject = nullptr;
    do {
      readObject = (TH1*)message.ReadObjectAny(TH1::Class());
      if (readObject != nullptr) {
	__MOUT__ << "[HistoReceiver::readPacket] searching hist: "<< readObject->GetName() << std::endl;
	TH1 *object = (TH1 *)subdir->FindObjectAny(readObject->GetName()); // find in memory
        TLOG(TLVL_DEBUG) << "object = " << object;
	if (object == NULL){
	  __MOUT__ << "[HistoReceiver::readPacket] saving new hist: "<< readObject->GetName() << std::endl;
	  subdir->WriteTObject(readObject);
	}else{
	  __MOUT__ << "[HistoReceiver::readPacket] updating hist: "<< object->GetName() << std::endl;
	  object->Add((TH1 *)readObject); 
	}
      }
    } while( readObject != nullptr);

    dir->cd();
  }
}
