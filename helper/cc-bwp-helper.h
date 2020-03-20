/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
*   Copyright (c) 2020 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License version 2 as
*   published by the Free Software Foundation;
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*/


#ifndef CC_BWP_HELPER_H
#define CC_BWP_HELPER_H


#include <ns3/mmwave-phy-mac-common.h>
#include <ns3/mmwave-control-messages.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/three-gpp-spectrum-propagation-loss-model.h>
#include <ns3/three-gpp-propagation-loss-model.h>
#include <ns3/spectrum-channel.h>
#include <memory>

namespace ns3 {

/*
 * Upper limits of the number of component carriers used for Carrier
 * Aggregation (CA). In NR, this number depends on the CC contiguousness.
 * Eventually, the number of CCs may also depend on the operation frequency
 */
static const uint8_t MAX_CC_INTRA_BAND = 8;  //!< In NR Rel. 16, up to 8 CCs can be aggregated in the same operation band
static const uint8_t MAX_CC_INTER_BAND = 16; //!< The maximum number of aggregated CCs is 16 in NR Rel. 16 (in more than one operation band)


/**
 * \brief Bandwidth part configuration information
 */
struct BandwidthPartInfo
{
  uint8_t m_bwpId {0};             //!< BWP id
  double m_centralFrequency {0.0};   //!< BWP central frequency
  double m_lowerFrequency {0.0};     //!< BWP lower frequency
  double m_higherFrequency {0.0};    //!< BWP higher frequency
  double m_channelBandwidth {0.0};   //!< BWP bandwidth

  /**
   * \brief Different types for the propagation loss model of this bandwidth parth
   */
  enum Scenario
  {
    RMa,
    UMa,
    UMi_StreetCanyon,
    InH_OfficeOpen,
    InH_OfficeMixed
  } m_scenario {RMa};

  /**
   * \brief Retrieve a string version of the scenario
   * \return the string-fied version of the scenario
   */
  std::string GetScenario () const;

  Ptr<SpectrumChannel> m_channel;            //!< Channel for the Bwp. Leave it nullptr to let the helper fill it
  Ptr<ThreeGppPropagationLossModel> m_propagation;   //!< Propagation model. Leave it nullptr to let the helper fill it
  Ptr<ThreeGppSpectrumPropagationLossModel> m_3gppChannel;   //!< MmWave Channel. Leave it nullptr to let the helper fill it
};

typedef std::unique_ptr<BandwidthPartInfo> BandwidthPartInfoPtr;
typedef std::unique_ptr<const BandwidthPartInfo> BandwidthPartInfoConstPtr;
typedef std::vector<std::reference_wrapper<BandwidthPartInfoPtr>> BandwidthPartInfoPtrVector;

std::ostream & operator<< (std::ostream & os, BandwidthPartInfo const & item);

/**
 * \brief Component carrier configuration element
 */
struct ComponentCarrierInfo
{
  uint8_t m_ccId {0};              //!< CC id
  double m_centralFrequency {0};   //!< BWP central frequency
  double m_lowerFrequency {0};     //!< BWP lower frequency
  double m_higherFrequency {0};    //!< BWP higher frequency
  double m_channelBandwidth {0};   //!< BWP bandwidth

  std::vector<BandwidthPartInfoPtr> m_bwp;  //!< Space for BWP

  /**
   * \brief Adds a bandwidth part configuration to the carrier
   *
   * \param bwp Description of the BWP to be added
   */
  bool AddBwp (BandwidthPartInfoPtr &&bwp);
};

typedef std::unique_ptr<ComponentCarrierInfo> ComponentCarrierInfoPtr;

std::ostream & operator<< (std::ostream & os, ComponentCarrierInfo const & item);


/**
 * \brief Operation band information structure
 *
 * Defines the range of frequencies of an operation band and includes a list of
 * component carriers (CC) and their contiguousness
 */
struct OperationBandInfo
{
  uint8_t m_bandId          {0};    //!< Operation band id
  double m_centralFrequency {0.0};  //!< Operation band central frequency
  double m_lowerFrequency   {0.0};  //!< Operation band lower frequency
  double m_higherFrequency  {0.0};  //!< Operation band higher frequency
  double m_channelBandwidth {0};    //!< Operation band bandwidth

  std::vector<ComponentCarrierInfoPtr> m_cc;

  /**
   * \brief Adds the component carrier definition given as an input reference
   * to the current operation band configuration
   *
   * \param id Where to put the cc
   * \param cc The information of the component carrier to be created
   */
  bool AddCc (ComponentCarrierInfoPtr &&cc);

