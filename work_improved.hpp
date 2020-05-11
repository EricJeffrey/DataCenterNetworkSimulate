#include <iostream>
#include "ns3/vector.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"

using namespace ns3;
using std::cerr;
using std::cout;
using std::endl;
using std::pair;
using std::string;
using std::to_string;
using std::vector;

void workimproved() {
    Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(true));
    // Config::SetDefault("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue(true));
    // LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
    // LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    /* nodes */
    NodeContainer cNodes, aNodes, tNodes, nNodes;
    cNodes.Create(2), aNodes.Create(4), tNodes.Create(4), nNodes.Create(8);

    // vector<CsmaHelper> csmacs(2);
    // for (auto &&csma : csmacs) {
    //     csma.SetChannelAttribute("DataRate", DataRateValue(DataRate("1Mbps")));
    //     csma.SetChannelAttribute("Delay", StringValue("500ns"));
    // }

    // 0,1: c1-a1,c1-a3; 2,4: c2-a2,c2-a4
    vector<PointToPointHelper> p2pcs(4);
    for (auto &&p2p : p2pcs) {
        p2p.SetDeviceAttribute("DataRate", StringValue("1.5Mbps"));
        p2p.SetChannelAttribute("Delay", StringValue("500ns"));
    }
    vector<CsmaHelper> csmas(8);
    for (auto &&csma : csmas) {
        csma.SetChannelAttribute("DataRate", DataRateValue(DataRate("1Mbps")));
        csma.SetChannelAttribute("Delay", StringValue("500ns"));
    }
    vector<NetDeviceContainer> ndevcs(4), ndevas(4), ndevts(4);
    // ndevcs[0] = csmacs[0].Install(NodeContainer(cNodes.Get(0), aNodes.Get(0), aNodes.Get(2)));
    // ndevcs[1] = csmacs[1].Install(NodeContainer(cNodes.Get(0), aNodes.Get(1), aNodes.Get(3)));
    ndevcs[0] = p2pcs[0].Install(NodeContainer(cNodes.Get(0), aNodes.Get(0)));
    ndevcs[1] = p2pcs[1].Install(NodeContainer(cNodes.Get(0), aNodes.Get(2)));
    ndevcs[2] = p2pcs[2].Install(NodeContainer(cNodes.Get(1), aNodes.Get(1)));
    ndevcs[3] = p2pcs[3].Install(NodeContainer(cNodes.Get(1), aNodes.Get(3)));

    ndevas[0] = csmas[0].Install(NodeContainer(aNodes.Get(0), tNodes.Get(0), tNodes.Get(1)));
    ndevas[1] = csmas[1].Install(NodeContainer(aNodes.Get(1), tNodes.Get(0), tNodes.Get(1)));
    ndevas[2] = csmas[2].Install(NodeContainer(aNodes.Get(2), tNodes.Get(2), tNodes.Get(3)));
    ndevas[3] = csmas[3].Install(NodeContainer(aNodes.Get(3), tNodes.Get(2), tNodes.Get(3)));

    ndevts[0] = csmas[4].Install(NodeContainer(tNodes.Get(0), nNodes.Get(0), nNodes.Get(1)));
    ndevts[1] = csmas[5].Install(NodeContainer(tNodes.Get(1), nNodes.Get(2), nNodes.Get(3)));
    ndevts[2] = csmas[6].Install(NodeContainer(tNodes.Get(2), nNodes.Get(4), nNodes.Get(5)));
    ndevts[3] = csmas[7].Install(NodeContainer(tNodes.Get(3), nNodes.Get(6), nNodes.Get(7)));

    InternetStackHelper inetstack;
    inetstack.Install(cNodes), inetstack.Install(aNodes);
    inetstack.Install(tNodes), inetstack.Install(nNodes);

    const char *tmps = "255.255.255.0";
    vector<Ipv4InterfaceContainer> ifcs(4), ifas(4), ifts(4);
    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", tmps), ifcs[0] = address.Assign(ndevcs[0]);
    address.SetBase("192.168.2.0", tmps), ifcs[1] = address.Assign(ndevcs[1]);
    address.SetBase("192.168.3.0", tmps), ifcs[2] = address.Assign(ndevcs[2]);
    address.SetBase("192.168.4.0", tmps), ifcs[3] = address.Assign(ndevcs[3]);
    address.SetBase("10.1.1.0", tmps), ifas[0] = address.Assign(ndevas[0]);
    address.SetBase("10.2.1.0", tmps), ifas[1] = address.Assign(ndevas[1]);
    address.SetBase("10.3.1.0", tmps), ifas[2] = address.Assign(ndevas[2]);
    address.SetBase("10.4.1.0", tmps), ifas[3] = address.Assign(ndevas[3]);
    address.SetBase("10.0.1.0", tmps), ifts[0] = address.Assign(ndevts[0]);
    address.SetBase("10.0.2.0", tmps), ifts[1] = address.Assign(ndevts[1]);
    address.SetBase("10.0.3.0", tmps), ifts[2] = address.Assign(ndevts[2]);
    address.SetBase("10.0.4.0", tmps), ifts[3] = address.Assign(ndevts[3]);

    // 安装一个sink
    auto installsink = [&](int appindex, Ipv4Address sinkaddr, int16_t sinkport,
                           Ptr<Node> sinknode, double sinkstart, double sinkend) {
        PacketSinkHelper sinkhelper("ns3::TcpSocketFactory", InetSocketAddress(sinkaddr, sinkport));
        ApplicationContainer app = sinkhelper.Install(sinknode);
        app.Start(Seconds(sinkstart));
        app.Stop(Seconds(sinkend));
    };
    // 安装一个onoff
    auto installonoff = [&](int appindex,
                            Ipv4Address sinkaddr, int16_t sinkport,
                            Ptr<Node> onoffnode, double onoffstart, double onoffend,
                            double ontime, double offtime, uint64_t packetsize) {
        OnOffHelper onoffhelper("ns3::TcpSocketFactory", InetSocketAddress(sinkaddr, sinkport));
        onoffhelper.SetAttribute("OnTime", StringValue(string("ns3::ConstantRandomVariable[Constant=") + std::to_string(ontime) + "]"));
        onoffhelper.SetAttribute("OffTime", StringValue(string("ns3::ConstantRandomVariable[Constant=") + std::to_string(offtime) + "]"));
        onoffhelper.SetAttribute("PacketSize", UintegerValue(packetsize));
        onoffhelper.SetAttribute("DataRate", StringValue("512Kbps"));
        ApplicationContainer app = onoffhelper.Install(onoffnode);
        app.Start(Seconds(onoffstart));
        app.Stop(Seconds(onoffend));
    };

    // inter-cluster, 4 sink, 4 onoff
    auto pattern1 = [&]() {
        int16_t sinkport = 8080;
        int sinkstart = 0, sinkend = 45;
        int onoffstart = 1, onoffend = 40, ontime = 50, offtime = 0;
        uint64_t packetsize = 4096;
        // sink on n1~n4, onoff on n5~n8
        for (int i = 0; i < 4; i++) {
            Ipv4Address address;
            if (i == 0 || i == 1) address = ifts[0].GetAddress(i + 1);
            if (i == 2 || i == 3) address = ifts[1].GetAddress(i - 2 + 1);
            installsink(i + 7, address, sinkport, nNodes.Get(i), sinkstart + i * 3, sinkend);
            installonoff(i + 11, address, sinkport, nNodes.Get(i + 4), onoffstart + i * 3, onoffend, ontime, offtime, packetsize);
        }
    };

    // many-to-one
    auto pattern2 = [&]() {
        // sink on n3, onoff on n1~n8
        int16_t sinkport = 8080;
        int sinkstart = 0, sinkend = 45;
        int onoffstart = 1, onoffend = 40, ontime = 50, offtime = 0;
        uint64_t packetsize = 4096;
        installsink(7 + 2, ifts[1].GetAddress(1), sinkport, nNodes.Get(2), sinkstart, sinkend);
        for (int i = 1; i <= 8; i++) {
            if (i == 3) continue;
            installonoff(i + 6, ifts[1].GetAddress(1), sinkport, nNodes.Get(i - 1), onoffstart + i * 3, onoffend,
                         ontime, offtime, packetsize);
        }
    };

    const int whichtoexe = 2;
    switch (whichtoexe) {
    case 1:
        pattern1();
        break;
    case 2:
        pattern2();
        break;
    }
    // routing and pcap
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    // pacap
    // p2pcs[0].EnablePcapAll("csma_c1a1");
    // p2pcs[1].EnablePcapAll("csma_c1a3");
    // p2pcs[2].EnablePcapAll("csma_c2a2");
    // p2pcs[3].EnablePcapAll("csma_c2a4");
    // csmas[0].EnablePcapAll("csma_a1");
    // csmas[1].EnablePcapAll("csma_a2");
    // csmas[2].EnablePcapAll("csma_a3");
    // csmas[3].EnablePcapAll("csma_a4");
    // csmas[4].EnablePcapAll("csma_t1");
    csmas[5].EnablePcapAll("csma_t2");
    // csmas[6].EnablePcapAll("csma_t3");
    // csmas[7].EnablePcapAll("csma_t4");

    Simulator::Run();
    Simulator::Destroy();
}

