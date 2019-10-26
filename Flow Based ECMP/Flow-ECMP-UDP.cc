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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/packet-sink.h"
#include "ns3/onoff-application.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE ("HW3-2");


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


int main (int argc, char *argv[])
{
    //LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
    //LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);


    uint32_t EcmpMode=0;
    uint32_t nPath;
    uint32_t nFlow;
    uint32_t MaxBytes=100000;
    CommandLine cmd;

    cmd.AddValue ("EcmpMode", "EcmpMode",EcmpMode);
    cmd.AddValue ("nPath", "nPath",nPath);
    cmd.AddValue ("nFlow", "nFlow",nFlow);
    cmd.AddValue ("MaxBytes", "MaxBytes",MaxBytes);

    cmd.Parse (argc, argv);
    
    
    Time::SetResolution (Time::NS);


    Config::SetDefault ("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue (true));
    Config::SetDefault ("ns3::Ipv4GlobalRouting::EcmpMode", UintegerValue (EcmpMode));
    
    PointToPointHelper pointToPoint;
    PointToPointHelper pointToPoint_last;
    uint32_t datarate_last=5*nFlow;
    pointToPoint_last.SetDeviceAttribute("DataRate",StringValue(to_string(datarate_last)+"Mbps"));
    pointToPoint_last.SetChannelAttribute("Delay", StringValue("2ms"));

    pointToPoint.SetDeviceAttribute ("DataRate", StringValue( "5Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue("2ms"));




    NodeContainer nodes;
    nodes.Create (nFlow+nPath+3);
    NodeContainer group1[nFlow];
    NetDeviceContainer group1Device[nFlow];
    for(int i=0;i<nFlow;i++)
    {
        group1[i]=NodeContainer(nodes.Get(i),nodes.Get(nFlow));
        group1Device[i]=pointToPoint.Install(group1[i]);
   
    }

    NodeContainer group2[nPath];
    NetDeviceContainer group2Device[nPath];
    for(int i=0;i<nPath;i++)
    {
        group2[i]=NodeContainer(nodes.Get(nFlow),nodes.Get(nFlow+1+i));
        group2Device[i]=pointToPoint.Install(group2[i]);
        
    }

    NodeContainer group3[nPath];
    NetDeviceContainer group3Device[nPath];
    for(int i=0;i<nPath;i++)
    {
        group3[i]=NodeContainer(nodes.Get(nFlow+1+i),nodes.Get(nFlow+nPath+1));
        group3Device[i]=pointToPoint.Install(group3[i]);
    }

    NodeContainer group4;
    NetDeviceContainer group4Device;
    group4=NodeContainer(nodes.Get(nFlow+nPath+1),nodes.Get(nFlow+nPath+2));
    group4Device=pointToPoint_last.Install(group4);
    stringstream ss;
    ss<<"Hw3-2/All-<"<<to_string(EcmpMode)<<">-<"<<to_string(nPath)<<">-<"<<to_string(nFlow)<<">";
    string s;
    ss>>s;

    pointToPoint.EnablePcapAll(s);
    InternetStackHelper stack;
    stack.Install (nodes);

    Ipv4AddressHelper address;
    Ipv4AddressGenerator::Init (Ipv4Address ("10.1.0.0"), Ipv4Mask ("/24"));
    Ipv4InterfaceContainer interface_ip[nFlow+2*nPath+1];

    for(int i=0;i<nFlow;i++)  //group1
    {
        Ipv4Address subnet = Ipv4AddressGenerator::NextNetwork (Ipv4Mask ("/24"));
        address.SetBase(subnet,"255.255.255.0");
        interface_ip[i]=address.Assign(group1Device[i]);
    }
    for(int i=0;i<nPath;i++)  //group2
    {
        Ipv4Address subnet = Ipv4AddressGenerator::NextNetwork (Ipv4Mask ("/24"));
        address.SetBase(subnet,"255.255.255.0");
        interface_ip[nFlow+i]=address.Assign(group2Device[i]);
    }

    for(int i=0;i<nPath;i++)   //group3
    {
        Ipv4Address subnet = Ipv4AddressGenerator::NextNetwork (Ipv4Mask ("/24"));
        address.SetBase(subnet,"255.255.255.0");
        interface_ip[nFlow+nPath+i]=address.Assign(group3Device[i]);
    }

    Ipv4Address subnet = Ipv4AddressGenerator::NextNetwork (Ipv4Mask ("/24"));
    address.SetBase(subnet,"255.255.255.0");
    interface_ip[nFlow+2*nPath]=address.Assign(group4Device);

    Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), 7999));

    PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);
    FlowAnalyzer tpa(Seconds(1.0));
    ApplicationContainer sinkApp=sinkHelper.Install(nodes.Get(nFlow+nPath+2));
    sinkApp.Get(0)->TraceConnectWithoutContext("Rx",MakeCallback(&FlowAnalyzer::RecvPkt,&tpa));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(100.0));

    ApplicationContainer client[nFlow];
    for(int i=0;i<nFlow;i++)
    {
        //send to nSource + i% nSink
        OnOffHelper OnOffClient ("ns3::UdpSocketFactory", InetSocketAddress(interface_ip[nFlow+2*nPath].GetAddress(1),7999));
        OnOffClient.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        OnOffClient.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        OnOffClient.SetAttribute("DataRate",StringValue("5Mbps"));
        OnOffClient.SetAttribute("MaxBytes",UintegerValue(MaxBytes));   
        OnOffClient.SetAttribute("PacketSize",UintegerValue(1000));
        client[i]=OnOffClient.Install(nodes.Get(i));
        client[i].Start(Seconds(1.0));
        client[i].Stop(Seconds(100.0));
    }

    
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


    NS_LOG_INFO ("Run Simulation.");
    Simulator::Run ();
    Simulator::Destroy ();
    
    NS_LOG_UNCOND("The calculated throughput with "<<nFlow<<" flow and "<<nPath<<" path is "<<tpa.CalcThruPut()<<" Mpbs.");
    NS_LOG_UNCOND("The calculated Avg FCT with "<<nFlow<<" flow and "<<nPath<<" path is "<<tpa.GetAvgFCT()<<"s.");
    NS_LOG_UNCOND("==================For Per Flow==================");
    tpa.GetEachFCT();
    tpa.GetEachTruPut();


    NS_LOG_INFO ("Done.");
    return 0;
}
