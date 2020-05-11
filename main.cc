#include "work_improved.hpp"
#include <iostream>
#include "ns3/vector.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"

NS_LOG_COMPONENT_DEFINE("MyProject-Hello");

using namespace ns3;
using std::cerr;
using std::cout;
using std::endl;
using std::pair;
using std::string;
using std::to_string;
using std::vector;

const bool outputdebug = true;
const bool enablepacap = true;

// write msg to stdout
void debugout(string msg) {
    if (outputdebug)
        cout << "debug\t\t" << msg << endl
             << "------------------------------------------" << endl;
}

/*
uint64_t lasttotalrx = 0;
// 计算吞吐量函数，返回执行时刻 sink 的totalRx，需要手动更新
uint64_t calculateThroughput(Ptr<PacketSink> sink) {
    Time now = Simulator::Now();
    // Convert Application RX Packets to MBits
    double curthroughput = (sink->GetTotalRx() - lasttotalrx) * (double)8 / 1e5;
    cout << now.GetSeconds() << "s: \t" << curthroughput << "Mbit/s" << endl;
    uint64_t ret = sink->GetTotalRx();
    Simulator::Schedule(MilliSeconds(100), &calculateThroughput, sink);
    return ret;
} 
*/

/* 
// 使用flowmonitor统计吞吐量
void job(Ptr<FlowMonitor> flowmonitor) {
    flowmonitor->CheckForLostPackets();
    auto stats = flowmonitor->GetFlowStats();
    for (auto &&tmp : stats) {
        debugout(string("flowid: ") + to_string(tmp.first) + ", txBytes: " + to_string(tmp.second.txBytes));
    }
    Simulator::Schedule(MilliSeconds(300), &job, flowmonitor);
}; */

