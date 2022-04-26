/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */


 #include "ns3/core-module.h"
 #include "ns3/network-module.h"
 #include "ns3/mobility-module.h"
 #include "ns3/lte-module.h"
 #include "ns3/config-store.h"
 #include "ns3/radio-bearer-stats-calculator.h"
 #include "ns3/lte-global-pathloss-database.h"

 #include <iomanip>
 #include <string>

 #include <ns3/log.h>

 using namespace ns3;

 NS_LOG_COMPONENT_DEFINE ("LenaPathlossTraces");


 int main (int argc, char *argv[])
 {
   double enbDist = 20.0;
   double radius = 10.0;
   uint32_t numUes = 1;


   CommandLine cmd (__FILE__);
   cmd.AddValue ("enbDist", "distance between the two eNBs", enbDist);
   cmd.AddValue ("radius", "the radius of the disc where UEs are placed around an eNB", radius);
   cmd.AddValue ("numUes", "how many UEs are attached to each eNB", numUes);
   cmd.Parse (argc, argv);

   ConfigStore inputConfig;
   inputConfig.ConfigureDefaults ();

   // parse again so you can override default values from the command line
   cmd.Parse (argc, argv);

   // determine the string tag that identifies this simulation run
   // this tag is then appended to all filenames

   UintegerValue runValue;
   GlobalValue::GetValueByName ("RngRun", runValue);

   std::ostringstream tag;
   tag  << "_enbDist" << std::setw (3) << std::setfill ('0') << std::fixed << std::setprecision (0) << enbDist
        << "_radius"  << std::setw (3) << std::setfill ('0') << std::fixed << std::setprecision (0) << radius
        << "_numUes"  << std::setw (3) << std::setfill ('0')  << numUes
        << "_rngRun"  << std::setw (3) << std::setfill ('0')  << runValue.Get () ;

   Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

   /*
   lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::FriisPropagationLossModel"));
   lteHelper->SetPathlossModelAttribute ("Frequency",DoubleValue (5000000000));
   */
   /*
   lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::ItuR1411LosPropagationLossModel"));
   lteHelper->SetPathlossModelAttribute ("Frequency",DoubleValue (5000000000));
   */

   lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::ItuR1411NlosOverRooftopPropagationLossModel"));
   lteHelper->SetPathlossModelAttribute ("Frequency",DoubleValue (5000000000));
   lteHelper->SetPathlossModelAttribute ("Environment",StringValue ("SubUrban"));
   lteHelper->SetPathlossModelAttribute ("CitySize",StringValue ("Large"));
   lteHelper->SetPathlossModelAttribute ("RooftopLevel",DoubleValue (5));
   lteHelper->SetPathlossModelAttribute ("StreetsWidth",DoubleValue (40));


   // Create Nodes: eNodeB and UE
   NodeContainer enbNodes;
   NodeContainer ueNodes1, ueNodes2, ueNodes3;
   enbNodes.Create (3);
   ueNodes1.Create (numUes);
   ueNodes2.Create (numUes);
   ueNodes3.Create (numUes);

   // Position of eNBs
   Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
   positionAlloc->Add (Vector (0.0, 0.0, 10));
   positionAlloc->Add (Vector (enbDist, 0.0, 10));
   positionAlloc->Add (Vector (0.0, enbDist, 10));
   MobilityHelper enbMobility;
   enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
   enbMobility.SetPositionAllocator (positionAlloc);
   enbMobility.Install (enbNodes);

   // Position of UEs attached to eNB 1
   MobilityHelper ue1mobility;
   ue1mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                     "X", DoubleValue (-4),
                                     "Y", DoubleValue (4),
                                     "Z", DoubleValue (1),
                                     "rho", DoubleValue (0.00001));
   ue1mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
   ue1mobility.Install (ueNodes1);

   // Position of UEs attached to eNB 2
   MobilityHelper ue2mobility;
   ue2mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                     "X", DoubleValue (0),
                                     "Y", DoubleValue (0.0),
                                     "Z", DoubleValue (1),
                                     "rho", DoubleValue (0));
   ue2mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
   ue2mobility.Install (ueNodes2);

   // Position of UEs attached to eNB 3
   MobilityHelper ue3mobility;
   ue3mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                     "X", DoubleValue (0),
                                     "Y", DoubleValue (0),
                                     "Z", DoubleValue (1),
                                     "rho", DoubleValue (0.0));
   ue3mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
   ue3mobility.Install (ueNodes3);

   // Create Devices and install them in the Nodes (eNB and UE)
   NetDeviceContainer enbDevs;
   NetDeviceContainer ueDevs1;
   NetDeviceContainer ueDevs2;
   NetDeviceContainer ueDevs3;
   enbDevs = lteHelper->InstallEnbDevice (enbNodes);
   ueDevs1 = lteHelper->InstallUeDevice (ueNodes1);
   ueDevs2 = lteHelper->InstallUeDevice (ueNodes2);
   ueDevs3 = lteHelper->InstallUeDevice (ueNodes3);

   // Attach UEs to a eNB
   lteHelper->Attach (ueDevs1, enbDevs.Get (0));
   lteHelper->Attach (ueDevs2, enbDevs.Get (1));
   lteHelper->Attach (ueDevs3, enbDevs.Get (2));

   // Activate an EPS bearer on all UEs
   enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
   EpsBearer bearer (q);
   lteHelper->ActivateDataRadioBearer (ueDevs1, bearer);
   lteHelper->ActivateDataRadioBearer (ueDevs2, bearer);
   lteHelper->ActivateDataRadioBearer (ueDevs3, bearer);

   Simulator::Stop (Seconds (0.5));

   // Insert RLC Performance Calculator
   std::string dlOutFname = "DlRlcStats";
   dlOutFname.append (tag.str ());
   std::string ulOutFname = "UlRlcStats";
   ulOutFname.append (tag.str ());

   lteHelper->EnableMacTraces ();
   lteHelper->EnableRlcTraces ();



   // keep track of all path loss values in two centralized objects
   DownlinkLteGlobalPathlossDatabase dlPathlossDb;
   UplinkLteGlobalPathlossDatabase ulPathlossDb;
   // we rely on the fact that LteHelper creates the DL channel object first, then the UL channel object,
   // hence the former will have index 0 and the latter 1
   Config::Connect ("/ChannelList/0/PathLoss",
                    MakeCallback (&DownlinkLteGlobalPathlossDatabase::UpdatePathloss, &dlPathlossDb));
   Config::Connect ("/ChannelList/1/PathLoss",
                     MakeCallback (&UplinkLteGlobalPathlossDatabase::UpdatePathloss, &ulPathlossDb));

   Simulator::Run ();

   //std::cout << std::endl << "Friis Propagation Loss Model result" << std::endl;
   //std::cout << std::endl << "Itu R1411 Los Propagation Loss Model result" << std::endl;
   std::cout << std::endl << "Itu R1411 NLos Over Rooftop Propagation Loss Model result" << std::endl;

   // print the pathloss values at the end of the simulation
   std::cout << std::endl << "Downlink pathloss:" << std::endl;
   dlPathlossDb.Print ();
   std::cout << std::endl << "Uplink pathloss:" << std::endl;
   ulPathlossDb.Print ();


   Simulator::Destroy ();


   return 0;
 }
