#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-address-generator.h"
#include "ns3/packet-sink.h"
#include "ns3/onoff-application.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("hw2-3");

class ThruPutAnalyzer 
{
    private:
        uint32_t m_totalRx = 0;
        Time m_startTime;
        Time m_lastTime;

    public:
        ThruPutAnalyzer(Time);

        void RecvPkt(Ptr<const Packet> p, const Address &a)
        {
            //when a packet arrives at the PacketSink, this function will be called
            //i. increase the m_totalRx variable by the size of the received packet
            //ii. update the m_lastTime as the current time in the simulator
             m_lastTime=Simulator::Now();
            m_totalRx++;
           
            // std::cout<<m_totalRx<<std::endl;


        }

        double CalcThruPut(int nSource, int maxBytes)
        {
            //computes and returns the throughput in the unit of Mbps as a double 
            //precision number, based on the class variables
           // std::cout<<:
        	//cout<<"Start Time: "<<m_startTime<<" Last Time: "<<m_lastTime<<endl;
       
            return (double)maxBytes *nSource * 8/(1e6*(m_lastTime.GetSeconds()-m_startTime.GetSeconds()));
        } 
};   
ThruPutAnalyzer::ThruPutAnalyzer(Time startTime)
        {
            m_startTime=startTime;
        }

