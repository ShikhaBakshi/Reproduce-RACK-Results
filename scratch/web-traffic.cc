/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 NITK Surathkal
 *
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
 *
 * Authors: Shikha Bakshi <shikhabakshi912@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

// All the pcaps will be collected in a folder named dsack in the ns-3 root directory.
Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
std::string dir = "rack/http/dumbbell-http/";
double stopTime = 300;

static void
ClientRx (Ptr<OutputStreamWrapper> stream, const Time &Rtt, const Address &address)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << " " << Rtt.GetSeconds () << std::endl;
}

void
ServerTx (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> packet)
{
  *stream->GetStream () << Simulator::Now ().GetMilliSeconds () << " " << "1" << std::endl;
//  std::cout << "Server sent a packet of " << packet->GetSize () << " bytes.\n";
}

static void
RxDrop (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << " " << p->GetSize () << std::endl;
}

void InstallHttpClient (Ptr<Node> node, Ipv4Address serverAddress, uint32_t nodeId)
{
  // Create HTTP client helper
  ThreeGppHttpClientHelper clientHelper (serverAddress);
  AsciiTraceHelper asciiTraceHelper;

  // Install HTTP client
  ApplicationContainer clientApps = clientHelper.Install (node);
  Ptr<ThreeGppHttpClient> httpClient = clientApps.Get (0)->GetObject<ThreeGppHttpClient> ();
  PointerValue varPtr;
  httpClient->GetAttribute ("Variables", varPtr);
  Ptr<ThreeGppHttpVariables> httpVariables = varPtr.Get<ThreeGppHttpVariables> ();
  httpVariables->SetRequestSize (500); // Set Request Size to 1KB
  clientApps.Start (Seconds (0.0));
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (dir + "rtt/client-RTT-" + std::to_string (nodeId) + ".plotme");
  httpClient->TraceConnectWithoutContext ("RxRtt", MakeBoundCallback (&ClientRx, stream));
  clientApps.Stop (Seconds (stopTime));
}

void InstallHttpServer (Ptr<Node> node, Ipv4Address serverAddress)
{
  ThreeGppHttpServerHelper serverHelper (serverAddress);
  AsciiTraceHelper asciiTraceHelper;

  ApplicationContainer serverApps = serverHelper.Install (node);
  Ptr<ThreeGppHttpServer> httpServer = serverApps.Get (0)->GetObject<ThreeGppHttpServer> ();
  // Setup HTTP variables for the server
  PointerValue varPtr;
  httpServer->GetAttribute ("Variables", varPtr);
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (dir + "rtt/server.plotme");
  httpServer->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&ServerTx, stream));
  Ptr<ThreeGppHttpVariables> httpVariables = varPtr.Get<ThreeGppHttpVariables> ();
  httpVariables->SetMainObjectSizeMean (1024000); // 500KB
  httpVariables->SetMainObjectSizeStdDev (40960); // 40KB
}

int main (int argc, char *argv[])
{
  uint32_t stream = 1;
  uint32_t dataSize = 524;
  uint32_t delAckCount = 1;
  bool dsack = false;
  bool rack = false;

  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> streamWrapper;

  std::string dirToSave = "mkdir -p " + dir;
  system (dirToSave.c_str ());
  system ((dirToSave + "/pcap/").c_str ());
  system ((dirToSave + "/rtt/").c_str ());

  CommandLine cmd;
  cmd.AddValue ("stream", "Seed value for random variable", stream);
  cmd.AddValue ("delAckCount", "Delayed ack count", delAckCount);
  cmd.AddValue ("stopTime", "Stop time for applications / simulation time will be stopTime", stopTime);
  cmd.AddValue ("dsack", "Enable/Disable DSACK mode", dsack);
  cmd.AddValue ("rack", "Enable/Disable RACK mode", rack);
  cmd.Parse (argc,argv);

  uv->SetStream (stream);

  // Create nodes
  NodeContainer senders, routers, receivers;
  routers.Create (2);
  senders.Create (5);
  receivers.Create (5);

  // Create point-to-point channels
  PointToPointHelper p2p, p2p1, p2p2;
        
  p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));

  p2p1.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2p1.SetChannelAttribute ("Delay", StringValue ("1ms"));

  // Create netdevices
  std::vector <NetDeviceContainer> leftToRouter;
  std::vector <NetDeviceContainer> routerToRight;

  // Router1 and Router2
  NetDeviceContainer r1r2ND = p2p.Install (routers.Get (0), routers.Get (1));

  // Sender i and Receiver i
  for (uint32_t i = 0; i < senders.GetN (); i++)
     {
       leftToRouter.push_back (p2p1.Install (senders.Get (i), routers.Get (0)));
       routerToRight.push_back (p2p1.Install (routers.Get (1), receivers.Get (i)));
     }

  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (0.0001));

  for (uint32_t i=0; i < receivers.GetN (); i++)
     {
       routerToRight [i].Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
       Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream (dir + "rtt/drop-" + std::to_string (i + 1) + ".plotme");
       routerToRight [i].Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, stream1));
     }

  // Install Stack
  InternetStackHelper stack;
  stack.Install (routers);
  stack.Install (senders);
  stack.Install (receivers);

  Ipv4AddressHelper ipAddresses ("10.0.0.0", "255.255.255.0");

  Ipv4InterfaceContainer r1r2IPAddress = ipAddresses.Assign (r1r2ND);
  ipAddresses.NewNetwork ();

  std::vector <Ipv4InterfaceContainer> leftToRouterIPAddress;

  for (uint32_t i = 0; i < senders.GetN (); i++)
     {
       leftToRouterIPAddress.push_back (ipAddresses.Assign (leftToRouter [i]));
       ipAddresses.NewNetwork ();
     }

  std::vector <Ipv4InterfaceContainer> routerToRightIPAddress;

  for (uint32_t i = 0; i < receivers.GetN (); i++)
     {
       routerToRightIPAddress.push_back (ipAddresses.Assign (routerToRight [i]));
       ipAddresses.NewNetwork ();
     }

  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1 << 20));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1 << 20));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (delAckCount));
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (dataSize));
  Config::SetDefault ("ns3::TcpSocketBase::Dsack", BooleanValue(dsack));
  Config::SetDefault ("ns3::TcpSocketBase::Rack", BooleanValue(rack));
  Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue(true));
  Config::SetDefault ("ns3::TcpSocketBase::WindowScaling", BooleanValue (true));
  Config::SetDefault ("ns3::RateErrorModel::ErrorUnit", StringValue ("ERROR_UNIT_PACKET"));


  // Install HttpServer Application
  for (uint32_t i = 0; i < senders.GetN () ; i++)
     {
       InstallHttpServer (senders.Get (i), leftToRouterIPAddress [i].GetAddress (0));
     }

  // Install HttpClient Application
  for (uint32_t i = 0; i < receivers.GetN (); i++)
     {
       InstallHttpClient (receivers.Get (i), leftToRouterIPAddress [i].GetAddress (0), i + 1);
     }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  p2p.EnablePcapAll (dir + "pcap/N", true);

  Simulator::Stop (Seconds (stopTime));
  Simulator::Run ();

  Simulator::Destroy ();
  return 0;
}

