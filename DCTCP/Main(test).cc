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
#include "ns3/traffic-control-module.h"
 #include "ns3/flow-monitor-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE ("HW4");


class QueueMonitor
{
    private:
        Time   m_startTime;
        Time   m_lastTime;
        std::vector<uint32_t> m_qLens;
        std::vector<Time> m_qTimes;

    public:
        QueueMonitor(){}
        QueueMonitor(Time);
        void QueueChange(uint32_t oldValue, uint32_t newValue)
        {
            m_qLens.push_back(newValue);
            m_qTimes.push_back(Simulator::Now());
            m_lastTime=Simulator::Now();
        }

        double GetAvgQueueLen()
        {
            double sum_lengths=0;
            double length=0;
            for(int i=0;i<m_qTimes.size()-1;i++)
            {
                length = (m_qTimes[i+1].GetSeconds() - m_qTimes[i].GetSeconds())*m_qLens[i];
                sum_lengths += length;
            }
            return (double)sum_lengths/(m_lastTime.GetSeconds() - m_startTime.GetSeconds());
        }

        void SaveQueueLen(std::string fn)
        {
            ofstream outfile;
            outfile.open(fn);
            for(int i=0;i<m_qLens.size();i++)
            {
               outfile<<m_qTimes[i].GetSeconds()<<" "<<m_qLens[i]<<endl;
            }
            outfile.close();
        }

};   
QueueMonitor::QueueMonitor(Time startTime)
        {
            m_startTime=startTime;
        }

class FlowAnalyzer 
{
    private:
        uint32_t m_totalRx = 0;
        Time m_startTime;
        Time m_lastTime;
        std::map<Ipv4Address, uint32_t> m_totalRxAll;
        std::map<Ipv4Address, Time> m_lastTimeAll;

    public:
        FlowAnalyzer(Time);

        void RecvPkt(Ptr<const Packet> p, const Address &a)
        {
            InetSocketAddress thisAddr = InetSocketAddress::ConvertFrom(a);
            Ipv4Address addrstr = thisAddr.GetIpv4();
            
            m_lastTimeAll[addrstr] = Simulator::Now();
            m_lastTime = Simulator::Now();

            m_totalRxAll[addrstr] += (p->GetSize() * 8);
            m_totalRx += (p->GetSize() * 8);


        }

        double CalcThruPut( Ipv4Address addr)
        {
            //computes and returns the throughput in the unit of Mbps as a double 
            //precision number, based on the class variables
           

               return (double)m_totalRxAll[addr]/(1e6*(m_lastTimeAll[addr].GetSeconds()-m_startTime.GetSeconds()));
        } 

        double CalcThruPut()
        {
            //computes and returns the throughput in the unit of Mbps as a double 
            //precision number, based on the class variables
       

               return m_totalRx/(1e6*(m_lastTime.GetSeconds()-m_startTime.GetSeconds()));
        } 


        void printer()
        {
            std::cout<<m_totalRx<<std::endl;
            std::cout<<m_startTime<<std::endl;
            std::cout<<m_lastTime<<std::endl;
        }

        double GetFCT(Ipv4Address addr)
        {
            return m_lastTimeAll[addr].GetSeconds()-m_startTime.GetSeconds();
        }

        double GetAvgFCT()
        {
            uint32_t num=0;
            double total=0;
              for (std::map<Ipv4Address, Time>::iterator it=m_lastTimeAll.begin(); it!=m_lastTimeAll.end(); ++it)
              {
                    num++;
                    total+=(it->second.GetSeconds()-m_startTime.GetSeconds());
              }
            return total/num;
        }

        void GetEachFCT()
        {
            for (std::map<Ipv4Address, Time>::iterator it=m_lastTimeAll.begin(); it!=m_lastTimeAll.end(); ++it)
              {
                    NS_LOG_UNCOND("Flow Completion Time with IP "<<it->first<<" is "<<(it->second.GetSeconds()-m_startTime.GetSeconds())<<"s");
              }
        }

        void GetEachTruPut()
        {
            for (std::map<Ipv4Address, uint32_t>::iterator it=m_totalRxAll.begin(); it!=m_totalRxAll.end(); ++it)
              {
                    NS_LOG_UNCOND("Throughput Rate with IP "<<it->first<<" is "<<CalcThruPut(it->first)<<" Mbps");
              }
        }


};   
FlowAnalyzer::FlowAnalyzer(Time startTime)
        {
            m_startTime=startTime;
        }



