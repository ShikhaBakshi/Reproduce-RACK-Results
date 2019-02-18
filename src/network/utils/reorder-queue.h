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

#ifndef REORDER_QUEUE_H
#define REORDER_QUEUE_H

#include <queue>
#include "ns3/queue.h"
#include "ns3/queue-size.h"

namespace ns3 {

/**
 * \ingroup queue
 *
 * \brief A FIFO packet queue that drops tail-end packets on overflow
 */
template <typename Item>
class ReorderQueue : public Queue<Item>
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief ReorderQueue Constructor
   *
   * Creates a droptail queue with a maximum size of 100 packets by default
   */
  ReorderQueue ();

  virtual ~ReorderQueue ();

  virtual bool Enqueue (Ptr<Item> item);
  virtual Ptr<Item> Dequeue (void);
  virtual Ptr<Item> Remove (void);
  virtual Ptr<const Item> Peek (void) const;

private:

  QueueSize m_maxSize;                //!< max queue size
  using Queue<Item>::Head;
  using Queue<Item>::Tail;
  using Queue<Item>::DoEnqueue;
  using Queue<Item>::DoDequeue;
  using Queue<Item>::DoRemove;
  using Queue<Item>::DoPeek;
  using Queue<Item>::DropBeforeEnqueue;
  std::queue<Ptr<Item> > m_packets;    //!< the packets in the queue
  uint32_t m_bytesInQueue;            //!< actual bytes in the queue
  uint32_t m_reorderDepth;
  uint32_t m_inSequenceLength;
  uint32_t m_holdCount;
  uint32_t m_inSequenceCount;
  Ptr<Packet> m_held;


  NS_LOG_TEMPLATE_DECLARE;     //!< redefinition of the log component
};


extern template class ReorderQueue<Packet>;
extern template class ReorderQueue<QueueDiscItem>;

} // namespace ns3

#endif /* REORDER_QUEUE_H */
