#include "otsdaq-demo/DemoRootUtilities/DemoDQMHistos.h"

#include <iostream>
#include <sstream>
#include <string>

#include <TDirectory.h>
#include <TFile.h>
#include <TH1I.h>

using namespace ots;

//========================================================================================================================
DemoDQMHistos::DemoDQMHistos(void) {}

//========================================================================================================================
DemoDQMHistos::~DemoDQMHistos(void) {}

//========================================================================================================================
void DemoDQMHistos::book(TFile* rootFile)
{
	std::cout << "Booking start!" << std::endl;
	TDirectory* currentDir = rootFile->mkdir("General", "General");
	currentDir->cd();

	sequenceNumbers_ = new TH1I("SequenceNumber", "Sequence Number", 256, 0, 255);
	dataNumbers_     = new TH1I("Data", "Data", 101, 0, 0x400000 * 100);
}

//========================================================================================================================
void DemoDQMHistos::fill(std::string& buffer, std::map<std::string, std::string> header)
{
	std::stringstream  ss;
	unsigned long long dataQW = *((unsigned long long*)&((buffer)[2]));
	{  // print
		ss << "dataP Read: 0x ";
		for(unsigned int i = 0; i < (buffer).size(); ++i)
			ss << std::hex << (int)(((buffer)[i] >> 4) & 0xF)
			   << (int)(((buffer)[i]) & 0xF) << " " << std::dec;
		ss << std::endl;
		std::cout << "\n" << ss.str();

		std::cout << "sequence = " << (int)*((unsigned char*)&((buffer)[1])) << std::endl;

		std::cout << "dataQW = 0x" << std::hex << (dataQW) << " " << std::dec << dataQW
		          << std::endl;
	}

	sequenceNumbers_->Fill((unsigned int)(*((unsigned char*)&((buffer)[1]))));
	dataNumbers_->Fill(dataQW
	                   //*((unsigned long long *)&((*dataP_)[2]))
	);
}

//========================================================================================================================
void DemoDQMHistos::load(std::string fileName)
{
	/*LORE 2016 MUST BE FIXED THIS MONDAY
	      DQMHistosBase::openFile (fileName);
	      numberOfTriggers_ = (TH1I*)theFile_->Get("General/NumberOfTriggers");

	      std::string directory = "Planes";
	      std::stringstream name;
	      for(unsigned int p=0; p<4; p++)
	      {
	              name.str("");
	              name << directory << "/Plane_" << p << "_Occupancy";
	              //FIXME Must organize better all histograms!!!!!
	              //planeOccupancies_.push_back((TH1I*)theFile_->Get(name.str().c_str()));
	      }
	      //canvas_ = (TCanvas*) theFile_->Get("MainDirectory/MainCanvas");
	      //histo1D_ = (TH1F*) theFile_->Get("MainDirectory/Histo1D");
	      //histo2D_ = (TH2F*) theFile_->Get("MainDirectory/Histo2D");
	      //profile_ = (TProfile*) theFile_->Get("MainDirectory/Profile");
	      closeFile();
	      */
}