int
main (int argc, char *argv[])
{

    // LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
    // LogComponentEnable ("BulkSendApplication", LOG_LEVEL_INFO);


    CommandLine cmd;

    uint32_t K=20;
    double g=0.0625;
    uint32_t qLen=200;
    bool dctcp=false;
    cmd.AddValue ("K", "K", K);
    cmd.AddValue ("g", "g", g);
    cmd.AddValue ("qLen", "qLen", qLen);
    cmd.AddValue ("dctcp", "dctcp", dctcp);
    cmd.Parse (argc, argv);
    
    
    Time::SetResolution (Time::NS);
    __gnu_cxx::string qlen=__gnu_cxx::to_string(qLen)+"p";


    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (500));
    Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
    Config::SetDefault ("ns3::TcpSocketBase::EcnMode", StringValue("ClassicEcn"));
    Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (500));
    Config::SetDefault ("ns3::RedQueueDisc::UseHardDrop", BooleanValue(false));
    Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (1.0));
    Config::SetDefault ("ns3::RedQueueDisc::MaxSize", StringValue (qlen));
    Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (K));
    Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (K));
    Config::SetDefault ("ns3::RedQueueDisc::UseEcn", BooleanValue (true));
    Config::SetDefault ("ns3::FifoQueueDisc::MaxSize", StringValue (qlen));
    if(dctcp==true)
    {
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpDctcp::GetTypeId()));
        Config::SetDefault ("ns3::TcpDctcp::g", DoubleValue (g));
    }

    NodeContainer nodes;
    nodes.Create (4);
    NodeContainer AC=NodeContainer(nodes.Get(0),nodes.Get(2));
    NodeContainer BC=NodeContainer(nodes.Get(1),nodes.Get(2));
    NodeContainer CD=NodeContainer(nodes.Get(2),nodes.Get(3));
    
    
    PointToPointHelper pointToPoint;
    PointToPointHelper p2p_cd;
    p2p_cd.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));

    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("4ms"));
    p2p_cd.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    p2p_cd.SetChannelAttribute ("Delay", StringValue ("4ms"));


    NetDeviceContainer nets;
    NetDeviceContainer AACC=pointToPoint.Install(AC);
    NetDeviceContainer BBCC=pointToPoint.Install(BC);
    NetDeviceContainer CCDD=p2p_cd.Install(CD);

    InternetStackHelper stack;
    stack.Install (nodes);

   TrafficControlHelper tch;
    QueueDiscContainer tch_app;
    if(dctcp==false)
    {
         tch.SetRootQueueDisc ("ns3::FifoQueueDisc");
    }
    else
    {
        tch.SetRootQueueDisc ("ns3::RedQueueDisc");
    }
    tch_app=tch.Install(CCDD);


    pointToPoint.EnablePcapAll("HW4");

    

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces1 = address.Assign (AACC);
    address.SetBase ("10.1.2.0", "255.255.255.0");
    
    Ipv4InterfaceContainer interfaces2 = address.Assign (BBCC);
    
    address.SetBase ("10.1.3.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces3 = address.Assign (CCDD);


 
    


    Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), 7999));
    PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
    ApplicationContainer sinkApp=sinkHelper.Install(nodes.Get(3));

    QueueMonitor qm(Seconds(0.0));
    tch_app.Get(0)->TraceConnectWithoutContext ("PacketsInQueue", MakeCallback(&QueueMonitor::QueueChange,&qm));
    FlowAnalyzer tpa(Seconds(0.0));
    sinkApp.Get(0)->TraceConnectWithoutContext("Rx",MakeCallback(&FlowAnalyzer::RecvPkt,&tpa));

    BulkSendHelper source ("ns3::TcpSocketFactory",InetSocketAddress (interfaces3.GetAddress (1), 7999));
   // Set the amount of data to send in bytes.  Zero is unlimited
    source.SetAttribute ("MaxBytes", UintegerValue (0));
    source.SetAttribute ("SendSize", UintegerValue (500));
    ApplicationContainer clientA = source.Install (nodes.Get (0));
    ApplicationContainer clientB = source.Install (nodes.Get (1));
    clientA.Start (Seconds (0));
    clientA.Stop (Seconds (10.0));
    clientB.Start (Seconds (0));
    clientB.Stop (Seconds (10.0));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(10.0));

    stringstream tmp;
    if(dctcp==true)
    {
        tmp<<"Hw4/queue-<dctcp>-<"<<to_string(K)<<">-<"<<to_string(g)<<">-<"<<to_string(qLen)<<">.txt";
    }
    else
    {
        tmp<<"Hw4/queue-<tcp>-<"<<to_string(K)<<">-<"<<to_string(g)<<">-<"<<to_string(qLen)<<">.txt";
    }
    string t;
    tmp>>t;
    
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();





    NS_LOG_INFO ("Run Simulation.");

    Simulator::Stop(Seconds(10.0));
    Simulator::Run ();
    Simulator::Destroy ();
    NS_LOG_INFO ("Done.");
    NS_LOG_UNCOND("The calculated throughput is "<<tpa.CalcThruPut()<<" Mpbs.");
    NS_LOG_UNCOND("The calculated Avg Queue Length is "<<qm.GetAvgQueueLen()<<".");
    QueueDisc::Stats st;
    st=tch_app.Get(0)->GetStats();
    NS_LOG_UNCOND("Queue stats:\n" << st );
    qm.SaveQueueLen(t);
    return 0;
}
