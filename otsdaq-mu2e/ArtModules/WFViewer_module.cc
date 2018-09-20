#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "canvas/Utilities/InputTag.h"

#include "artdaq-core/Data/Fragments.hh"

#include "otsdaq-demo/Overlays/FragmentType.hh"
#include "otsdaq-demo/Overlays/DataGenFragment.hh"

#include "cetlib/exception.h"

#include "TFile.h"
#include "TRootCanvas.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TAxis.h"
#include "TH1D.h"
#include "TStyle.h"

#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutMacros.h"

#include <numeric>
#include <vector>
#include <functional>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <initializer_list>
#include <limits>

namespace ots {

  class WFViewer : public art::EDAnalyzer {

  public:
    explicit WFViewer (fhicl::ParameterSet const & p);
    virtual ~WFViewer () = default;

    void analyze (art::Event const & e) override;
    void beginRun(art::Run const &) override;
    double calcmean (const float *);

  private:

    std::unique_ptr<TCanvas> canvas_[2];
    std::vector<Double_t> x_;
    int prescale_;
    art::RunNumber_t current_run_;

    std::size_t num_x_plots_;
    std::size_t num_y_plots_;

    std::vector<std::string> fragment_type_labels_;
    std::vector<artdaq::Fragment::fragment_id_t> fragment_ids_;

    std::vector<std::unique_ptr<TGraph>> graphs_;
    std::vector<std::unique_ptr<TH1D>> histograms_;

    std::map<artdaq::Fragment::fragment_id_t, std::size_t> id_to_index_;
    std::string outputFileName_;
    TFile* fFile_;
    bool writeOutput_;    
  };

}

ots::WFViewer::WFViewer (fhicl::ParameterSet const & ps):
  art::EDAnalyzer(ps), 
  prescale_(ps.get<int> ("prescale")), 
  current_run_(0), 
  num_x_plots_(ps.get<std::size_t>("num_x_plots", std::numeric_limits<std::size_t>::max() )),
  num_y_plots_(ps.get<std::size_t>("num_y_plots", std::numeric_limits<std::size_t>::max() )),
  fragment_type_labels_(ps.get<std::vector<std::string>>("fragment_type_labels")),
  fragment_ids_(ps.get<std::vector<artdaq::Fragment::fragment_id_t> >("fragment_ids")),
  graphs_( fragment_ids_.size() ), 
  histograms_( fragment_ids_.size() ),
  outputFileName_(ps.get<std::string>("fileName","otsdaqdemo_onmon.root")),
  writeOutput_(ps.get<bool>("write_to_file", false))
{
   __COUT__ << "WFViewer CONSTRUCTOR BEGIN!!!!" << std::endl;
   prescale_ = 1;
   if (num_x_plots_ == std::numeric_limits<std::size_t>::max() ||
       num_y_plots_ == std::numeric_limits<std::size_t>::max() ) {

     switch ( fragment_ids_.size() ) {
     case 1: num_x_plots_ = num_y_plots_ = 1; break;
     case 2: num_x_plots_ = 2; num_y_plots_ = 1; break;
     case 3:
     case 4: num_x_plots_ = 2; num_y_plots_ = 2; break;
     case 5: 
     case 6:  num_x_plots_ = 3; num_y_plots_ = 2; break;
     case 7:
     case 8: num_x_plots_ = 4; num_y_plots_ = 2; break;
     default: 
       num_x_plots_ = num_y_plots_ = static_cast<std::size_t>( ceil( sqrt( fragment_type_labels_.size() ) ) );
     }

   }

   // id_to_index_ will translate between a fragment's ID and where in
   // the vector of graphs and histograms it's located

   for (std::size_t i_f = 0; i_f < fragment_ids_.size(); ++i_f) {
     id_to_index_[ fragment_ids_[i_f] ] = i_f;
   }


  // Throw out any duplicate fragment_type_labels_ ; in this context we only
  // care about the different types that we plan to encounter, not how
  // many of each there are

  sort( fragment_type_labels_.begin(), fragment_type_labels_.end() );
  fragment_type_labels_.erase( unique( fragment_type_labels_.begin(), fragment_type_labels_.end() ), fragment_type_labels_.end() );

  gStyle->SetOptStat("irm");
  gStyle->SetMarkerStyle(22);
  gStyle->SetMarkerColor(4);

  std::cout << __COUT_HDR_FL__ << "WFViewer CONSTRUCTOR END" << std::endl;
}

