/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <iostream>
#include <fstream>

#include "ns3/ipv4-global-routing-helper.h"

#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/dce-module.h"
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/tap-bridge-module.h"

// Default Network Topology
//
// Number of csma and wifi nodes in each can be increased up to 250
//                          |
//                 Rank 0   |   Rank 1
// -------------------------|----------------------------
//   WLAN 11.11.11.0
//                 
//  ================
//  |    |    |    |   
// n5   n6   n7   n0 ---------------- n1   n2 
//                   LAN 9.9.9.0      |    |    
//                                   ========
//                                     LAN 10.10.10.0

using namespace ns3;


void
CreateFiles ()
{
  FILE *fp = fopen ("files-0/index.html", "wb"); 
  int i;
  for (i = 100; i < 1000;i++)
    {
      fprintf (fp, "%d\n\n", i);
    }
  fclose (fp);
}

NS_LOG_COMPONENT_DEFINE ("wifi_csma_tap");

int 
main (int argc, char *argv[])
{
  bool verbose = true;
  //uint32_t i,j;
  uint32_t nWifi11 = 4;
  uint32_t nCsma10 = 2;
  uint32_t np2p9 = 2;
  bool tracing = true;
  std::string mode = "ConfigureLocal";
  std::string tapName = "thetap";
  //std::string tapName2 = "secondtap";

  mkdir ("files-0",0777);
  CreateFiles ();

  CommandLine cmd;
  cmd.AddValue ("nCsma11", "Number of \"extra\" CSMA nodes/devices in LAN 11", nWifi11);
  cmd.AddValue ("nCsma10", "Number of \"extra\" CSMA nodes/devices in LAN 10", nCsma10);
  cmd.AddValue ("nCsma9", "Number of \"extra\" CSMA nodes/devices in LAN 9", np2p9);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);
  cmd.AddValue ("mode", "Mode setting of TapBridge", mode);
  cmd.AddValue ("tapName", "Name of the OS tap device", tapName);
  //cmd.AddValue ("tapName2","Name of the OS second tap device", tapName2);

  cmd.Parse (argc,argv);

  // Check for valid number of csma or wifi nodes
  // 250 should be enough, otherwise IP addresses 
  // soon become an issue
  

