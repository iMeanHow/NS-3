/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 NITK Surathkal
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
 * Author: Shravya K.S. <shravya.ks0@gmail.com>
 *
 */

#include "tcp-dctcp.h"
#include "ns3/log.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"
#include "ns3/abort.h"
#include "ns3/node.h"
#include "math.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/sequence-number.h"
#include "ns3/double.h"
#include "ns3/nstime.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpDctcp");

NS_OBJECT_ENSURE_REGISTERED (TcpDctcp);

TypeId TcpDctcp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpDctcp")
    .SetParent<TcpNewReno> ()
    .AddConstructor<TcpDctcp> ()
    .SetGroupName ("Internet")
    .AddAttribute ("g",
                   "Value of g for updating DCTCP.Alpha",
                   DoubleValue (0.0625),
                   MakeDoubleAccessor (&TcpDctcp::m_g),
                   MakeDoubleChecker<double> (0))
  ;
  return tid;
}

std::string TcpDctcp::GetName () const
{
  return "TcpDctcp";
}

TcpDctcp::TcpDctcp ()
  : TcpNewReno ()
{
  NS_LOG_FUNCTION (this);
}

TcpDctcp::TcpDctcp (const TcpDctcp& sock)
  : TcpNewReno (sock)
{
  NS_LOG_FUNCTION (this);
  m_g = sock.m_g;
}

TcpDctcp::~TcpDctcp (void)
{
  NS_LOG_FUNCTION (this);
}

Ptr<TcpCongestionOps> TcpDctcp::Fork (void)
{
  NS_LOG_FUNCTION (this);
  return CopyObject<TcpDctcp> (this);
}

void
TcpDctcp::ReduceCwnd (Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this << tcb);

  /***************************************************/
  //** DCTCP: Fill your code here

  tcb->m_cWnd = (1.0 - tcb->m_alpha / 2.0)* tcb->m_cWnd;
  /***************************************************/
}
void
TcpDctcp::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time &rtt)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked << rtt);

  /***************************************************/
  //** DCTCP: Fill your code here
  uint32_t bytesAcked=segmentsAcked * tcb->m_segmentSize ;
  tcb->m_bytesAcked += bytesAcked;
  if (tcb->m_ecnState == TcpSocketState::ECN_ECE_RCVD)
    {
      tcb->m_bytesMarked += bytesAcked;
    }
  if(tcb->m_windowEnd.GetValue()==0)
  {
    tcb->m_windowEnd = tcb->m_nextTxSequence;
  }
  if (tcb->m_lastAckedSeq >= tcb->m_windowEnd)
  {
    double m=0;
    if (tcb->m_bytesMarked  >  0)
      {
        m = (double) tcb->m_bytesMarked / tcb->m_bytesAcked;
      }
    tcb->m_alpha = (1.0 - m_g) * tcb->m_alpha + m_g * m;
    tcb->m_windowEnd =tcb->m_nextTxSequence;
    tcb->m_bytesAcked=0;
    tcb->m_bytesMarked=0;
  }

  /***************************************************/
}

void
TcpDctcp::ProcessCE (Ptr<TcpSocketState> tcb, bool currentCE)
{
  NS_LOG_FUNCTION (this << tcb);

  /***************************************************/
  //** DCTCP: Fill your code here
  if(currentCE==true)
     tcb->m_ecnState = TcpSocketState::ECN_CE_RCVD;
   else
      tcb->m_ecnState = TcpSocketState::ECN_IDLE;
 //tcb->m_ecnState = currentCE ? TcpSocketState::ECN_CE_RCVD : TcpSocketState::ECN_IDLE;
  /***************************************************/
}

void
TcpDctcp::CwndEvent (Ptr<TcpSocketState> tcb,
                     const TcpSocketState::TcpCAEvent_t event)
{
  NS_LOG_FUNCTION (this << tcb << event);
  switch (event)
    {
    case TcpSocketState::CA_EVENT_ECN_IS_CE:
      ProcessCE (tcb, true);
      break;
    case TcpSocketState::CA_EVENT_ECN_NO_CE:
      ProcessCE (tcb, false);
      break;
    default:
      /* Don't care for the rest. */
      break;
    }

}
}