#include "otsdaq-mu2e/ArtModules/HistoReceiver.hh"
#include "otsdaq/Macros/CoutMacros.h"
#include "TBufferFile.h"
#include "TH1.h"

namespace ots {
  void HistoReceiver::addHistogram(TObject*readObject, TDirectory*subdir){
    //    __MOUT__ << "[HistoReceiver::addHistogram] searching hist: "<< readObject->GetName() << std::endl;
    TH1 *object = (TH1 *)subdir->FindObjectAny(readObject->GetName()); // find in memory
    TLOG(TLVL_DEBUG) << "subdir = "  << subdir->GetName();
    TLOG(TLVL_DEBUG) << "hname  = "  << readObject->GetName();
    TLOG(TLVL_DEBUG) << "object = "  << object;
    if (object == nullptr){
      // __MOUT__ << "[HistoReceiver::addHistogram] saving new hist: "<< readObject->GetName() << std::endl;
      subdir->WriteTObject(readObject);
    }else{
      //      __MOUT__ << "[HistoReceiver::addHistogram] updating hist: "<< object->GetName() << std::endl;
      object->Add((TH1 *)readObject); 
    }
  }
  
  void HistoReceiver::readPacket(TDirectory *dir, std::string* buf){
  
    TBufferFile   message(TBuffer::kWrite);         // prepare message
    message.WriteBuf(buf->data(), buf->size()); // copy buffer
    message.SetReadMode();
    message.SetBufferOffset(0); // move pointer

    std::string directoryNameStdString;
    __MOUT__ << "[HistoReceiver::readPacket] starts..." << std::endl;

    do {
      message.ReadStdString(directoryNameStdString);
      TString directoryName(directoryNameStdString);
      
      __MOUT__ << "[HistoReceiver::readPacket] Moving in dir: "<< directoryNameStdString << std::endl;

      TDirectory* subdir = dir;
      TString     dirStr;
      Ssiz_t      from   = 0;
      while(directoryName.Tokenize(dirStr, from, "/")){
	subdir = subdir->mkdir(dirStr.Data(), "",kTRUE);
      }

      //now let's add the histograms
      TObject *readObject = nullptr;
      do {
	auto lengthBefore = message.Length();
	readObject = (TH1*)message.ReadObjectAny(TH1::Class());
	if (readObject != nullptr) {
	  addHistogram(readObject, subdir);
	}else {
	  message.SetBufferOffset(lengthBefore);
	}
      } while( readObject != nullptr);
      dir->cd();
      __MOUT__ << "[HistoReceiver::readPacket] message.BufferSize() - message.Length() = " << (message.BufferSize() - message.Length()) << std::endl;
      // if (message.BufferSize() - message.Length()){
      // 	char nn[10];
      // 	message.ReadArray(&nn[0]);
      // 	nn[9] = '\0';
      // 	__MOUT__ << "[HistoReceiver::readPacket] nn[10] = " << std::string(nn) << std::endl;
      // }
    }while(directoryNameStdString != "");
  }
}