void test() {
    Config::SetDefault("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue(true));
    Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(true));

    int pattern = 1;
    //pattern 1 :inter-cluster
    //pattern 2 :many-to-one
    //printf("input pattern number :\n");
    //scanf("%d\n",pattern);

    //最底层8个server
    NodeContainer server1;
    server1.Create(1);
    NodeContainer server2;
    server2.Create(1);
    NodeContainer server3;
    server3.Create(1);
    NodeContainer server4;
    server4.Create(1);
    NodeContainer server5;
    server5.Create(1);
    NodeContainer server6;
    server6.Create(1);
    NodeContainer server7;
    server7.Create(1);
    NodeContainer server8;
    server8.Create(1);
    //底层4个网络
    NodeContainer btmLayerNodes1;
    btmLayerNodes1.Create(1);
    btmLayerNodes1.Add(server1);
    btmLayerNodes1.Add(server2);
    NodeContainer btmLayerNodes2;
    btmLayerNodes2.Create(1);
    btmLayerNodes2.Add(server3);
    btmLayerNodes2.Add(server4);
    NodeContainer btmLayerNodes3;
    btmLayerNodes3.Create(1);
    btmLayerNodes3.Add(server5);
    btmLayerNodes3.Add(server6);
    NodeContainer btmLayerNodes4;
    btmLayerNodes4.Create(1);
    btmLayerNodes4.Add(server7);
    btmLayerNodes4.Add(server8);
    //中层网络
    NodeContainer midLayerNodes1;
    midLayerNodes1.Create(1);
    midLayerNodes1.Add(btmLayerNodes1.Get(0));
    midLayerNodes1.Add(btmLayerNodes2.Get(0));
    NodeContainer midLayerNodes2;
    midLayerNodes2.Create(1);
    midLayerNodes2.Add(btmLayerNodes1.Get(0));
    midLayerNodes2.Add(btmLayerNodes2.Get(0));
    NodeContainer midLayerNodes3;
    midLayerNodes3.Create(1);
    midLayerNodes3.Add(btmLayerNodes3.Get(0));
    midLayerNodes3.Add(btmLayerNodes4.Get(0));
    NodeContainer midLayerNodes4;
    midLayerNodes4.Create(1);
    midLayerNodes4.Add(btmLayerNodes3.Get(0));
    midLayerNodes4.Add(btmLayerNodes4.Get(0));
    //顶层网络
    NodeContainer topLayerNodes1, topLayerNodes2;
    topLayerNodes1.Create(1);
    topLayerNodes1.Add(midLayerNodes1.Get(0));
    topLayerNodes1.Add(midLayerNodes3.Get(0));
    topLayerNodes2.Create(1);
    topLayerNodes2.Add(midLayerNodes2.Get(0));
    topLayerNodes2.Add(midLayerNodes4.Get(0));
    // NS_LOG_INFO("NODES DONE");
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("1.0Mbps"));
    csma.SetChannelAttribute("Delay", StringValue("500ns"));
    //top
    NetDeviceContainer topLayerDevices1, topLayerDevices2;
    topLayerDevices1 = csma.Install(topLayerNodes1);
    topLayerDevices2 = csma.Install(topLayerNodes2);
    //中层
    NetDeviceContainer midLayerDevices1, midLayerDevices2;
    NetDeviceContainer midLayerDevices3, midLayerDevices4;
    midLayerDevices1 = csma.Install(midLayerNodes1);
    midLayerDevices2 = csma.Install(midLayerNodes2);
    midLayerDevices3 = csma.Install(midLayerNodes3);
    midLayerDevices4 = csma.Install(midLayerNodes4);
    //底层
    NetDeviceContainer btmLayerDevice1, btmLayerDevice2, btmLayerDevice3, btmLayerDevice4;
    btmLayerDevice1 = csma.Install(btmLayerNodes1);
    btmLayerDevice2 = csma.Install(btmLayerNodes2);
    btmLayerDevice3 = csma.Install(btmLayerNodes3);
    btmLayerDevice4 = csma.Install(btmLayerNodes4);
    // NS_LOG_INFO("CSMA DONE");
    // NS_LOG_INFO("DEVICE DONE");
    //安装栈
    InternetStackHelper stack;
    stack.Install(btmLayerNodes1);
    stack.Install(btmLayerNodes2);
    stack.Install(btmLayerNodes3);
    stack.Install(btmLayerNodes4);
    stack.Install(midLayerNodes1.Get(0));
    stack.Install(midLayerNodes2.Get(0));
    stack.Install(midLayerNodes3.Get(0));
    stack.Install(midLayerNodes4.Get(0));
    stack.Install(topLayerNodes1.Get(0));
    stack.Install(topLayerNodes2.Get(0));
    // NS_LOG_INFO("DEVICE STACK DONE");
    //设置IP
    Ipv4AddressHelper address;
    //底层
    Ipv4InterfaceContainer btmInterface1, btmInterface2, btmInterface3, btmInterface4;
    address.SetBase("10.0.1.0", "255.255.255.0");
    btmInterface1 = address.Assign(btmLayerDevice1);
    address.SetBase("10.0.2.0", "255.255.255.0");
    btmInterface2 = address.Assign(btmLayerDevice2);
    address.SetBase("10.0.3.0", "255.255.255.0");
    btmInterface3 = address.Assign(btmLayerDevice3);
    address.SetBase("10.0.4.0", "255.255.255.0");
    btmInterface4 = address.Assign(btmLayerDevice4);
    // NS_LOG_INFO("BTM ASSIGN DONE");
    //中层
    Ipv4InterfaceContainer midInterface1, midInterface2, midInterface3, midInterface4;
    address.SetBase("10.1.1.0", "255.255.255.0");
    midInterface1 = address.Assign(midLayerDevices1);
    address.SetBase("10.2.1.0", "255.255.255.0");
    midInterface2 = address.Assign(midLayerDevices2);
    address.SetBase("10.3.1.0", "255.255.255.0");
    midInterface3 = address.Assign(midLayerDevices3);
    address.SetBase("10.4.1.0", "255.255.255.0");
    midInterface4 = address.Assign(midLayerDevices4);
    // NS_LOG_INFO("MID ASSIGN DONE");
    //顶层
    Ipv4InterfaceContainer topInterface1, topInterface2;
    address.SetBase("192.168.1.0", "255.255.255.0");
    topInterface1 = address.Assign(topLayerDevices1);
    address.SetBase("192.168.2.0", "255.255.255.0");
    topInterface2 = address.Assign(topLayerDevices2);
    // NS_LOG_INFO("ASSIGN DONE");
    //  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    int16_t sinkPort = 8080;

    if (pattern == 1) {
        // NS_LOG_INFO("inter-cluster pattern");
        //布置8个server
        //server1
        Address sinkAddress1(InetSocketAddress(btmInterface1.GetAddress(1), sinkPort));
        PacketSinkHelper packetSinkHelper1("ns3::TcpSocketFactory", sinkAddress1);
        ApplicationContainer sinkApp1 = packetSinkHelper1.Install(server1);
        sinkApp1.Start(Seconds(0.0));
        sinkApp1.Stop(Seconds(5.0));
        // NS_LOG_INFO("SERVER1 DONE");
        //server2
        Address sinkAddress2(InetSocketAddress(btmInterface1.GetAddress(2), sinkPort));
        PacketSinkHelper packetSinkHelper2("ns3::TcpSocketFactory", sinkAddress2);
        ApplicationContainer sinkApp2 = packetSinkHelper2.Install(server2);
        sinkApp2.Start(Seconds(0.0));
        sinkApp2.Stop(Seconds(5.0));
        //server3
        Address sinkAddress3(InetSocketAddress(btmInterface2.GetAddress(1), sinkPort));
        PacketSinkHelper packetSinkHelper3("ns3::TcpSocketFactory", sinkAddress3);
        ApplicationContainer sinkApp3 = packetSinkHelper3.Install(server3);
        sinkApp3.Start(Seconds(0.0));
        sinkApp3.Stop(Seconds(5.0));
        //server4
        Address sinkAddress4(InetSocketAddress(btmInterface2.GetAddress(2), sinkPort));
        PacketSinkHelper packetSinkHelper4("ns3::TcpSocketFactory", sinkAddress4);
        ApplicationContainer sinkApp4 = packetSinkHelper4.Install(server4);
        sinkApp4.Start(Seconds(0.0));
        sinkApp4.Stop(Seconds(5.0));
        //server5
        Address sinkAddress5(InetSocketAddress(btmInterface3.GetAddress(1), sinkPort));
        PacketSinkHelper packetSinkHelper5("ns3::TcpSocketFactory", sinkAddress5);
        ApplicationContainer sinkApp5 = packetSinkHelper5.Install(server5);
        sinkApp5.Start(Seconds(0.0));
        sinkApp5.Stop(Seconds(5.0));
        //server6
        Address sinkAddress6(InetSocketAddress(btmInterface3.GetAddress(2), sinkPort));
        PacketSinkHelper packetSinkHelper6("ns3::TcpSocketFactory", sinkAddress6);
        ApplicationContainer sinkApp6 = packetSinkHelper6.Install(server6);
        sinkApp6.Start(Seconds(0.0));
        sinkApp6.Stop(Seconds(5.0));
        //server7
        Address sinkAddress7(InetSocketAddress(btmInterface4.GetAddress(1), sinkPort));
        PacketSinkHelper packetSinkHelper7("ns3::TcpSocketFactory", sinkAddress7);
        ApplicationContainer sinkApp7 = packetSinkHelper7.Install(server7);
        sinkApp7.Start(Seconds(0.0));
        sinkApp7.Stop(Seconds(5.0));
        //server8
        Address sinkAddress8(InetSocketAddress(btmInterface4.GetAddress(2), sinkPort));
        PacketSinkHelper packetSinkHelper8("ns3::TcpSocketFactory", sinkAddress8);
        ApplicationContainer sinkApp8 = packetSinkHelper8.Install(server8);
        sinkApp8.Start(Seconds(0.0));
        sinkApp8.Stop(Seconds(5.0));
        // NS_LOG_INFO("SERVERS DONE");

        //server1 -> server5
        ApplicationContainer clientApp1;
        OnOffHelper client1("ns3::TcpSocketFactory", InetSocketAddress(btmInterface3.GetAddress(1), sinkPort));
        // NS_LOG_INFO("TEST1");
        client1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=50]"));
        client1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        // NS_LOG_INFO("TEST2");
        client1.SetAttribute("DataRate", DataRateValue(DataRate("1.0Mbps")));
        client1.SetAttribute("PacketSize", UintegerValue(2000));
        // NS_LOG_INFO("ATTRIBUTES DONE");
        clientApp1 = client1.Install(server1);
        clientApp1.Start(Seconds(1));
        clientApp1.Stop(Seconds(4));
        // NS_LOG_INFO("SERVER1->SERVER5 DONE");
        //server6 -> server2
        ApplicationContainer clientApp2;
        OnOffHelper client2("ns3::TcpSocketFactory", InetSocketAddress(btmInterface1.GetAddress(2), sinkPort));
        client2.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=50]"));
        client2.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        client2.SetAttribute("DataRate", DataRateValue(DataRate("1.0Mbps")));
        client2.SetAttribute("PacketSize", UintegerValue(2000));
        clientApp2 = client2.Install(server6);
        clientApp2.Start(Seconds(1));
        clientApp2.Stop(Seconds(4));
        // NS_LOG_INFO("SERVER6->SERVER2 DONE");
        //server3 -> server7
        ApplicationContainer clientApp3;
        OnOffHelper client3("ns3::TcpSocketFactory", InetSocketAddress(btmInterface4.GetAddress(1), sinkPort));
        client3.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=50]"));
        client3.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        client3.SetAttribute("DataRate", DataRateValue(DataRate("1.0Mbps")));
        client3.SetAttribute("PacketSize", UintegerValue(2000));
        clientApp3 = client3.Install(server3);
        clientApp3.Start(Seconds(1));
        clientApp3.Stop(Seconds(4));
        //server8 -> server4
        ApplicationContainer clientApp4;
        OnOffHelper client4("ns3::TcpSocketFactory", InetSocketAddress(btmInterface2.GetAddress(2), sinkPort));
        client4.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=50]"));
        client4.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        client4.SetAttribute("DataRate", DataRateValue(DataRate("1.0Mbps")));
        client4.SetAttribute("PacketSize", UintegerValue(2000));
        clientApp4 = client4.Install(server8);
        clientApp4.Start(Seconds(1));
        clientApp4.Stop(Seconds(4));
        // NS_LOG_INFO("MANY TO MANY DONE");

        //	pointToPoint.EnablePcapAll("project1_inter_cluster");
        csma.EnablePcap("ic_1", server1, false);
        csma.EnablePcap("ic_2", server2, false);
        csma.EnablePcap("ic_3", server3, false);
        csma.EnablePcap("ic_4", server4, false);
        csma.EnablePcap("ic_5", server5, false);
        csma.EnablePcap("ic_6", server6, false);
        csma.EnablePcap("ic_7", server7, false);
        csma.EnablePcap("ic_8", server8, false);

    } else if (pattern == 2) {
        // NS_LOG_INFO("many to one pattern");
        //全发给server1
        Address sinkAddress(InetSocketAddress(btmInterface1.GetAddress(1), sinkPort));
        PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", sinkAddress);
        ApplicationContainer sinkApp;
        sinkApp = packetSinkHelper.Install(server1);
        sinkApp.Start(Seconds(1.0));
        sinkApp.Stop(Seconds(60.0));
        ApplicationContainer clientApp1, clientApp2, clientApp3, clientApp4, clientApp5, clientApp6, clientApp7;
        OnOffHelper client("ns3::TcpSocketFactory", InetSocketAddress(btmInterface1.GetAddress(1), sinkPort));
        client.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=50]"));
        client.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        client.SetAttribute("DataRate", DataRateValue(DataRate("1.5Mbps")));
        client.SetAttribute("PacketSize", UintegerValue(2000));
        clientApp1 = client.Install(server2);
        clientApp1.Start(Seconds(2));
        clientApp1.Stop(Seconds(50));
        clientApp2 = client.Install(server3);
        clientApp2.Start(Seconds(3));
        clientApp2.Stop(Seconds(50));
        clientApp3 = client.Install(server4);
        clientApp3.Start(Seconds(4));
        clientApp3.Stop(Seconds(50));
        clientApp4 = client.Install(server5);
        clientApp4.Start(Seconds(5));
        clientApp4.Stop(Seconds(50));
        clientApp5 = client.Install(server6);
        clientApp5.Start(Seconds(6));
        clientApp5.Stop(Seconds(50));
        clientApp6 = client.Install(server7);
        clientApp6.Start(Seconds(7));
        clientApp6.Stop(Seconds(50));
        clientApp7 = client.Install(server8);
        clientApp7.Start(Seconds(8));
        clientApp7.Stop(Seconds(50));

        //	pointToPoint.EnablePcapAll("project1_many_to_one");
        csma.EnablePcap("many_to_one_server1", server1, true);
    }
    //pointToPoint.EnablePcapAll("project1");
    //csma.EnablePcapAll("project1");
    //Simulator::Stop(Seconds(60));
    Simulator::Run();
    Simulator::Destroy();
}