/*
  if (nWifi11 > 250 ||  np2p9 != 2 || nCsma10 > 250)
    {
      std::cout << "Too many csma nodes, no more than 250 each." << std::endl;
      return 1;
    }

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }
*/
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  
  
  NodeContainer csmaNodes10;
  csmaNodes10.Create (nCsma10);

  NodeContainer wifiNodes11;
  wifiNodes11.Create (nWifi11);

    //PointToPointHelper and set the associated default Attributes so that we create a 5 Mbps transmitter on devices created using the helper and a 2 msec delay on channels created by the helper. 
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("80ms"));
  //We then Install the devices on the nodes and the channel between them
  NodeContainer p2pNodes9 = NodeContainer (wifiNodes11.Get (3), csmaNodes10.Get (1));
  NetDeviceContainer p2pDevices9 = pointToPoint.Install (p2pNodes9);


  CsmaHelper csma10;
  csma10.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma10.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (10560)));

  NetDeviceContainer csmaDevices10;
  csmaDevices10 = csma10.Install (csmaNodes10);
 
  //
  // The topology has a Wifi network of four nodes on the left side.  We'll make
  // node zero the AP and have the other three will be the STAs.
  //
  

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());

  Ssid ssid = Ssid ("left");
  WifiHelper wifi;
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ArfWifiManager");

  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid));
  NetDeviceContainer wifiDevices11 = wifi.Install (wifiPhy, wifiMac, wifiNodes11.Get (0));


  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid),
                   "ActiveProbing", BooleanValue (false));
  wifiDevices11.Add (wifi.Install (wifiPhy, wifiMac, NodeContainer (wifiNodes11.Get (1), wifiNodes11.Get (2), wifiNodes11.Get (3))));

  MobilityHelper mobility;
  mobility.Install (wifiNodes11);


  DceManagerHelper dceManager;
  dceManager.Install (csmaNodes10.Get(0));

  
  InternetStackHelper stack;
  stack.Install(wifiNodes11);
  stack.Install(csmaNodes10);
  

  Ipv4AddressHelper address;
  

  address.SetBase ("9.9.9.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces9;
  p2pInterfaces9 = address.Assign (p2pDevices9);

  //address.SetBase ("11.11.11.0", "255.255.255.0","0.0.0.7");
  address.SetBase ("11.11.11.0","255.255.255.0");
  Ipv4InterfaceContainer WifiInterfaces11;
  WifiInterfaces11 = address.Assign (wifiDevices11);

  address.SetBase ("10.10.10.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces10;
  csmaInterfaces10 = address.Assign (csmaDevices10);
  
  
  TapBridgeHelper tapBridge;
  tapBridge.SetAttribute ("Mode", StringValue (mode));
  tapBridge.SetAttribute ("DeviceName", StringValue (tapName));
  tapBridge.Install (wifiNodes11.Get(0),wifiDevices11.Get (0));
  /*
  TapBridgeHelper tapBridge2;
  tapBridge2.SetAttribute("Mode", StringValue (mode));
  tapBridge2.SetAttribute ("DeviceName", StringValue (tapName2));
  tapBridge2.Install (csmaNodes10.Get (1), csmaDevices10.Get (1));
*/
  

Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


Ptr<Ipv4> p2pnode1_addr = p2pNodes9.Get(0)->GetObject<Ipv4> ();
Ipv4Address p2pnode1_int_one = p2pnode1_addr->GetAddress(1,0).GetLocal();
std::cout <<"p2pnode1 interface one:"<< p2pnode1_int_one << std::endl;
Ipv4Address p2pnode1_int_two = p2pnode1_addr->GetAddress(2,0).GetLocal();
std::cout << "p2pnode1 interface two:"<<p2pnode1_int_two << std::endl;

Ptr<Ipv4> p2pnode2_addr = p2pNodes9.Get(1)->GetObject<Ipv4> ();
Ipv4Address p2pnode2_int_one = p2pnode2_addr->GetAddress(1,0).GetLocal();
std::cout <<"p2pnode2 interface one:"<< p2pnode2_int_one << std::endl;
Ipv4Address p2pnode2_int_two = p2pnode2_addr->GetAddress(2,0).GetLocal();
std::cout << "p2pnode2 interface two:"<<p2pnode2_int_two << std::endl;
  
/*
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> p2pnode1 = ipv4RoutingHelper.GetStaticRouting (p2pNodes9.Get(0)->GetObject<Ipv4>());
  p2pnode1->AddNetworkRouteTo (Ipv4Address ("10.10.10.0"), Ipv4Mask ("255.255.255.0"), 1);
  
  Ptr<Ipv4StaticRouting> p2pnode2 = ipv4RoutingHelper.GetStaticRouting (p2pNodes9.Get(1)->GetObject<Ipv4>());
  p2pnode2->AddNetworkRouteTo (Ipv4Address ("11.11.11.0"), Ipv4Mask ("255.255.255.0"), 1);
*/

  DceApplicationHelper dce;
  ApplicationContainer server;

  dce.SetStackSize (1 << 20);

  // Launch the server HTTP
  dce.SetBinary ("thttpd");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  //  dce.AddArgument ("-D");
  dce.SetUid (1);
  dce.SetEuid (1);
  server = dce.Install (csmaNodes10.Get (0));
  server.Start (Seconds (0.1));

  Simulator::Stop (Seconds (600.0));

  if (tracing == true)
    {
      pointToPoint.EnablePcap("dec_wifi-csma-tap", p2pDevices9.Get (0), true);
      csma10.EnablePcap ("dec_wifi-csma-tap", csmaDevices10.Get (0), true);
      wifiPhy.EnablePcap ("dec_wifi-csma-tap", wifiDevices11.Get (0), true);
    }

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
