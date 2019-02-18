/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Natale Patriciello <natale.patriciello@gmail.com>
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
 */
#include "ns3/log.h"
#include "ns3/tcp-westwood.h"
#include "tcp-general-test.h"
#include "ns3/simple-channel.h"
#include "ns3/node.h"
#include "tcp-error-model.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpRackTest");

class TcpRackSackTest : public TcpGeneralTest
{
public:
  /**
   * \brief Constructor
   * \param congControl Type of congestion control.
   * \param seqToKill Sequence number of the packet to drop.
   * \param rack Boolean variable to enable or disable RACK.
   * \param msg Test message.
   */
  TcpRackSackTest (bool sack, bool rack, const std::string &msg);

  virtual Ptr<TcpSocketMsgBase> CreateSenderSocket (Ptr<Node> node);

  protected:

  virtual void Tx (const Ptr<const Packet> p, const TcpHeader&h, SocketWho who);

  bool m_sackState;
  bool m_rackState;

};

TcpRackSackTest::TcpRackSackTest (bool sack, bool rack, const std::string &msg)
  : TcpGeneralTest (msg),
    m_sackState (sack),
    m_rackState (rack)
{
}

Ptr<TcpSocketMsgBase>
TcpRackSackTest::CreateSenderSocket (Ptr<Node> node)
{
  Ptr<TcpSocketMsgBase> socket = TcpGeneralTest::CreateSenderSocket (node);
  socket->SetAttribute ("Rack", BooleanValue (m_rackState));
  socket->SetAttribute ("Sack", BooleanValue (m_sackState));
  return socket;
}

void
TcpRackSackTest::Tx (const Ptr<const Packet> p, const TcpHeader&h, SocketWho who)
{

  if ((h.GetFlags () & TcpHeader::SYN))
    {
      std::cout << h.HasOption (TcpOption::SACKPERMITTED);
      NS_TEST_ASSERT_MSG_EQ (h.HasOption (TcpOption::SACKPERMITTED), true,
                             "SackPermitted disabled but option enabled");
    }
}


class TcpRackTest : public TcpGeneralTest
{
public:
  /**
   * \brief Constructor
   * \param congControl Type of congestion control.
   * \param seqToKill Sequence number of the packet to drop.
   * \param rack Boolean variable to enable or disable RACK.
   * \param msg Test message.
   */
  TcpRackTest (TypeId congControl, uint32_t seqToKill, bool rack, const std::string &msg);

  virtual Ptr<ErrorModel> CreateSenderErrorModel ();
  virtual Ptr<ErrorModel> CreateReceiverErrorModel ();

  virtual Ptr<TcpSocketMsgBase> CreateSenderSocket (Ptr<Node> node);

protected:


  virtual void CongStateTrace (const TcpSocketState::TcpCongState_t oldValue,
                               const TcpSocketState::TcpCongState_t newValue);

  virtual void CWndTrace (uint32_t oldValue, uint32_t newValue);

  /**
   * \brief Check if the packet being dropped is the right one.
   * \param ipH IPv4 header.
   * \param tcpH TCP header.
   * \param p The packet.
   */
  void PktDropped (const Ipv4Header &ipH, const TcpHeader& tcpH, Ptr<const Packet> p);

  virtual void ConfigureProperties ();
  virtual void ConfigureEnvironment ();


  bool m_pktDropped;      //!< The packet has been dropped.
  bool m_pktWasDropped;   //!< The packet was dropped (according to the receiver).
  uint32_t m_seqToKill;   //!< Sequence number to drop.
  bool m_rack;            //!< Enable/Disable RACK.

  Ptr<TcpSeqErrorModel> m_errorModel; //!< Error model.
};

TcpRackTest::TcpRackTest (TypeId typeId, uint32_t seqToKill, bool rack,
                                  const std::string &msg)
  : TcpGeneralTest (msg),
    m_pktDropped (false),
    m_seqToKill (seqToKill),
    m_rack (rack)
{
  m_congControlTypeId = typeId;
}

void
TcpRackTest::ConfigureProperties ()
{
  TcpGeneralTest::ConfigureProperties ();
  SetInitialSsThresh (SENDER, 0);
}

