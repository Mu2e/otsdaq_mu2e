#include "otsdaq-mu2e/ArtModules/HistoReceiver.hh"
#include "TBufferFile.h"
                  #include "TH1.h"
#include "otsdaq/Macros/CoutMacros.h"

namespace ots
{
void HistoReceiver::addHistogram(TH1* h, TDirectory* subdir, int mode)
{
	TH1* h_out = (TH1*)subdir->FindObjectAny(h->GetName());  // find in memory
	if(h_out == nullptr)
		subdir->WriteTObject(h);
	else
	{
		// __COUT__ << "[HistoReceiver::" << __func__ << "] Updating histogram "<< h->GetName() << std::endl;
		if(mode == kAdd)
		{
			h_out->Add(h);
		}
		else if(mode == kReplace)
		{
			h_out->Reset();
			h_out->Add(h);
		}
	}
}
void HistoReceiver::addGraph(TGraph* g, TDirectory* subdir, int mode)
{
	TGraph* g_out = (TGraph*)subdir->FindObjectAny(g->GetName());  // find in memory
	if(g_out == nullptr)
		subdir->WriteTObject(g);
	else
	{
		// __COUT__ << "[HistoReceiver::" << __func__ << "] Updating graph "<< h->GetName() << std::endl;
		// FIXME: Add graph replacement/addition logic
		const int npoints = g->GetN();
		g_out->Set(0);
		for(int ipoint = 0; ipoint < npoints; ++ipoint)
			g_out->AddPoint(g->GetX()[ipoint], g->GetY()[ipoint]);
	}
}

void HistoReceiver::addObject(TObject* readObject, TDirectory* subdir, int mode)
{
	if(readObject->InheritsFrom(TH1::Class()))
	{
		TH1* h = (TH1*)readObject;
		addHistogram(h, subdir, mode);
	}
	else if(readObject->InheritsFrom(TGraph::Class()))
	{
		TGraph* g = (TGraph*)readObject;
		addGraph(g, subdir, mode);
	}
	else
	{
		__COUT__ << "[HistoReceiver::" << __func__ << "] Unknown object type with name "
		         << readObject->GetName() << std::endl;
	}
}

void HistoReceiver::readPacket(TDirectory* dir, std::string* buf)
{
	TBufferFile message(TBuffer::kWrite);        // prepare message
	message.WriteBuf(buf->data(), buf->size());  // copy buffer
	message.SetReadMode();
	message.SetBufferOffset(0);  // move pointer

	std::string directoryNameStdString;

	do
	{
		message.ReadStdString(directoryNameStdString);
		// check if the plotting mode was defined
		int mode(kAdd);
		if(directoryNameStdString.find(":") != std::string::npos)
		{
			mode = parseMode(
			    directoryNameStdString.substr(directoryNameStdString.find(":") + 1));
			directoryNameStdString =
			    directoryNameStdString.substr(0, directoryNameStdString.find(":"));
		}
		TString directoryName(directoryNameStdString);

		__COUT__ << "[HistoReceiver::readPacket] Moving in dir: "
		         << directoryNameStdString << std::endl;

		TDirectory* subdir = dir;
		TString     dirStr;
		Ssiz_t      from = 0;
		while(directoryName.Tokenize(dirStr, from, "/"))
		{
			subdir = subdir->mkdir(dirStr.Data(), "", kTRUE);
		}

		//now let's add the plottables
		TObject* readObject = nullptr;
		do
		{
			auto lengthBefore = message.Length();
			readObject        = (TObject*)message.ReadObjectAny(TObject::Class());
			if(readObject != nullptr)
			{
				addObject(readObject, subdir, mode);
			}
			else
			{
				message.SetBufferOffset(lengthBefore);
			}
		} while(readObject != nullptr);
		dir->cd();
		//      __COUT__ << "[HistoReceiver::readPacket] message.BufferSize() - message.Length() = " << (message.BufferSize() - message.Length()) << std::endl;
		// if (message.BufferSize() - message.Length()){
		// 	char nn[10];
		// 	message.ReadArray(&nn[0]);
		// 	nn[9] = '\0';
		// 	__COUT__ << "[HistoReceiver::readPacket] nn[10] = " << std::string(nn) << std::endl;
		// }
	} while(directoryNameStdString != "");
	buf = nullptr;
}

int HistoReceiver::parseMode(std::string mode)
{
	if(mode == "replace")
		return kReplace;
	if(mode == "add")
		return kAdd;
	return kAdd;  //default to adding
}

}  // namespace ots
