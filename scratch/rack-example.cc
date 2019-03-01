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
std::string dir = "rack/example/";
double stopTime = 10;

static void
CwndChange (uint32_t oldCwnd, uint32_t newCwnd)
{
  std::ofstream fPlotQueue (dir + "Traces/cwnd.plotme", std::ios::out | std::ios::app);
  fPlotQueue << Simulator::Now ().GetSeconds () << " " << newCwnd/524.0 << std::endl;
  fPlotQueue.close ();
}

void
TraceCwnd (uint32_t node, uint32_t cwndWindow,
           Callback <void, uint32_t, uint32_t> CwndTrace)
{
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string (node) + "/$ns3::TcpL4Protocol/SocketList/" + std::to_string (cwndWindow) + "/CongestionWindow", CwndTrace);
}

static void
RttChange (Time oldRtt, Time newRtt)
{
  std::ofstream fPlotQueue (dir + "Traces/rtt.plotme", std::ios::out | std::ios::app);
  fPlotQueue << Simulator::Now ().GetSeconds () << " " << newRtt.GetSeconds () << std::endl;
  fPlotQueue.close ();
}

void
TraceRtt (uint32_t node, uint32_t cwndWindow,
           Callback <void, Time, Time> RttTrace)
{
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string (node) + "/$ns3::TcpL4Protocol/SocketList/" + std::to_string (cwndWindow) + "/RTT", RttTrace);
}


int main (int argc, char *argv[])
{
  uint32_t stream = 1;
  uint32_t dataSize = 524;
  uint32_t delAckCount = 1;
  bool dsack = false;
  bool rack = false;
  bool reorder = false;
  bool dupack = true;

  std::string dirToSave = "mkdir -p " + dir;
  system (dirToSave.c_str ());
  system ((dirToSave + "/pcap/").c_str ());
  system ((dirToSave + "/Traces/").c_str ());

  CommandLine cmd;
  cmd.AddValue ("stream", "Seed value for random variable", stream);
  cmd.AddValue ("delAckCount", "Delayed ack count", delAckCount);
  cmd.AddValue ("stopTime", "Stop time for applications / simulation time will be stopTime", stopTime);
  cmd.AddValue ("dsack", "Enable/Disable DSACK mode", dsack);
  cmd.AddValue ("rack", "Enable/Disable RACK mode", rack);
  cmd.AddValue ("reorder", "Enable/Disable Rrordering of packets", reorder);
  cmd.AddValue ("dupack", "Enable/Disable 3-DUPACK", dupack);
  cmd.Parse (argc,argv);

  uv->SetStream (stream);

  // Create nodes
  NodeContainer senders, routers, receivers;
  routers.Create (2);
  senders.Create (1);
  receivers.Create (1);

  // Create point-to-point channels
  PointToPointHelper p2p, p2p1;

  if (reorder)
    {
      p2p.SetQueue ("ns3::ReorderQueue");
    }
  p2p.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("10ms"));

  p2p1.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p1.SetChannelAttribute ("Delay", StringValue ("1ms"));


  // Create netdevices
  std::vector <NetDeviceContainer> leftToRouter;
  std::vector <NetDeviceContainer> routerToRight;

  // Router1 and Router2
  NetDeviceContainer r1r2ND = p2p.Install (routers.Get (0), routers.Get (1));

  // Sender i and Receiver i
  for (int i = 0; i < 1; i++)
     {
       leftToRouter.push_back (p2p1.Install (senders.Get (i), routers.Get (0)));
       routerToRight.push_back (p2p1.Install (routers.Get (1), receivers.Get (i)));
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

  for (int i = 0; i < 1; i++)
     {
       leftToRouterIPAddress.push_back (ipAddresses.Assign (leftToRouter [i]));
       ipAddresses.NewNetwork ();
     }

  std::vector <Ipv4InterfaceContainer> routerToRightIPAddress;

  for (int i = 0; i < 1; i++)
     {
       routerToRightIPAddress.push_back (ipAddresses.Assign (routerToRight [i]));
       ipAddresses.NewNetwork ();
     }

  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1 << 20));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1 << 20));
  Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (10));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (delAckCount));
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (dataSize));
  Config::SetDefault ("ns3::TcpSocketBase::Dsack", BooleanValue(dsack));
  Config::SetDefault ("ns3::TcpSocketBase::Rack", BooleanValue(rack));
  Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue(true));
  Config::SetDefault ("ns3::FifoQueueDisc::MaxSize", QueueSizeValue (QueueSize ("50p")));
  Config::SetDefault ("ns3::TcpSocketBase::WindowScaling", BooleanValue (true));

  if (!dupack)
    {
      Config::SetDefault ("ns3::TcpSocketBase::ReTxThreshold", UintegerValue (25));
    }

  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> streamWrapper;

  uint16_t port = 50000;

  // Install Sink Applications
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (receivers.Get (0));
  sinkApps.Start (Seconds (Seconds(0)));
  sinkApps.Stop (Seconds (stopTime));


  // Install BulkSend Application
  BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (routerToRightIPAddress [0].GetAddress (1), port));

  source.SetAttribute ("MaxBytes", UintegerValue (0));
  ApplicationContainer sourceApps = source.Install (senders.Get (0));
  sourceApps.Start (Seconds(0));
  sourceApps.Stop (Seconds (stopTime));
  Simulator::Schedule (Seconds (0.001), &TraceCwnd, 2, 0, MakeCallback (&CwndChange));
  Simulator::Schedule (Seconds (0.001), &TraceRtt, 2, 0, MakeCallback(&RttChange));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  TrafficControlHelper tch;
  tch.SetRootQueueDisc ("ns3::FifoQueueDisc");

  QueueDiscContainer qd;
  tch.Uninstall (routers.Get (0)->GetDevice (0));
  qd.Add (tch.Install (routers.Get (0)->GetDevice (0)).Get (0));
  p2p.EnablePcapAll (dir + "pcap/N", true);

  Simulator::Stop (Seconds (stopTime));
  Simulator::Run ();

  Simulator::Destroy ();
  return 0;
}


