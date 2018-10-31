/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *   Copyright (c) 2018 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 *   Author: Biljana Bojovic <bbojovic@cttc.es>

 */

#include "ns3/mmwave-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store.h"
#include "ns3/mmwave-helper.h"
#include "ns3/log.h"
#include "ns3/mmwave-point-to-point-epc-helper.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/eps-bearer-tag.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/abort.h"
#include "ns3/object.h"
#include "ns3/mmwave-mac-scheduler-ns3.h"
#include "ns3/mmwave-mac-scheduler-ofdma.h"
#include "ns3/mmwave-mac-scheduler-ofdma-rr.h"
#include "ns3/mmwave-phy-mac-common.h"
#include "ns3/basic-data-calculators.h"
#include "ns3/antenna-array-3gpp-model.h"

using namespace ns3;


std::string
BuildFileNameString (std::string directoryName, std::string fileName, std::string tag)
{
  std::ostringstream oss;
  oss << directoryName << fileName<< "-" << tag;
  return oss.str ();
}

class Nr3gppIndoorCalibration
{

public:

  void UeReception (RxPacketTraceParams params);
  void UeSnrPerProcessedChunk (double snr);//, Ptr<MobilityModel> mm, uint32_t nodeId);
  void UeRssiPerProcessedChunk (double rssidBm);
  Nr3gppIndoorCalibration ();
  void Run (void);
  ~Nr3gppIndoorCalibration ();
  NodeContainer SelectWellPlacedUes (const NodeContainer ueNodes, const NodeContainer gnbNodes, double min3DDistance, uint32_t numberOfUesToBeSelected);

private:

  Ptr<MinMaxAvgTotalCalculator<double> > m_sinrCell;
  Ptr<MinMaxAvgTotalCalculator<double> > m_mcsCell;
  Ptr<MinMaxAvgTotalCalculator<double> > m_rbNumCell;
  std::ofstream m_outSinrFile;
  std::ofstream m_outSnrFile;
  std::ofstream m_outRssiFile;
  std::ofstream m_outUePositionsFile;
  std::ofstream m_outGnbPositionsFile;

};

void UeReceptionTrace (Nr3gppIndoorCalibration* scenario, RxPacketTraceParams params)
{
  scenario->UeReception(params);
 }

/*void UeSnrPerProcessedChunkTrace (Nr3gppIndoorCalibration* scenario, double snr, Ptr<MobilityModel> mm, uint32_t nodeId)
{
  scenario->UeSnrPerProcessedChunk (snr, mm, nodeId);
}*/

void UeSnrPerProcessedChunkTrace (Nr3gppIndoorCalibration* scenario, double snr)
{
  scenario->UeSnrPerProcessedChunk (snr);
}

void UeRssiPerProcessedChunkTrace (Nr3gppIndoorCalibration* scenario, double rssidBm)
{
  scenario->UeRssiPerProcessedChunk (rssidBm);
}

void
Nr3gppIndoorCalibration::UeReception (RxPacketTraceParams params)
{
  m_sinrCell-> Update (params.m_sinr);
  m_mcsCell->Update (params.m_mcs);
  m_rbNumCell->Update (params.m_rbAssignedNum);
  m_outSinrFile<<params.m_cellId<<params.m_rnti<<"\t"<<10*log10(params.m_sinr)<<std::endl;
}

/*
void
Nr3gppIndoorCalibration::UeSnrPerProcessedChunk (double snr, Ptr<MobilityModel> mm, uint32_t nodeId)
{
  m_outSnrFile<<nodeId<<"\t"<<10*log10(snr)<<"\t"<<mm->GetPosition().x<<"\t"<<mm->GetPosition().y<<"\t"<<mm->GetPosition().z<<""<<std::endl;
}
*/
void
Nr3gppIndoorCalibration::UeSnrPerProcessedChunk (double snr)
{
  m_outSnrFile<<10*log10(snr)<<std::endl;
}


void
Nr3gppIndoorCalibration::UeRssiPerProcessedChunk (double rssidBm)
{
  m_outRssiFile<<rssidBm<<std::endl;
}

Nr3gppIndoorCalibration::Nr3gppIndoorCalibration ()
{
  m_sinrCell = Create<MinMaxAvgTotalCalculator<double> >();
  m_mcsCell = Create<MinMaxAvgTotalCalculator<double> >();
  m_rbNumCell = Create<MinMaxAvgTotalCalculator<double> >();

}