void
TcpRackTest::ConfigureEnvironment ()
{
  TcpGeneralTest::ConfigureEnvironment ();
  SetAppPktCount (100);
}

Ptr<ErrorModel>
TcpRackTest::CreateSenderErrorModel ()
{
  return 0;
}

Ptr<ErrorModel>
TcpRackTest::CreateReceiverErrorModel ()
{
  m_errorModel = CreateObject<TcpSeqErrorModel> ();
  m_errorModel->AddSeqToKill (SequenceNumber32 (m_seqToKill));
  m_errorModel->SetDropCallback (MakeCallback (&TcpRackTest::PktDropped, this));

  return m_errorModel;
}


Ptr<TcpSocketMsgBase>
TcpRackTest::CreateSenderSocket (Ptr<Node> node)
{
  Ptr<TcpSocketMsgBase> socket = TcpGeneralTest::CreateSenderSocket (node);
  socket->SetAttribute ("MinRto", TimeValue (Seconds (10.0)));
  socket->SetAttribute ("Sack", BooleanValue (true));
  socket->SetAttribute ("Dsack", BooleanValue (true));
  if (m_rack)
    {
      socket->SetAttribute ("Rack", BooleanValue (true));
    }

  return socket;
}

void
TcpRackTest::PktDropped (const Ipv4Header &ipH, const TcpHeader& tcpH, Ptr<const Packet> p)
{
  NS_LOG_FUNCTION (this << ipH << tcpH);

  m_pktDropped = true;

  NS_TEST_ASSERT_MSG_EQ (tcpH.GetSequenceNumber (), SequenceNumber32 (m_seqToKill),
                         "Packet dropped but sequence number differs");
}

void
TcpRackTest::CongStateTrace (const TcpSocketState::TcpCongState_t oldValue,
                                 const TcpSocketState::TcpCongState_t newValue)
{
  NS_LOG_FUNCTION (this << oldValue << newValue);

  if (oldValue == TcpSocketState::CA_OPEN && newValue == TcpSocketState::CA_DISORDER)
    {
    }
  else if (oldValue == TcpSocketState::CA_DISORDER && m_rack)
  {
    NS_TEST_ASSERT_MSG_EQ (newValue, TcpSocketState::CA_RECOVERY, "RACK not working");
  }
  else if (oldValue == TcpSocketState::CA_DISORDER && !m_rack)
  {
    NS_TEST_ASSERT_MSG_EQ (newValue, TcpSocketState::CA_LOSS, "Something working");
  }
}

void
TcpRackTest::CWndTrace (uint32_t oldValue, uint32_t newValue)
{
  NS_LOG_FUNCTION (this << oldValue << newValue);

  if (m_rack)
    {
      std::ofstream fPlotQueue ("rack-on.plotme", std::ios::out | std::ios::app);
      fPlotQueue << Simulator::Now ().GetSeconds () << " " << newValue/500.0 << std::endl;
      fPlotQueue.close ();
    }
  else
    {
      std::ofstream fPlotQueue ("rack-off.plotme", std::ios::out | std::ios::app);
      fPlotQueue << Simulator::Now ().GetSeconds () << " " << newValue/500.0 << std::endl;
      fPlotQueue.close ();
    }
}
/**
 * \ingroup internet-test
 * \ingroup tests
 *
 * \brief Testsuite for the RACK
 */
class TcpRackTestSuite : public TestSuite
{
public:
  TcpRackTestSuite () : TestSuite ("tcp-rack-test", UNIT)
  {
    AddTestCase (new TcpRackSackTest (true, true, "Sack enable"), TestCase::QUICK);
    AddTestCase (new TcpRackSackTest (false, true, "Sack disable"), TestCase::QUICK);

    AddTestCase (new TcpRackTest (TcpNewReno::GetTypeId (), 48501, true, "Rack testing"), TestCase::QUICK);
    AddTestCase (new TcpRackTest (TcpNewReno::GetTypeId (), 48501, false, "Rack testing"), TestCase::QUICK);
  }
};

static TcpRackTestSuite g_TcpRackTestSuite; //!< Static variable for test initialization