double ots::WFViewer::calcmean(const float *data) {
  int ii;
  double mean=0;
  //uint32_t *lp;
  //lp = (uint32_t *)data;
  for(ii=0;ii<10;ii++) {
    //std::cout << __COUT_HDR_FL__ << " DJN+ " <<  data[ii] << std::endl;
    mean+=data[ii];
  }
  mean /= 10;
  std::cout << "WFViewer mean is " << mean <<" data[0] is " << data[0] <<" data[1] is " << data[1] << " 2=" << data[2] << " 3=" << data[3] << std::endl << std::endl;
  std::cout << "WFViewer mean is " << mean <<" data[15997] is " << data[15997] <<" data[15998] is " << data[15998] << " 2=" << data[15999] << " 3=" << data[15999] << std::endl << std::endl;
   //"Hex[0]:" << std::hex << lp[0] << " [1]:" << std::hex << lp[1] << " [2]:" << std::hex << lp[2] << " [3]:" << std::hex << lp[3] << " [4]:" << std::hex << lp[4] << std::endl << std::endl << std::endl;
  return mean;
}

void ots::WFViewer::analyze (art::Event const & e) {

	std::cout << __COUT_HDR_FL__ << "WFViewer Analyzing event " << e.event() << std::endl;
  static std::size_t evt_cntr = -1;
  evt_cntr++;

  // John F., 1/22/14 -- there's probably a more elegant way of
  // collecting fragments of various types using ART interface code;
  // will investigate. Right now, we're actually re-creating the
  // fragments locally

  artdaq::Fragments fragments;

  for (auto label: fragment_type_labels_) {

    art::Handle<artdaq::Fragments> fragments_with_label;

    e.getByLabel ("daq", label, fragments_with_label);
    
    //    for (int i_l = 0; i_l < static_cast<int>(fragments_with_label->size()); ++i_l) {
    //      fragments.emplace_back( (*fragments_with_label)[i_l] );
    //    }

    //std::cout << __COUT_HDR_FL__ << "WFViewer: There are " << (*fragments_with_label).size() << " fragments in this event" << std::endl;

    for (auto frag : *fragments_with_label) { 
      fragments.emplace_back( frag);
    }
  }

  // John F., 1/5/14 

  // Here, we loop over the fragments passed to the analyze
  // function. A warning is flashed if either (A) the fragments aren't
  // all from the same event, or (B) there's an unexpected number of
  // fragments given the number of boardreaders and the number of
  // fragments per board

  // For every Nth event, where N is the "prescale" setting, plot the
  // distribution of ADC counts from each board_id / fragment_id
  // combo. Also, if "digital_sum_only" is set to false in the FHiCL
  // string, then plot, for the Nth event, a graph of the ADC values
  // across all channels in each board_id / fragment_id combo

  artdaq::Fragment::sequence_id_t expected_sequence_id = std::numeric_limits<artdaq::Fragment::sequence_id_t>::max();

  //  for (std::size_t i = 0; i < fragments.size(); ++i) {
  for (const auto& frag : fragments ) {

    // Pointers to the types of fragment overlays WFViewer can handle;
    // only one will be used per fragment, of course

    std::unique_ptr<DataGenFragment> drPtr;
    
    //  const auto& frag( fragments[i] );  // Basically a shorthand

    //    if (i == 0) 
    if (expected_sequence_id == std::numeric_limits<artdaq::Fragment::sequence_id_t>::max()) { 
      expected_sequence_id = frag.sequenceID();
    }

    if (expected_sequence_id != frag.sequenceID()) {
      mf::LogWarning("WFViewer") << "Warning in WFViewer: expected fragment with sequence ID " << expected_sequence_id << ", received one with sequence ID " << frag.sequenceID();
    }
    
    FragmentType fragtype = static_cast<FragmentType>( frag.type() );
    //std::cout << __COUT_HDR_FL__ << "WFViewer: Fragment type is " << fragtype << " (DataGen=" << FragmentType::DataGen << ")" << std::endl;
    // John F., 1/22/14 -- this should definitely be improved; I'm
    // just using the max # of bits per ADC value for a given fragment
    // type as is currently defined for the V172x fragments (as
    // opposed to the Toy fragment, which have this value in their
    // metadata). Since it's not using external variables for this
    // quantity, this would need to be edited should these values
    // change.

    switch ( fragtype ) {

    case FragmentType::DataGen:
    	drPtr.reset( new DataGenFragment(frag ));
      break;
    default: 
        throw cet::exception("Error in WFViewer: unknown fragment type supplied: " + fragmentTypeToString(fragtype));
    }

    artdaq::Fragment::fragment_id_t fragment_id = frag.fragmentID();
    std::size_t ind = id_to_index_[ fragment_id ];


    // If a histogram doesn't exist for this board_id / fragment_id combo, create it

    if (!histograms_[ind]) {

      histograms_[ind] = std::unique_ptr<TH1D>(new TH1D( Form ("Fragment_%d_hist", fragment_id), "Title Dennis", 300, (Double_t)0.0, (Double_t)std::numeric_limits<uint8_t>::max()));

      histograms_[ind]->SetTitle (Form ("Frag %d, Type %s", fragment_id, 
				       fragmentTypeToString( fragtype  ).c_str() ) );
      histograms_[ind]->GetXaxis()->SetTitle("Vector Mean");
    }

    // For every event, fill the histogram (prescale is ignored here)

    // Is there some way to templatize an ART module? If not, we're
    // stuck with this switch code...

    switch ( fragtype ) {

    case FragmentType::DataGen:
      //for (auto val = drPtr->dataBegin(); val <= drPtr->dataEnd(); ++val )
      {
	auto val = drPtr->dataBegin();
	double the_mean = calcmean(val->data);
	std::cout << __COUT_HDR_FL__ << "DJN WFViewer: Putting datapoint " << the_mean << " into histogram" << std::endl;
	mf::LogInfo("WFViewer") << "Putting datapoint " << the_mean << " into histogram";
	//histograms_[ind]->Fill( static_cast<uint8_t>(val->data[0]) );
	histograms_[ind]->Fill( the_mean);
      }
      break;
  
    default: 
      throw cet::exception("Error in WFViewer: unknown fragment type supplied: " + fragmentTypeToString(fragtype));
    }

    if (evt_cntr % prescale_ - 1 && prescale_ > 1) {
      continue;
    }

    // Draw the histogram

    canvas_[0]->cd(ind+1);
    histograms_[ind]->Draw();

    canvas_[0] -> Modified();
    canvas_[0] -> Update();

    if(writeOutput_) {
      canvas_[0]->Write("wf0", TObject::kOverwrite);
      fFile_->Write();
    }

  }
}

void ots::WFViewer::beginRun(art::Run const &e) {
  if (e.run () == current_run_) return;
  current_run_ = e.run ();

  if(writeOutput_) {
    fFile_ = new TFile(outputFileName_.c_str(), "RECREATE");
    fFile_->cd();
  }

  for (int i = 0; i < 2; i++) canvas_[i] = 0;
  for (auto &x: graphs_) x = 0;
  for (auto &x: histograms_) x = 0;

  for (int i = 0; i < 1 ; i++) {
    canvas_[i] = std::unique_ptr<TCanvas>(new TCanvas(Form("wf%d",i)));
    canvas_[i]->Divide (num_x_plots_, num_y_plots_);
    canvas_[i]->Update ();
    ((TRootCanvas*)canvas_[i]->GetCanvasImp ())->DontCallClose ();
  }

  canvas_[0]->SetTitle("ADC Value Distribution");

  if(writeOutput_) {
    canvas_[0]->Write();
  }

}

DEFINE_ART_MODULE(ots::WFViewer)
