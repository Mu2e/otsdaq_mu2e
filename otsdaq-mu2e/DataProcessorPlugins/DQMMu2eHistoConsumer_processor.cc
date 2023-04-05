#include "otsdaq-mu2e/DataProcessorPlugins/DQMMu2eHistoConsumer.h"
#include "otsdaq/Macros/CoutMacros.h"
#include "otsdaq/Macros/ProcessorPluginMacros.h"
#include "otsdaq/MessageFacility/MessageFacility.h"
#include <TBufferFile.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TH1F.h>
#include <TTree.h>
#include <chrono>
#include <thread>

using namespace ots;

//========================================================================================================================
DQMMu2eHistoConsumer::DQMMu2eHistoConsumer(
    std::string supervisorApplicationUID, std::string bufferUID,
    std::string processorUID, const ConfigurationTree &theXDAQContextConfigTree,
    const std::string &configurationPath)
    : WorkLoop(processorUID),
      DQMHistosConsumerBase(supervisorApplicationUID, bufferUID, processorUID,
                            LowConsumerPriority),
      Configurable(theXDAQContextConfigTree, configurationPath),
      saveFile_(theXDAQContextConfigTree.getNode(configurationPath)
                    .getNode("SaveFile")
                    .getValue<bool>()),
      filePath_(theXDAQContextConfigTree.getNode(configurationPath)
                    .getNode("FilePath")
                    .getValue<std::string>()),
      radixFileName_(theXDAQContextConfigTree.getNode(configurationPath)
                         .getNode("RadixFileName")
                         .getValue<std::string>())

{
  // std::cout << "[In DQMMu2eHistoConsumer () ] Initiating ..." << std::endl;
}

//========================================================================================================================
DQMMu2eHistoConsumer::~DQMMu2eHistoConsumer(void) {
  DQMHistosBase::closeFile();
}
//========================================================================================================================
void DQMMu2eHistoConsumer::startProcessingData(std::string runNumber) {
  // std::cout << __PRETTY_FUNCTION__
  //           << filePath_ + "/" + radixFileName_ + "_Run" + runNumber + ".root"
  //           << std::endl;
  DQMHistosBase::openFile(filePath_ + "/" + radixFileName_ + "_Run" +
                          runNumber + ".root");
  DQMHistosBase::myDirectory_ =
      DQMHistosBase::theFile_->mkdir("Mu2eHistos", "Mu2eHistos");
  DQMHistosBase::myDirectory_->cd();

  // for (int station = 0; station < 1; station++) {
  //   for (int plane = 0; plane < 2; plane++) {
  //     for (int panel = 0; panel < 6; panel++) {
  //       for (int straw = 0; straw < 96; straw++) {
  //         testHistos_.BookHistos(DQMHistosBase::myDirectory_,
  //                                "Pedestal" + std::to_string(station) + " " +
  //                                    std::to_string(plane) + " " +
  //                                    std::to_string(panel) + " " +
  //                                    std::to_string(straw));
  //       }
  //     }
  //   }
  // }

  // for (int station = 0; station < 1; station++) {
  //   for (int plane = 0; plane < 2; plane++) {
  //     for (int panel = 0; panel < 6; panel++) {
  //       testHistos_.BookHistos(DQMHistosBase::myDirectory_,
  //                              "Straw Hits" + std::to_string(station) + " " +
  //                                  std::to_string(plane) + " " +
  //                                  std::to_string(panel) + " " +
  //                                  std::to_string(0));
  //     }
  //   }
  // }

  // testHistos_.BookHistos(DQMHistosBase::myDirectory_, "Pedestal");
  // testHistos_.BookHistos(DQMHistosBase::myDirectory_, "deltaTT");
  // std::cout << __PRETTY_FUNCTION__ << "Starting!" << std::endl;
  DataConsumer::startProcessingData(runNumber);
  // std::cout << __PRETTY_FUNCTION__ << "Started!" << std::endl;
}

//========================================================================================================================
void DQMMu2eHistoConsumer::stopProcessingData(void) {
  // std::cout << "[In DQMMu2eHistoConsumer () ] Stopping ..." << std::endl;

  DataConsumer::stopProcessingData();
  if (saveFile_) {
    // std::cout << "[In DQMMu2eHistoConsumer () ] Saving ..." << std::endl;
    DQMHistosBase::save();
  }
  closeFile();
}

//========================================================================================================================
void DQMMu2eHistoConsumer::pauseProcessingData(void) {
  // std::cout << "[In DQMMu2eHistoConsumer () ] Pausing ..." << std::endl;
  DataConsumer::stopProcessingData();
}

//========================================================================================================================
void DQMMu2eHistoConsumer::resumeProcessingData(void) {
  // std::cout << "[DQMMu2eHistoConsumer::resumeProcessingData] Resuming ..." << std::endl;
  DataConsumer::startProcessingData("");
}

//========================================================================================================================
bool DQMMu2eHistoConsumer::workLoopThread(toolbox::task::WorkLoop *workLoop) {
  // std::cout<<"[In DQMMu2eHistoConsumer () ] CallingFastRead ..."<<std::endl;
  fastRead();
  return WorkLoop::continueWorkLoop_;
}

//========================================================================================================================
void DQMMu2eHistoConsumer::fastRead(void) {

  if (DataConsumer::read(dataP_, headerP_) < 0 || dataP_ == nullptr  || headerP_ == nullptr) // is there something in the buffer?
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 10
    //__CFG_COUT__ << "There is nothing in the buffer" << std::endl;
    return;
  }
  // std::cout << "[DQMMu2eHistoConsumer::fastRead] reading BUFFER..." << std::endl;
  histReceiver_.readPacket(DQMHistosBase::myDirectory_, dataP_);
  
}

DEFINE_OTS_PROCESSOR(DQMMu2eHistoConsumer)
