/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 University of Washington
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
 */

#include "reorder-queue.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ReorderQueue");

NS_OBJECT_TEMPLATE_CLASS_DEFINE (ReorderQueue,Packet);

template <typename Item>
TypeId
ReorderQueue<Item>::GetTypeId (void)
{
  static TypeId tid = TypeId (("ns3::ReorderQueue<" + GetTypeParamName<ReorderQueue<Item> > () + ">").c_str ())
    .SetParent<Queue<Item> > ()
    .SetGroupName ("Network")
    .template AddConstructor<ReorderQueue<Item> > ()
    .AddAttribute ("MaxLength",
                   "The max queue size",
                   QueueSizeValue (QueueSize ("100p")),
                   MakeQueueSizeAccessor (&ReorderQueue::m_maxSize),
                   MakeQueueSizeChecker ())
    .AddAttribute ("ReorderDepth", 
                   "The number of packets that will bypass a held packet",
                   UintegerValue (3),
                   MakeUintegerAccessor (&ReorderQueue::m_reorderDepth),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("InSequenceLength",
                   "The number of packets until a reordering event",
                   UintegerValue (10),
                   MakeUintegerAccessor (&ReorderQueue::m_inSequenceLength),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

template <typename Item>
ReorderQueue<Item>::ReorderQueue () :
  Queue<Item> (),
  m_packets (),
  m_bytesInQueue (0),
  m_holdCount (0),
  m_inSequenceCount (0),
  m_held (0),
  NS_LOG_TEMPLATE_DEFINE ("ReorderQueue")
{
  NS_LOG_FUNCTION (this);
}

template <typename Item>
ReorderQueue<Item>::~ReorderQueue ()
{
  NS_LOG_FUNCTION (this);
}

template <typename Item>
bool
ReorderQueue<Item>::Enqueue (Ptr<Item> item)
{
  NS_LOG_FUNCTION (this << item);

  if (m_maxSize.GetUnit () == QueueSizeUnit::PACKETS && (m_packets.size () >= m_maxSize.GetValue ()))
    {
      NS_LOG_LOGIC ("Queue full (at max packets) -- droppping pkt");
      DropBeforeEnqueue (item);
      return false;
    }

  if (m_maxSize.GetUnit () == QueueSizeUnit::BYTES && (m_bytesInQueue + item->GetSize () >= m_maxSize.GetValue ()))
    {
      NS_LOG_LOGIC ("Queue full (packet would exceed max bytes) -- droppping pkt");
      DropBeforeEnqueue (item);
      return false;
    }

  m_bytesInQueue += item->GetSize ();
  m_packets.push (item);

  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return true;
}

template <typename Item>
Ptr<Item>
ReorderQueue<Item>::Dequeue (void)
{
  NS_LOG_FUNCTION (this);

  if (m_packets.empty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  Ptr<Item> p;
  if (m_holdCount == m_reorderDepth && m_held)
    {
      p = m_held;
      m_held = 0;
      m_bytesInQueue -= p->GetSize ();
      NS_LOG_LOGIC ("Released " << p->GetUid ());
      m_inSequenceCount = 0;
      m_holdCount = 0;
    }

  else
    {
      if (m_inSequenceCount == m_inSequenceLength)
        {
          m_held = m_packets.front ();
          NS_LOG_LOGIC ("Hold " << m_held->GetUid ());
          m_packets.pop ();
          m_inSequenceCount = 0;
          m_holdCount = 0;
          if (m_packets.empty ())
            {
              // We need to release this packet since no others to reorder with
              NS_LOG_LOGIC ("Release " << m_held->GetUid () << " anyway");
              p = m_held;
              m_held = 0;
              m_bytesInQueue -= p->GetSize ();
              return p;
            }
        }
      p = m_packets.front ();
      m_packets.pop ();
      m_bytesInQueue -= p->GetSize ();
      NS_LOG_LOGIC ("Popped " << p);
      if (m_held)
        {
          m_holdCount++;
        }
      else
        {
          m_inSequenceCount++;
        }
    }

  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return p;
}

template <typename Item>
Ptr<const Item>
ReorderQueue<Item>::Peek (void) const
{
  NS_LOG_FUNCTION (this);

  return DoPeek (Head ());
}

template <typename Item>
Ptr<Item>
ReorderQueue<Item>::Remove (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<Item> item = DoRemove (Head ());

  NS_LOG_LOGIC ("Removed " << item);

  return item;
}

} // namespace ns3