int
main (int argc, char *argv[])
{
	// LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
	// LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

	bool ecmp = false;
	uint32_t nSink = 64;
	uint32_t maxBytes = 1000000;
	uint32_t nSource;
	CommandLine cmd;
    cmd.AddValue ("ecmp", "ecmp", ecmp);
    cmd.AddValue ("nSink", "nSink",nSink);
    cmd.AddValue ("maxBytes", "maxBytes",maxBytes);
    cmd.Parse (argc, argv);
    nSource = 128 - nSink;

    if (ecmp == true) 
    {
    	Config::SetDefault ("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue (true));
    }


	// 2.a. Create all hosts, then all edges, then all aggregate switches, and finally all core switches
	NodeContainer hosts, edges, aggrs, cores;
	hosts.Create (128);
	edges.Create (32);
	aggrs.Create (32);
	cores.Create (16);

	// 2.e. All links have 5Mbps bandwidth and 2ms delay
	PointToPointHelper pointToPoint;
	pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
	pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

	// 2.b. Install all host-to-edge links as PointToPoint channels
	// 2.c. Install all edge-to-aggr links as PointToPoint channels
	NodeContainer edgeToHost[32][4];
	NetDeviceContainer edgeToHostDevice[32][4];
	NodeContainer aggrToEdge[32][4];
	NetDeviceContainer aggrToEdgeDevice[32][4];

	for(int i = 0; i < 32; i++)
	{
		for(int j = 0; j < 4; j++)
		{
			edgeToHost[i][j] = NodeContainer(hosts.Get (i*4+j), edges.Get (i));
			aggrToEdge[i][j] = NodeContainer(edges.Get ((i/4)*4+j), aggrs.Get (i));
			edgeToHostDevice[i][j] = pointToPoint.Install (edgeToHost[i][j]);
			aggrToEdgeDevice[i][j] = pointToPoint.Install (aggrToEdge[i][j]);
		}
	}

	// 2.d. Install all aggr-to-core links as PointToPoint channels
	NodeContainer coreToAggr[16][8];
	NetDeviceContainer coreToAggrDevice[16][8];

	for(int i = 0; i < 16; i++)
	{
		for(int j = 0; j < 8; j++)
		{
			coreToAggr[i][j] = NodeContainer(aggrs.Get (i/4+4*j), cores.Get (i));
			coreToAggrDevice[i][j] = pointToPoint.Install (coreToAggr[i][j]);
		}
	}

	// 3. Install InternetStack on all nodes
	InternetStackHelper stack;
	stack.Install (hosts);
	stack.Install (edges);
	stack.Install (aggrs);
	stack.Install (cores);

	Ipv4AddressHelper address;
	Ipv4AddressGenerator::Init (Ipv4Address ("10.1.0.0"), Ipv4Mask ("/24"));
	Ipv4InterfaceContainer edgeToHostIP[32][4];
	Ipv4InterfaceContainer aggrToEdgeIP[32][4];
	Ipv4InterfaceContainer coreToAggrIP[16][8];

	// 4.e.i. All host-edge links with the host NetDevice in front of the edge NetDevice
	for(int i = 0; i < 32; i++)
	{
		for(int j = 0; j < 4; j++)
		{
			Ipv4Address subnet = Ipv4AddressGenerator::NextNetwork (Ipv4Mask ("/24"));
			address.SetBase (subnet, "255.255.255.0");
			edgeToHostIP[i][j] = address.Assign (edgeToHostDevice[i][j]);
		}
	}
	// 4.e.ii. All edge-aggr links sorted first in the order of aggr switches, and then the edge
	// switches that connect to each aggr switch, with the edge NetDevice in front of
	// the aggr NetDevice
	for(int i = 0; i < 32; i++)
	{
		for(int j = 0; j < 4; j++)
		{
			Ipv4Address subnet = Ipv4AddressGenerator::NextNetwork (Ipv4Mask ("/24"));
			address.SetBase (subnet, "255.255.255.0");
			aggrToEdgeIP[i][j] = address.Assign (aggrToEdgeDevice[i][j]);
		}
	}

	// 4.e.iii. All aggr-core links sorted first in the order of core groups, then the core
	// switches, and finally the aggr switches that connect to each core switch, with
	// the aggr NetDevice in front of the core NetDevice
	for(int i = 0; i < 16; i++)
	{
		for(int j = 0; j < 8; j++)
		{
			Ipv4Address subnet = Ipv4AddressGenerator::NextNetwork (Ipv4Mask ("/24"));
			address.SetBase (subnet, "255.255.255.0");
			coreToAggrIP[i][j] = address.Assign (coreToAggrDevice[i][j]);
		}
	}

	// 5. Populate the global routing tables
	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	//install onoffapplication on the first nSource hosts
	ApplicationContainer client[128];
	for(int i=0;i<nSource;i++)
	{
		//send to nSource + i% nSink
		OnOffHelper OnOffClient ("ns3::TcpSocketFactory", InetSocketAddress(edgeToHostIP[(i+nSource)/4][(i+nSource)%4].GetAddress (0),7999));
		OnOffClient.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    	OnOffClient.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
   		OnOffClient.SetAttribute("DataRate",StringValue("5Mbps"));
    	OnOffClient.SetAttribute("MaxBytes",UintegerValue(maxBytes));   
    	OnOffClient.SetAttribute("PacketSize",UintegerValue(1000));
    	client[i]=OnOffClient.Install(hosts.Get(i));
    	client[i].Start(Seconds(5.0));
	}

	//install PacketSink on the rest nSink hosts
	ApplicationContainer sinkApp[128];
	ThruPutAnalyzer tpa(Seconds(5.0));
	for(int i=nSource; i<128;i++)
	{
		Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), 7999));
    	PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);

    	sinkApp[i]=sinkHelper.Install(hosts.Get(i));
    	sinkApp[i].Get(0)->TraceConnectWithoutContext("Rx",MakeCallback(&ThruPutAnalyzer::RecvPkt,&tpa));
		sinkApp[i].Start(Seconds(0.0));
	}
	

	// 8. Enable Pcap tracing on the first host and the last host, with prefix “Hw2-3/All”; 

	for(int i=0;i<32;i++)
		for(int j=0;j<4;j++)
		{
			//pointToPoint.EnablePcap ("Hw2-3/All", edgeToHost[i][j].Get (0)->GetId (), false);
			//enable hosts pcap
			pointToPoint.EnablePcap ("Hw2-3/All", edgeToHost[i][j].Get (1)->GetId (), false);
			//enable edges pcap

		}

	Simulator::Run ();
  	Simulator::Destroy ();
  	 NS_LOG_UNCOND("The calculated throughput with "<<nSource<<" senders is "<<tpa.CalcThruPut(nSource,maxBytes)<<" Mpbs.");

  	return 0;

}