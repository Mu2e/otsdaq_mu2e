#include "otsdaq-mu2e/ArtModules/HistoSender.hh"

#include "TRACE/trace.h"
#define TRACE_NAME "HistoSender"

namespace ots
{
namespace
{
constexpr UInt_t kHistoPacketMagic = 0x4f545331;  // 'OTS1'
}

HistoSender::HistoSender(std::string serverHost, int serverPort)
    : sender_(serverHost, serverPort)
{
	sender_.connect();
}

HistoSender::~HistoSender(void)
{
	//sender_.disconnect();
}

void HistoSender::sendHistogram(std::string directoryName, TH1* hist)
{
	TLOG(TLVL_DEBUG) << "Sending single histogram to directory " << directoryName;
	TBufferFile buffer_(TBufferFile::kWrite);
	buffer_.SetWriteMode();
	buffer_.WriteStdString(directoryName);
	buffer_ << kHistoPacketMagic;
	buffer_ << static_cast<UInt_t>(1);
	buffer_.WriteObject(hist);
	sender_.sendPacket(buffer_.Buffer(), buffer_.Length());
	buffer_.Reset();
}

void HistoSender::sendHistograms(std::string directoryName, std::vector<TH1*>& hists)
{
	TLOG(TLVL_DEBUG) << "Sending " << hists.size() << " histograms to directory "
	                 << directoryName;
	TBufferFile buffer_(TBufferFile::kWrite);
	buffer_.SetWriteMode();
	buffer_.WriteStdString(directoryName);
	buffer_ << kHistoPacketMagic;
	buffer_ << static_cast<UInt_t>(hists.size());
	for(size_t i = 0; i < hists.size(); ++i)
	{
		buffer_.WriteObject(hists[i]);
	}
	sender_.sendPacket(buffer_.Buffer(), buffer_.Length());
	buffer_.Reset();
}
void HistoSender::sendHistograms(std::map<std::string, std::vector<TH1*>>& hists)
{
	//    TLOG(TLVL_DEBUG) << "Sending " << hists.size() << " histograms to directory " << directoryName;
	TBufferFile buffer_(TBufferFile::kWrite);
	buffer_.SetWriteMode();
	for(auto hist_iter : hists)
	{
		std::string dirName = hist_iter.first;
		buffer_.WriteStdString(dirName);
		buffer_ << kHistoPacketMagic;
		buffer_ << static_cast<UInt_t>(hist_iter.second.size());
		for(size_t i = 0; i < hist_iter.second.size(); ++i)
		{
			buffer_.WriteObject(hist_iter.second[i]);
		}
	}
	sender_.sendPacket(buffer_.Buffer(), buffer_.Length());
	buffer_.Reset();
}

void HistoSender::sendGraphs(std::map<std::string, std::vector<TGraph*>>& graphs)
{
	TBufferFile buffer_(TBufferFile::kWrite);
	buffer_.SetWriteMode();
	for(auto iter : graphs)
	{
		std::string dirName = iter.first;
		buffer_.WriteStdString(dirName);
		buffer_ << kHistoPacketMagic;
		buffer_ << static_cast<UInt_t>(iter.second.size());
		for(size_t i = 0; i < iter.second.size(); ++i)
		{
			buffer_.WriteObject(iter.second[i]);
		}
	}
	sender_.sendPacket(buffer_.Buffer(), buffer_.Length());
	buffer_.Reset();
}
}  // namespace ots
