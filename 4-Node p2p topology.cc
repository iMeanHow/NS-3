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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("HW1-1");

int
main (int argc, char *argv[])
{
    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    uint32_t StuID;
    CommandLine cmd;
    cmd.AddValue ("StuID", "Student ID", StuID);
    cmd.Parse (argc, argv);
    
    
    Time::SetResolution (Time::NS);

    NodeContainer nodes;
    nodes.Create (4);
    NodeContainer AC=NodeContainer(nodes.Get(0),nodes.Get(2));
    NodeContainer BC=NodeContainer(nodes.Get(1),nodes.Get(2));
    NodeContainer CD=NodeContainer(nodes.Get(2),nodes.Get(3));
    
    
    PointToPointHelper pointToPoint;
    
    __gnu_cxx::string da=__gnu_cxx::to_string(StuID%10+1)+"Mbps";
    __gnu_cxx::string ca=__gnu_cxx::to_string(StuID%3+1)+"ms";
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue (da));
    pointToPoint.SetChannelAttribute ("Delay", StringValue (ca));

    NetDeviceContainer nets;
    NetDeviceContainer AACC=pointToPoint.Install(AC);
    NetDeviceContainer BBCC=pointToPoint.Install(BC);
    NetDeviceContainer CCDD=pointToPoint.Install(CD);
    pointToPoint.EnablePcapAll("HW1-1");

    InternetStackHelper stack;
    stack.Install (nodes);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces1 = address.Assign (AACC);
    address.SetBase ("10.1.2.0", "255.255.255.0");
    
    Ipv4InterfaceContainer interfaces2 = address.Assign (BBCC);
    
    address.SetBase ("10.1.3.0", "255.255.255.0");
    
    Ipv4InterfaceContainer interfaces3 = address.Assign (CCDD);
    UdpEchoServerHelper echoServer (7999);

    ApplicationContainer serverApps = echoServer.Install (nodes.Get (3));
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (Seconds (10.0));

    UdpEchoClientHelper echoClient (interfaces3.GetAddress (1),7999);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (3));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
    ApplicationContainer clientA = echoClient.Install (nodes.Get (0));
    ApplicationContainer clientB = echoClient.Install (nodes.Get (1));
    ApplicationContainer clientC = echoClient.Install (nodes.Get (2));

    
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    clientA.Start (Seconds (2));
    clientA.Stop (Seconds (10.0));
    clientB.Start (Seconds (2));
    clientB.Stop (Seconds (10.0));
    clientC.Start (Seconds (2));
    clientC.Stop (Seconds (10.0));
    NS_LOG_INFO ("Run Simulation.");
    Simulator::Run ();
    Simulator::Destroy ();
    NS_LOG_INFO ("Done.");
    return 0;
}