void work() {
    Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(true));
    // LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
    // LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    /* nodes */
    NodeContainer cNodes, aNodes, tNodes, nNodes;
    cNodes.Create(1), aNodes.Create(2), tNodes.Create(4), nNodes.Create(8);

    /* channels */
    // Point to Point Protocol(PPP)
    PointToPointHelper p2pc1a1, p2pc1a2;
    // CSMA=Ethernet: 0,1 -> a1, a2;  2,3,4,5 -> t1, t2, t3, t4;
    vector<CsmaHelper> csmas(6);
    {
        p2pc1a1.SetDeviceAttribute("DataRate", StringValue("1.5Mbps"));
        p2pc1a1.SetChannelAttribute("Delay", StringValue("500ns"));
        p2pc1a2.SetDeviceAttribute("DataRate", StringValue("1.5Mbps"));
        p2pc1a2.SetChannelAttribute("Delay", StringValue("500ns"));
        for (auto &&csmahelper : csmas) {
            csmahelper.SetChannelAttribute("DataRate", DataRateValue(DataRate("1Mbps")));
            csmahelper.SetChannelAttribute("Delay", StringValue("500ns"));
        }
    }
    /* netdevices */
    NetDeviceContainer ndevicesc1a1, ndevicesc1a2, ndevicesa1;
    NetDeviceContainer ndevicesa2, ndevicest1, ndevicest2, ndevicest3, ndevicest4;
    {
        ndevicesc1a1 = p2pc1a1.Install(cNodes.Get(0), aNodes.Get(0));
        ndevicesc1a2 = p2pc1a2.Install(cNodes.Get(0), aNodes.Get(1));
        ndevicesa1 = csmas[0].Install(NodeContainer(aNodes.Get(0), tNodes.Get(0), tNodes.Get(1)));
        ndevicesa2 = csmas[1].Install(NodeContainer(aNodes.Get(1), tNodes.Get(2), tNodes.Get(3)));
        ndevicest1 = csmas[2].Install(NodeContainer(tNodes.Get(0), nNodes.Get(0), nNodes.Get(1)));
        ndevicest2 = csmas[3].Install(NodeContainer(tNodes.Get(1), nNodes.Get(2), nNodes.Get(3)));
        ndevicest3 = csmas[4].Install(NodeContainer(tNodes.Get(2), nNodes.Get(4), nNodes.Get(5)));
        ndevicest4 = csmas[5].Install(NodeContainer(tNodes.Get(3), nNodes.Get(6), nNodes.Get(7)));
    }

    /* install inet stack */
    InternetStackHelper inetstack;
    inetstack.Install(cNodes), inetstack.Install(aNodes);
    inetstack.Install(tNodes), inetstack.Install(nNodes);

    /* assign ip -> interface */
    Ipv4InterfaceContainer ifc1a1, ifc1a2, ifa1, ifa2, ift1, ift2, ift3, ift4;
    Ipv4AddressHelper address;
    {
        // c1-a1
        address.SetBase("192.168.1.0", "255.255.255.0"), ifc1a1 = address.Assign(ndevicesc1a1);
        // c1-a2
        address.SetBase("192.168.2.0", "255.255.255.0"), ifc1a2 = address.Assign(ndevicesc1a2);
        // a1-t1t2
        address.SetBase("10.1.1.0", "255.255.255.0"), ifa1 = address.Assign(ndevicesa1);
        // a2-t3t4
        address.SetBase("10.2.1.0", "255.255.255.0"), ifa2 = address.Assign(ndevicesa2);
        // t1-n1n2
        address.SetBase("10.0.1.0", "255.255.255.0"), ift1 = address.Assign(ndevicest1);
        // t2-n3n4
        address.SetBase("10.0.2.0", "255.255.255.0"), ift2 = address.Assign(ndevicest2);
        // t3-n5n6
        address.SetBase("10.0.3.0", "255.255.255.0"), ift3 = address.Assign(ndevicest3);
        // t4-n7n8
        address.SetBase("10.0.4.0", "255.255.255.0"), ift4 = address.Assign(ndevicest4);
    }

    /* Application */

    // applications, 0:c;  1,2:a1,a2;  3~6:t1~t4; 7~14:n1~n8
    vector<ApplicationContainer> sinkapps(15), onOffapps(15);
    // 安装一个sink
    auto installsink = [&](int appindex, Ipv4Address sinkaddr, int16_t sinkport,
                           Ptr<Node> sinknode, double sinkstart, double sinkend) {
        PacketSinkHelper sinkhelper("ns3::TcpSocketFactory", InetSocketAddress(sinkaddr, sinkport));
        sinkapps[appindex] = sinkhelper.Install(sinknode);
        sinkapps[appindex].Start(Seconds(sinkstart));
        sinkapps[appindex].Stop(Seconds(sinkend));
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
        onoffhelper.SetAttribute("DataRate", StringValue("256Kbps"));
        onOffapps[appindex] = onoffhelper.Install(onoffnode);
        onOffapps[appindex].Start(Seconds(onoffstart));
        onOffapps[appindex].Stop(Seconds(onoffend));
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
            if (i == 0 || i == 1) address = ift1.GetAddress(i + 1);
            if (i == 2 || i == 3) address = ift2.GetAddress(i - 2 + 1);
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
        installsink(7 + 2, ift2.GetAddress(1), sinkport, nNodes.Get(2), sinkstart, sinkend);
        for (int i = 1; i <= 8; i++) {
            if (i == 3) continue;
            installonoff(i + 6, ift2.GetAddress(1), sinkport, nNodes.Get(i - 1), onoffstart + i * 3, onoffend,
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

    /* calculate throughout */

    // routing and pcap
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    // pacap
    if (enablepacap) {
        p2pc1a1.EnablePcapAll("p2p-c1a1");
        p2pc1a2.EnablePcapAll("p2p_c1a2");
        csmas[0].EnablePcapAll("csma_a1");
        csmas[1].EnablePcapAll("csma_a2");
        csmas[2].EnablePcapAll("csma_t1");
        csmas[3].EnablePcapAll("csma_t2");
        csmas[4].EnablePcapAll("csma_t3");
        csmas[5].EnablePcapAll("csma_t4");
    }

    Simulator::Run();
    Simulator::Destroy();
}

int main(int argc, char const *argv[]) {
    const int which = 1;
    if (which == 1)
        workimproved();
    else if (which == 2)
        work();
    else if (which == 3)
        test();
}