  /**
   * @brief GetBwpAt
   * @param ccId
   * @param bwpId
   * @return
   */
  BandwidthPartInfoPtr & GetBwpAt (uint32_t ccId, uint32_t bwpId) const;

  BandwidthPartInfoPtrVector GetBwps() const;
};

std::ostream & operator<< (std::ostream & os, OperationBandInfo const & item);

/**
 * \brief Manages the correct creation of operation bands, component carriers and bandwidth parts
 */
class CcBwpCreator
{
public:
  /**
   * \brief Minimum configuration requirements for a OperationBand
   */
  struct SimpleOperationBandConf
  {
    /**
     * \brief Default constructor
     * \param centralFreq Central Frequency
     * \param channelBw Bandwidth
     * \param num Numerology
     * \param sched Scheduler
     */
    SimpleOperationBandConf (double centralFreq = 28e9, double channelBw = 400e6, uint8_t numCc = 1,
                             BandwidthPartInfo::Scenario scenario = BandwidthPartInfo::RMa)
      : m_centralFrequency (centralFreq), m_channelBandwidth (channelBw), m_numCc (numCc), m_scenario (scenario)
    {
    }
    double m_centralFrequency {28e9};   //!< Central Freq
    double m_channelBandwidth        {400e6};  //!< Total Bandwidth of the operation band
    uint8_t m_numCc           {1};      //!< Number of CC in this OpBand
    uint8_t m_numBwp          {1};      //!< Number of BWP per CC
    BandwidthPartInfo::Scenario m_scenario {BandwidthPartInfo::RMa}; //!< Scenario
  };

  /**
   * \brief Creates an operation band by splitting the available bandwidth into
   * equally-large contiguous carriers. Carriers will have common parameters like numerology.
   *
   * \param conf Minimum configuration
   */
  OperationBandInfo CreateOperationBandContiguousCc (const SimpleOperationBandConf &conf);

  /**
   * \brief Creates an operation band with non-contiguous CC.
   *
   * \param conf Minimum configuration for every CC.
   */
  OperationBandInfo CreateOperationBandNonContiguousCc (const std::vector<SimpleOperationBandConf> &configuration);

  /**
   * @brief GetAllBwps
   * @param operationBands
   * @return
   */
  static BandwidthPartInfoPtrVector
  GetAllBwps (const std::vector<std::reference_wrapper<OperationBandInfo> > &operationBands);

  /**
   * \brief Plots the CA/BWP configuration using GNUPLOT. There must be a valid
   * configuration
   *
   * \param filename The path to write the output gnuplot file
   */
  static void PlotNrCaBwpConfiguration (const std::vector<OperationBandInfo> &bands,
                                        const std::string &filename);

  /**
   * \brief Plots the CA/BWP configuration using GNUPLOT. There must be a valid
   * configuration
   *
   * \param filename The path to write the output gnuplot file
   */
  static void PlotLteCaConfiguration (const std::vector<OperationBandInfo> &bands,
                                      const std::string &filename);


private:
  void InitializeCc (std::unique_ptr<ComponentCarrierInfo> &cc,
                     double ccBandwidth, double lowerFreq, uint8_t ccPosition, uint8_t ccId) const;
  void InitializeBwp (std::unique_ptr<BandwidthPartInfo> &bwp,
                      double bwOfBwp, double lowerFreq, uint8_t bwpPosition,
                      uint8_t bwpId) const;
  std::unique_ptr<ComponentCarrierInfo> CreateCc (double ccBandwidth, double lowerFreq,
                                                  uint8_t ccPosition, uint8_t ccId,
                                                  uint8_t bwpNumber, BandwidthPartInfo::Scenario scenario);

  /**
   * \brief Plots a 2D rectangle defined by the input points and places a label
   *
   * \param outFile The output
   * \param index The drawn rectangle id
   * \param xmin The minimum value of the rectangle in the horizontal (x) axis
   * \param xmax The maximum value of the rectangle in the horizontal (x) axis
   * \param ymin The minimum value of the rectangle in the vertical (y) axis
   * \param ymax The minimum value of the rectangle in the vertical (y) axis
   * \param label The text to be printed inside the rectangle
   */
  static void PlotFrequencyBand (std::ofstream &outFile,
                                 uint16_t index,
                                 double xmin,
                                 double xmax,
                                 double ymin,
                                 double ymax,
                                 const std::string &label);

  uint8_t m_operationBandCounter {0};
  uint8_t m_componentCarrierCounter {0};
  uint8_t m_bandwidthPartCounter {0};
};

}

#endif /* CC_BWP_HELPER_H */
