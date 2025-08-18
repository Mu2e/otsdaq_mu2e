#ifndef _ots_ROCTimingPaddlesInterface_h_
#define _ots_ROCTimingPaddlesInterface_h_

#include "otsdaq-mu2e/FEInterfaces/ROCPolarFireCoreInterface.h"

namespace ots
{
class ROCTimingPaddlesInterface : public ROCPolarFireCoreInterface
{
	// clang-format off
  public:
    ROCTimingPaddlesInterface(const std::string&       rocUID,
			    const ConfigurationTree& theXDAQContextConfigTree,
			    const std::string&       interfaceConfigurationPath);

    ~ROCTimingPaddlesInterface(void);



    // state machine
    //----------------
    // state machine
    //----------------
    void 								configure					(void) override;
    //void 								halt						(void) override;
    //void 								pause						(void) override;
    //void 								resume						(void) override;
    //void 								start						(std::string runNumber) override;
    //void 								stop						(void) override;
    bool 								running		                (void) override;

    void Configure					(__ARGS__);
    void ReadMarkerHistograms		(__ARGS__);
    void ResetHistograms            (__ARGS__);
    void Reset                      (__ARGS__);
    void PrepareDataRuns            (__ARGS__);
    void SelectiveReset             (__ARGS__);
    void GetStatus                  (__ARGS__);
    void BERT                       (__ARGS__);
    void ReadRxFIFO                 (__ARGS__);
    void ReadTxFIFO                 (__ARGS__);

    std::string readBuffer(uint32_t loc_addr);
    std::array<std::string, 4> ReadMarkerHistograms(void);
    std::vector<uint16_t> readHistogram(std::vector<DTCLib::roc_data_t>& histAddrs, uint32_t loc_addr, std::stringstream& outss);

  private:

  std::string histogramTemplate3Markers =  R"({
        "data" : [{
                "x": [-2, -1, 0, 1, 2],
				        "y": <LOOPBACK>,
                "type": "bar",
                "name": "Loopback Markers",
                "opacity": 0.75
            },
            {
				        "x": [-2, -1, 0, 1, 2],
                "y": <CLOCK>,
                "type": "bar",
                "name": "Clock Markers",
                "opacity": 0.75
            },
			      {
				        "x": [-2, -1, 0, 1, 2],
                "y": <EVENT>,
                "type": "bar",
                "name": "Event Markers",
                "opacity": 0.75
            }],
        "layout" : {
                "title" : { "text" : <TITLE>},
                "xaxis" : { "title" : {"text" : "Bin"}, "titlefont": { "size" : 10 }, "showticklabels" : true },
                "yaxis" : { "title" : {"text" : "Count"}, "titlefont": { "size" : 10 }, "zeroline" : true }
            }
    })";

    std::string histogramTemplate1Marker =  R"({
        "data" : [{
                "x": [-2, -1, 0, 1, 2],
				        "y": <MARKER-DATA>,
                "type": "bar",
                "name": <MARKER-TYPE>,
                "opacity": 0.75
            }],
        "layout" : {
                "title" : { "text" : <TITLE>},
                "xaxis" : { "title" : {"text" : "Bin"}, "titlefont": { "size" : 10 }, "showticklabels" : true },
                "yaxis" : { "title" : {"text" : "Count"}, "titlefont": { "size" : 10 }, "zeroline" : true }
            }
    })";

	// clang-format on
};

}  // namespace ots

#endif