Nr3gppIndoorCalibration::~Nr3gppIndoorCalibration ()
{
  m_outSinrFile.close();
  m_outUePositionsFile.close();
  m_outGnbPositionsFile.close();
}

/*
 * \brief Function selects UE nodes from the list that are placed with a minimum
 * distance from the closes gNB
 * \param ueNodes - container of UE nodes
 * \param gnbNodes - container of gNB nodes
 * \param minimumDistance - the minimum that shall be between UE and gNB
 * \param numberOfUesToBeSelected -the number of UE nodes to be selected from the original container
 */

NodeContainer
Nr3gppIndoorCalibration::SelectWellPlacedUes (const NodeContainer ueNodes, const NodeContainer gnbNodes, double minDistance, uint32_t numberOfUesToBeSelected)
{
  NodeContainer ueNodesFiltered;
  bool correctDistance = true;

  for (NodeContainer::Iterator itUe = ueNodes.Begin(); itUe!=ueNodes.End(); itUe++)
    {
      correctDistance = true;
      Ptr<MobilityModel> ueMm = (*itUe)->GetObject<MobilityModel>();
      Vector uePos = ueMm->GetPosition ();

      for (NodeContainer::Iterator itGnb = gnbNodes.Begin(); itGnb!=gnbNodes.End(); itGnb++)
        {
          Ptr<MobilityModel> gnbMm = (*itGnb)->GetObject<MobilityModel>();
          Vector gnbPos = gnbMm->GetPosition ();
          double x = uePos.x - gnbPos.x;
          double y = uePos.y - gnbPos.y;
          double distance = sqrt (x * x + y * y);
          //double distance3D = ueMm->GetDistanceFrom (gnbMm);

          if (distance < minDistance)
            {
              correctDistance = false;
              //NS_LOG("The UE node "<<(*itUe)->GetId() << " has wrong position, discarded.");
              break;
            }
        }

      if (correctDistance)
        {
          ueNodesFiltered.Add(*itUe);
        }
      if (ueNodesFiltered.GetN() >= numberOfUesToBeSelected)
        {
          break;
        }
    }
  return ueNodesFiltered;
}

