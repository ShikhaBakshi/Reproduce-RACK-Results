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
 Author: Shikha Bakshi <shikhabakshi912@gmail.com>
 *       Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 *
 */

#include "tcp-rack.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpRack");
NS_OBJECT_ENSURE_REGISTERED (TcpRack);

TypeId
TcpRack::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpRack")
    .SetParent<Object> ()
    .AddConstructor<TcpRack> ()
    .SetGroupName ("Internet")
  ;
  return tid;
}

TcpRack::TcpRack (void)
  : Object (),
    m_rackXmitTs (0),
    m_rackEndSeq (0),
    m_rackRtt (0),
    m_reoWnd (0),
    m_minRtt (0),
    m_rttSeq (0),
    m_dsack (false),
    m_reoWndIncr (1),
    m_reoWndPersist (16),
    m_srtt (0),
    m_alpha (0.125)
{
  NS_LOG_FUNCTION (this);
}

TcpRack::TcpRack (const TcpRack &other)
  : Object (other),
    m_rackXmitTs (other.m_rackXmitTs),
    m_rackEndSeq (other.m_rackEndSeq),
    m_rackRtt (other.m_rackRtt),
    m_reoWnd (other.m_reoWnd),
    m_minRtt (other.m_minRtt),
    m_rttSeq (other.m_rttSeq),
    m_dsack (other.m_dsack),
    m_reoWndIncr (other.m_reoWndIncr),
    m_reoWndPersist (other.m_reoWndPersist),
    m_srtt (other.m_srtt),
    m_alpha (other.m_alpha)
{
  NS_LOG_FUNCTION (this);
}

TcpRack::~TcpRack ()
{
  NS_LOG_FUNCTION (this);
}

bool 
TcpRack::SentAfter (Time t1, Time t2, uint32_t seq1, uint32_t seq2)
{
  NS_LOG_FUNCTION (this);
  return (t1 > t2 || (t1 == t2 && seq1 > seq2));
}

void
TcpRack::UpdateStats (uint32_t tser, bool retrans, Time xmitTs, SequenceNumber32 endSeq, SequenceNumber32 sndNxt, Time lastRtt)
{
  NS_LOG_FUNCTION (this);

  // Calculate RTT
  Time rtt = lastRtt;

  // Check if the ACK was for a retransmitted packet. Also if it was a spurious retransmission
  if (retrans)
    {
      if (tser < xmitTs.GetInteger ())
        {
          return;
        }

      if (rtt < m_minRtt)
        {
          return;
        }
    }

  if (rtt > Seconds(0))
    {
      m_rackRtt = rtt;
      m_rttSeq = sndNxt;
    }

  if (m_minRtt == 0)
    {
      m_minRtt = m_rackRtt;
    }
  else
    {
      m_minRtt = std::min (m_minRtt, m_rackRtt);
    }

  if (m_srtt == 0)
    {
      m_srtt = m_rackRtt.GetMilliSeconds ();
    }
  else
    {
      m_srtt = (1 - m_alpha) * m_srtt + m_alpha * m_rackRtt.GetMilliSeconds ();
    }

  if (SentAfter (xmitTs, m_rackXmitTs, endSeq.GetValue (), m_rackEndSeq.GetValue ()))
    {
      m_rackXmitTs = xmitTs;
      m_rackEndSeq = endSeq;
    }
}

void
TcpRack::UpdateReoWnd (bool reordSeen, bool dsackSeen, SequenceNumber32 sndNxt, SequenceNumber32 sndUna,
                       Ptr<TcpSocketState> tcb, uint32_t sacked, uint32_t dupAckThresh, bool exiting)
{
  NS_LOG_FUNCTION (this);

  if (dsackSeen)
    {
      m_dsack = true;
    }

  // React to DSACK once per round trip
  if (sndUna < m_rttSeq)
    {
      m_dsack = false;
    }

  if (m_dsack)
    {
      m_reoWndIncr++;
      m_dsack = false;
      m_rttSeq = sndNxt;
      m_reoWndPersist = 16;      // Keep window for 16 recoveries
    }
  else if (exiting)              // Exiting Loss Recovery
    {
      m_reoWndPersist--;
      if (m_reoWndPersist <= 0)
        {
          m_reoWndIncr = 1;
        }
    }

  if (!reordSeen)
    {
      if ((tcb->m_congState >= TcpSocketState::CA_RECOVERY) || (sacked >= dupAckThresh))
        {
          m_reoWnd = 0;
          return;
        }
    }

  m_reoWnd = (m_minRtt / 4).GetSeconds () * m_reoWndIncr;
  m_reoWnd = std::min (m_reoWnd, m_srtt);
  return;
}

} // namespace ns3