void
Nr3gppIndoorCalibration::Run (void)
{
    Time simTime = MilliSeconds (200);
    Time udpAppStartTimeDl = MilliSeconds (100);
    Time udpAppStopTimeDl = MilliSeconds (200);
    uint32_t packetSize = 1000;
    DataRate udpRate = DataRate ("0.2kbps");
    uint16_t ueCount = 240;
    double minDistance = 7.0;
    // BS atnenna height is 3 meters
    double gNbHeight = 3;
    // UE antenna height is 1.5 meters
    double ueHeight = 1.5;

    std::string tag = "conf1";
    Config::SetDefault ("ns3::AntennaArrayModel::AntennaOrientation", EnumValue (AntennaArrayModel::Z0));
    Config::SetDefault ("ns3::MmWave3gppChannel::Speed", DoubleValue (3.0));
    // set the antenna array model type
    Config::SetDefault("ns3::MmWaveHelper::GnbAntennaArrayModelType", TypeIdValue(AntennaArray3gppModel::GetTypeId()));
    Config::SetDefault("ns3::MmWaveHelper::UeAntennaArrayModelType", TypeIdValue(AntennaArrayModel::GetTypeId()));
    Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Scenario", StringValue("InH-OfficeOpen")); // with antenna height of 10 m
    //Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Scenario", StringValue("InH-OfficeMixed")); // with antenna height of 10 m

    //std::string resultsDirectory = "campaigns/3gpp-calibration/results/";
    std::string resultsDirectory = "";
    std::string filenameSinr = BuildFileNameString ( resultsDirectory , "sinrs", tag);
    std::string filenameSnr = BuildFileNameString ( resultsDirectory , "snrs", tag);
    std::string filenameRssi = BuildFileNameString ( resultsDirectory , "rssi", tag);
    std::string filenameUePositions = BuildFileNameString ( resultsDirectory , "3gpp-indoor-ue-positions", tag);
    std::string filenameGnbPositions = BuildFileNameString( resultsDirectory , "3gpp-indoor-gnb-positions", tag);

    m_outSinrFile.open (filenameSinr.c_str (), std::ofstream::out | std::ofstream::app);
    m_outSinrFile.setf (std::ios_base::fixed);

    if(!m_outSinrFile.is_open())
      {
        NS_ABORT_MSG("Can't open file " << filenameSinr);
      }

    m_outSnrFile.open (filenameSnr.c_str (), std::ofstream::out | std::ofstream::app);
    m_outSnrFile.setf (std::ios_base::fixed);

    if(!m_outSnrFile.is_open())
      {
        NS_ABORT_MSG("Can't open file " << filenameSnr);
      }

    m_outRssiFile.open (filenameRssi.c_str (), std::ofstream::out | std::ofstream::app);
    m_outRssiFile.setf (std::ios_base::fixed);

    if(!m_outRssiFile.is_open())
      {
        NS_ABORT_MSG("Can't open file " << filenameRssi);
      }

    m_outUePositionsFile.open (filenameUePositions.c_str (), std::ofstream::out | std::ofstream::app);
    m_outUePositionsFile.setf (std::ios_base::fixed);

    if(!m_outUePositionsFile.is_open())
      {
        NS_ABORT_MSG("Can't open file " << filenameUePositions);
      }

    m_outGnbPositionsFile.open (filenameGnbPositions.c_str (), std::ofstream::out | std::ofstream::app);
    m_outGnbPositionsFile.setf (std::ios_base::fixed);

    if(!m_outGnbPositionsFile.is_open())
      {
        NS_ABORT_MSG("Can't open file " << filenameGnbPositions);
      }

    Config::SetDefault ("ns3::MmWavePhyMacCommon::MacSchedulerType", TypeIdValue (TypeId::LookupByName("ns3::MmWaveMacSchedulerTdmaPF")));

    Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Shadowing", BooleanValue(true));
   // Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(999999999));
   // Config::SetDefault ("ns3::LteRlcUmLowLat::MaxTxBufferSize", UintegerValue(999999999));

    Config::SetDefault ("ns3::MmWave3gppChannel::CellScan", BooleanValue (false));
    Config::SetDefault ("ns3::MmWave3gppChannel::BeamSearchAngleStep", DoubleValue (30.0));

    Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (320));
    // Parameters according to R1-1703534
    // 3GPP TSG RAN WG1 Meetging #88, 2017
    // Evaluation assumptions for Phase 1 NR MIMO system level calibration,
    Config::SetDefault ("ns3::MmWaveEnbPhy::TxPower", DoubleValue(23));
    Config::SetDefault ("ns3::MmWavePhyMacCommon::CenterFreq", DoubleValue(30e9));
    Config::SetDefault ("ns3::MmWavePhyMacCommon::Numerology", UintegerValue(2));
    Config::SetDefault ("ns3::MmWavePhyMacCommon::Bandwidth", DoubleValue(40e6));
    // Shall be 4x8 = 32 antenna elements
    Config::SetDefault("ns3::MmWaveEnbNetDevice::AntennaNumDim1", UintegerValue(4));
    Config::SetDefault("ns3::MmWaveEnbNetDevice::AntennaNumDim2", UintegerValue(8));
    // Shall be 2x4 = 8 antenna elements
    Config::SetDefault("ns3::MmWaveUeNetDevice::AntennaNumDim1", UintegerValue(2));
    Config::SetDefault("ns3::MmWaveUeNetDevice::AntennaNumDim2", UintegerValue(4));
    // UE antenna gain shall be set to 5 dBi
    // gNB noise figure shall be set to 7 dB
    Config::SetDefault("ns3::MmWaveEnbPhy::NoiseFigure", DoubleValue (7));
    // UE noise figure shall be set to 10 dB
    Config::SetDefault("ns3::MmWaveUePhy::NoiseFigure", DoubleValue (10));
    // set LOS,NLOS condition
    Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::ChannelCondition", StringValue("a"));
    // setup the mmWave simulation
    Ptr<MmWaveHelper> mmWaveHelper = CreateObject<MmWaveHelper> ();
    mmWaveHelper->SetAttribute ("PathlossModel", StringValue ("ns3::MmWave3gppPropagationLossModel"));
    mmWaveHelper->SetAttribute ("ChannelModel", StringValue ("ns3::MmWave3gppChannel"));

    Ptr<MmWavePointToPointEpcHelper> epcHelper = CreateObject<MmWavePointToPointEpcHelper> ();
    mmWaveHelper->SetEpcHelper (epcHelper);
    mmWaveHelper->Initialize();

    // create base stations and mobile terminals
    NodeContainer gNbNodes;
    NodeContainer ueNodes;
    MobilityHelper mobility;

    gNbNodes.Create (12);
    ueNodes.Create (ueCount);

    // The indoor-hotspot scenario for the system level calibration Phase 1 - R11700144
    Ptr<ListPositionAllocator> gNbPositionAlloc = CreateObject<ListPositionAllocator> ();

    for (uint8_t j = 0; j < 2; j++)
        {
          for (uint8_t i = 0; i < 6; i++)
            {
              gNbPositionAlloc->Add(Vector( i*20, j*20, gNbHeight));
            }
         }
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator (gNbPositionAlloc);
    mobility.Install (gNbNodes);

    Ptr<RandomBoxPositionAllocator> ueRandomRectPosAlloc = CreateObject<RandomBoxPositionAllocator> ();
    double minBigBoxX = -10.0;
    double minBigBoxY = -15.0;
    double maxBigBoxX = 110.0;
    double maxBigBoxY =  35.0;

    NodeContainer selectedUeNodes;

    for (uint8_t j = 0; j < 2; j++)
      {
        double minSmallBoxY = minBigBoxY + j * (maxBigBoxY-minBigBoxY)/2;

        for (uint8_t i = 0; i < 6; i++)
          {
            double minSmallBoxX = minBigBoxX + i * (maxBigBoxX - minBigBoxX)/6;
            Ptr<UniformRandomVariable> ueRandomVarX = CreateObject<UniformRandomVariable>();
            ueRandomVarX->SetAttribute ("Min", DoubleValue (minSmallBoxX));
            ueRandomVarX->SetAttribute ("Max", DoubleValue (minSmallBoxX + (maxBigBoxX - minBigBoxX)/6 - 0.0001 ));
            ueRandomRectPosAlloc->SetX(ueRandomVarX);
            Ptr<UniformRandomVariable> ueRandomVarY = CreateObject<UniformRandomVariable>();
            ueRandomVarY->SetAttribute ("Min", DoubleValue (minSmallBoxY));
            ueRandomVarY->SetAttribute ("Max", DoubleValue (minSmallBoxY + (maxBigBoxY-minBigBoxY)/2 - 0.0001 ));
            ueRandomRectPosAlloc->SetY(ueRandomVarY);
            Ptr<ConstantRandomVariable> ueRandomVarZ = CreateObject<ConstantRandomVariable>();
            ueRandomVarZ->SetAttribute("Constant", DoubleValue(ueHeight));
            ueRandomRectPosAlloc->SetZ(ueRandomVarZ);


            uint8_t smallBoxIndex = j*6 + i;

            NodeContainer smallBoxCandidateNodes;
            NodeContainer smallBoxGnbNode;

            smallBoxGnbNode.Add(gNbNodes.Get(smallBoxIndex));

            for (uint32_t n = smallBoxIndex*ueCount/12; n < smallBoxIndex*(uint32_t)(ueCount/12) + (uint32_t)(ueCount/12); n++ )
              {
                smallBoxCandidateNodes.Add(ueNodes.Get(n));
              }
            mobility.SetPositionAllocator (ueRandomRectPosAlloc);
            mobility.Install (smallBoxCandidateNodes);
            selectedUeNodes.Add(SelectWellPlacedUes (smallBoxCandidateNodes, smallBoxGnbNode , minDistance, 10));
          }
      }

    //NS_LOG("UE nodes candidate list has: "<<ueNodes.GetN()<<" elements, while selected list has:"<<selectedUeNodes.GetN());
    //mobility.SetPositionAllocator (ueRandomRectPosAlloc);
    //install mmWave net devices
    NetDeviceContainer gNbDevs = mmWaveHelper->InstallEnbDevice (gNbNodes);
    NetDeviceContainer ueNetDevs = mmWaveHelper->InstallUeDevice (selectedUeNodes);

    // create the internet and install the IP stack on the UEs
    // get SGW/PGW and create a single RemoteHost
    Ptr<Node> pgw = epcHelper->GetPgwNode ();
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create (1);
    Ptr<Node> remoteHost = remoteHostContainer.Get (0);
    InternetStackHelper internet;
    internet.Install (remoteHostContainer);
    // connect a remoteHost to pgw. Setup routing too
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
    p2ph.SetDeviceAttribute ("Mtu", UintegerValue (2500));
    p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.000)));
    NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
    // in this container, interface 0 is the pgw, 1 is the remoteHost
    //Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
    remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
    internet.Install (ueNodes);
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueNetDevs));


    // Set the default gateway for the UEs
    for (uint32_t j = 0; j < ueNodes.GetN(); ++j)
      {
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNodes.Get(j)->GetObject<Ipv4> ());
        ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
      }

    // attach UEs to the closest eNB
    mmWaveHelper->AttachToClosestEnb (NetDeviceContainer(ueNetDevs), NetDeviceContainer(gNbDevs));

    // assign IP address to UEs, and install UDP downlink applications
    uint16_t dlPort = 1234;
    ApplicationContainer clientAppsDl;
    ApplicationContainer serverAppsDl;

    Time udpInterval = Time::FromDouble((packetSize*8) / static_cast<double> (udpRate.GetBitRate ()), Time::S);

    UdpServerHelper dlPacketSinkHelper (dlPort);
    serverAppsDl.Add (dlPacketSinkHelper.Install (ueNodes));

    // configure UDP downlink traffic
    for (uint32_t i = 0 ; i < ueNetDevs.GetN(); i ++)
      {
        UdpClientHelper dlClient (ueIpIface.GetAddress (i), dlPort);
        dlClient.SetAttribute ("MaxPackets", UintegerValue(0xFFFFFFFF));
        dlClient.SetAttribute("PacketSize", UintegerValue(packetSize));
        dlClient.SetAttribute ("Interval", TimeValue (udpInterval)); // we try to saturate, we just need to measure during a short time, how much traffic can handle each BWP
        clientAppsDl.Add (dlClient.Install (remoteHost));
      }

    // start UDP server and client apps
    serverAppsDl.Start(udpAppStartTimeDl);
    clientAppsDl.Start(udpAppStartTimeDl);

    serverAppsDl.Stop(udpAppStopTimeDl);
    clientAppsDl.Stop(udpAppStopTimeDl);

    for (uint32_t i = 0 ; i < ueNetDevs.GetN(); i ++)
      {
        Ptr<MmWaveSpectrumPhy > ue1SpectrumPhy = DynamicCast<MmWaveUeNetDevice>
        (ueNetDevs.Get(i))->GetPhy()->GetDlSpectrumPhy();
        ue1SpectrumPhy->TraceConnectWithoutContext("RxPacketTraceUe", MakeBoundCallback(&UeReceptionTrace, this));
        Ptr<mmWaveInterference> ue1SpectrumPhyInterference = ue1SpectrumPhy->GetMmWaveInterference();
        NS_ABORT_IF(!ue1SpectrumPhyInterference);
        ue1SpectrumPhyInterference->TraceConnectWithoutContext("SnrPerProcessedChunk", MakeBoundCallback (&UeSnrPerProcessedChunkTrace, this));
                                                                                                          //ueNetDevs.Get(i)->GetNode()->GetObject<MobilityModel>(),
                                                                                                          //ueNetDevs.Get(i)->GetNode()->GetId()
        ue1SpectrumPhyInterference->TraceConnectWithoutContext("RssiPerProcessedChunk", MakeBoundCallback (&UeRssiPerProcessedChunkTrace, this));
      }

    //mmWaveHelper->EnableTraces();
    Simulator::Stop (simTime);
    Simulator::Run ();

    for (uint j = 0; j < ueNodes.GetN(); j++)
      {
          Vector v = ueNodes.Get(j)->GetObject<MobilityModel>()->GetPosition();
          m_outUePositionsFile<<"\n"<<j<<"\t"<<v.x<<"\t"<<v.y<<"\t"<<v.z<<" ";
      }

    for (uint j = 0; j < gNbNodes.GetN(); j++)
      {
          Vector v = gNbNodes.Get(j)->GetObject<MobilityModel>()->GetPosition();
          m_outGnbPositionsFile<<"\n"<<j<<"\t"<<v.x<<"\t"<<v.y<<"\t"<<v.z<<" ";
      }

    Ptr<UdpServer> serverApp1 = serverAppsDl.Get(0)->GetObject<UdpServer>();
    double throughput1 = (serverApp1->GetReceived() * packetSize * 8)/(udpAppStopTimeDl-udpAppStartTimeDl).GetSeconds();


    std::cout<<"\n UE:  "<<throughput1/1e6<<" Mbps"<<
        "\t Avg.SINR:"<< 10*log10(m_sinrCell->getMean()) << "\t Avg.MCS:"<<m_mcsCell->getMean()<<"\t Avg. RB Num:"<<m_rbNumCell->getMean();

    Simulator::Destroy ();
}


int
main (int argc, char *argv[])
{
  Nr3gppIndoorCalibration indoor;
  indoor.Run();
  return 0;

}
