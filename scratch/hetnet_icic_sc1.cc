/*
 * macro_pico_scenario.cc
 *
 *  Created on: 1 Feb 2014
 *      Author: dermot
 */


#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/log.h"
#include <sys/timeb.h>
#include <iostream>
#include <ns3/internet-trace-helper.h>
#include "ns3/netanim-module.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/constant-velocity-mobility-model.h"
#include <string>
#include "ns3/radio-bearer-stats-calculator.h"



using namespace ns3;
using namespace std;

string NumberToString(int num);
void DisplayCurrentTime(void);
int getMilliSpan(int nTimeStart);
int getMilliCount();

int simTime = 30;

NS_LOG_COMPONENT_DEFINE("myglobalrouting");

int main(int argc, char *argv[])
{
	int PeNB_radius = 40;
	int number_PeNB = 3;
	int number_MeNB = 1;
	int number_PUEs = 4;		//number of UEs per PeNB
	int number_MUEs = 4;
	int number_enbs = number_PeNB + number_MeNB;
	int number_UEs = (number_PeNB * number_PUEs) + (number_MeNB * number_MUEs);
	int interPacketInterval = 100;
	vector<uint16_t> neighbourNodes(number_PeNB);

	//Config::SetDefault("ns3::LteEnbRrc::DefaultTransmissionMode", UintegerValue(5));	// Activates MIMO Model

	//Helper Modules
	Ptr <LteHelper> lteHelper = CreateObject<LteHelper> ();	//LTE Helper module
	Ptr<EpcHelper> epcHelper = CreateObject<EpcHelper> ();	//EPC Helper Module - EPC Network provides a means for simulation of end-to-end IP connectivity over the lte model
	lteHelper->SetEpcHelper(epcHelper);
	lteHelper->SetSchedulerType("ns3::PfFfMenbMacScheduler");
	lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::FriisSpectrumPropagationLossModel"));
	MobilityHelper ENBmobility[2], mobility;							//Array slot 0 for MeNB mobility, Array slot 1 for PeNB mobilit


	// Create Protocol Stacks
	Ipv4StaticRoutingHelper ipv4RoutingHelper;

	//Define the pgw/RemoteHost nodes
	Ptr<Node> pgw = epcHelper->GetPgwNode();
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.Install(pgw);

	//Create a single remoteHost
	NodeContainer remoteHostContainer;
	remoteHostContainer.Create(1);
	Ptr<Node> remoteHost = remoteHostContainer.Get(0);

	//Install the internet protocol stack on the remotehost
	InternetStackHelper internet;
	internet.Install(remoteHostContainer);

	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.Install(remoteHostContainer);

	PointToPointHelper p2p;
	p2p.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
	p2p.SetDeviceAttribute("Mtu", UintegerValue(1500));
	p2p.SetChannelAttribute("Delay", TimeValue(MilliSeconds(1)));

	//Creates Devices for the pgw/remoteHost nodes, and installs the p2p network on them
	NetDeviceContainer internetDevices = p2p.Install(pgw, remoteHost);

	Ipv4AddressHelper ipv4h;
	ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
	Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
	// interface 0 is localhost, 1 is the p2p device
	Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

	Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
	remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

	NodeContainer MeNBnodes, PeNBnodes; //Create MeNB and PeNB node containers
	NodeContainer UEnodes[number_enbs];
	cout << "Ip Address of remoteHost" << remoteHostAddr.Get()  << endl;
	MeNBnodes.Create(number_MeNB);
	PeNBnodes.Create(number_PeNB);

	for (int i=0;i<number_PeNB;i++)
	{
		UEnodes[i].Create(number_PUEs);
		if (i==number_PeNB-1)
		{
			UEnodes[i+1].Create(number_MUEs);
		}

	}
	cout << "ENB and UE Nodes created " << endl;

	MobilityHelper UEmobility[number_enbs];	//Sets up  mobility models for each set of PeNB and MeN UEs

	//Set mobility model of the PeNB and MeNB nodes
	Ptr<ListPositionAllocator> MacropositionAlloc = CreateObject<ListPositionAllocator> ();
	MacropositionAlloc->Add(Vector(150.0, 150.0, 10.0));

	ENBmobility[0].SetPositionAllocator(MacropositionAlloc);
	ENBmobility[0].SetMobilityModel("ns3::ConstantPositionMobilityModel");
	ENBmobility[0].Install (MeNBnodes);

	Vector3D PeNBposition[number_PeNB];		//Creates a vector that will store the coordinates of each PeNB
	Vector MeNBposition = MeNBnodes.Get(0)->GetObject<MobilityModel>()->GetPosition();

	//SET UP X,Y,Z COORDINATES FOR THE PICO BASE STATIONS (REFINE THIS)
	for(int i=0;i<number_PeNB;i++)
	{
		PeNBposition[i].z = 10;
		if (i-1>-1)
		{
			PeNBposition[i].x = (MeNBposition.x)  + (PeNB_radius);
			PeNBposition[i].y= (MeNBposition.y + PeNBposition[i-1].y) * 0.5 + (1*PeNB_radius);
		}
		else	//ONLY EXECUTED FOR THE FIRST PICO NODE
		{
			PeNBposition[i].x = (MeNBposition.x)  + (PeNB_radius);
			PeNBposition[i].y = (MeNBposition.y * 0.5) + PeNB_radius;
		}
	}
	//_______________________________________________________________


	// Set up PeNB mobility models and install the model onto the nodes
	for(uint32_t i=0; i<PeNBnodes.GetN();i++){
		Ptr<ListPositionAllocator> PicopositionAlloc = CreateObject<ListPositionAllocator> ();

		PicopositionAlloc->Add(PeNBposition[i]);

		ENBmobility[1].SetMobilityModel("ns3::ConstantPositionMobilityModel");
		ENBmobility[1].SetPositionAllocator(PicopositionAlloc);
		ENBmobility[1].Install(PeNBnodes.Get(i));
	}
	cout << "ENB mobility models set up"<< endl;

	//Set up the mobility model for the UEs
	for(int i=0; i<number_enbs;i++)
	{
		if (i >= number_PeNB)
		{
			UEmobility[i].SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
						                   "X", StringValue(NumberToString(MeNBposition.x)),
						                   "Y", StringValue(NumberToString(MeNBposition.y)),
						                   "Rho", StringValue ("ns3::UniformRandomVariable[Min=-250|Max=250]"));
						                   //RandomDiscPositionAllocator randomly sets a position for each UE within a disc centred at (160,160) with radius 160
			UEmobility[i].SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
						                    "Mode", StringValue ("Time"),				//The condition to change speed and direction (either distance or time)
						                    "Time", StringValue ("5s"),					//Delay defines how long until the UE changes direction and speed
						                    "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=3]"),	//speed in m/s
						                    "Bounds", StringValue ("-400|400|-400|400"));		//Sets the region in which the UEs may move
			UEmobility[i].Install (UEnodes[i]);	//Apply the mobility model to the UEs
		}
		else if (i<number_PeNB)
		{
			UEmobility[i].SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
												"X", StringValue(NumberToString(PeNBposition[i].x)),
												"Y", StringValue(NumberToString(PeNBposition[i].y)),
												"Rho", StringValue ("ns3::UniformRandomVariable[Min=-40|Max=40]"));
												//RandomDiscPositionAllocator randomly sets a position for each UE within a disc centred at (160,160) with radius 160
			UEmobility[i].SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
											"Mode", StringValue ("Time"),				//The condition to change speed and direction (either distance or time)
											"Time", StringValue ("5s"),					//Delay defines how long until the UE changes direction and speed
											"Speed", StringValue ("ns3::ConstantRandomVariable[Constant=3]"),	//speed in m/s
											"Bounds", StringValue ("-400|400|-400|400"));		//Sets the region in which the UEs may move
			UEmobility[i].Install (UEnodes[i]);	//Apply the mobility model to the UEs
		}
	}


	//CREATE NET DEVICE CONTAINERS
	NetDeviceContainer MeNBDevs = lteHelper->InstallEnbDevice (MeNBnodes);
	//NetDeviceContainer PeNBDevs = lteHelper->InstallEnbDevice (PeNBnodes);
	NetDeviceContainer PeNBDevs = lteHelper->InstallPEnbDevice (PeNBnodes, MeNBnodes, true);
	NetDeviceContainer UEDevs[number_UEs];

	//Set up DataRadioBearer
	Ipv4InterfaceContainer ueIpIface[number_UEs];

	//Install UE Devices onto UE nodes, and attach to PeNB
	for(int i = 0; i < number_enbs; i++)
	{
		UEDevs[i] = lteHelper->InstallUeDevice(UEnodes[i]);
		internet.Install(UEnodes[i]);
		ueIpIface[i] = epcHelper->AssignUeIpv4Address (NetDeviceContainer (UEDevs[i]));

		for (uint32_t j = 0; j < UEnodes[i].GetN (); j++)
		{
			lteHelper->ActivateDedicatedEpsBearer(UEDevs[i].Get(j), EpsBearer(EpsBearer::GBR_CONV_VIDEO), EpcTft::Default());
			Ptr<Node> ueNode = UEnodes[i].Get (j);

		      // Set the default gateway for the UE
		      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
		      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
		      if (i < number_PeNB)
		      {
		      	  lteHelper->Attach (UEDevs[i].Get(j), PeNBDevs.Get(i));
		      }
		      else if (i>=number_PeNB)
		      {
		    	  lteHelper->Attach (UEDevs[i].Get(j), MeNBDevs.Get(0));	//Attach UEs to the MeNB
		      }
		}
	}
	cout << "UE Devices installed " << endl;

	Ptr<Node> MeNBnode1 = MeNBnodes.Get(0);
	for(uint32_t i=0; i <PeNBnodes.GetN();i++)
	{
		//Set up an X2 interface between PeNBs and MeNB
		Ptr<Node> PeNBnode1 = PeNBnodes.Get(i);
		neighbourNodes[i] = PeNBnode1->GetId();
		lteHelper->AddX2Interface(MeNBnode1, PeNBnode1);

		//Sets up an X2 interface between PeNBs
		if(i+1 < PeNBnodes.GetN())
		{
			Ptr<Node> PeNBnode2 = PeNBnodes.Get(i+1);		//Get the next PeNB
			lteHelper->AddX2Interface(PeNBnode1, PeNBnode2);

		}
	}


	//MeNB Physical Layer Control
	Ptr<LteEnbNetDevice> LteMeNBDev = MeNBDevs.Get(0)->GetObject<LteEnbNetDevice>();
	Ptr<LteEnbPhy> MeNBPhy = LteMeNBDev->GetPhy();
	MeNBPhy->SetAttribute("TxPower", DoubleValue(46.0));


	Ptr<LteEnbRrc> MeNBrrc = LteMeNBDev->GetRrc();
	MeNBrrc->setNeighbourNodes(neighbourNodes);


	//Ptr<LteEnbMac> MeNBmac = LteMeNBDev->GetMac();
	//PeNB Physical Layer Control
	Ptr <LteEnbNetDevice> LtePeNBDev[PeNBnodes.GetN()];
	Ptr<LteEnbPhy> PeNBPhy[PeNBnodes.GetN()];
	for(uint32_t i=0; i<PeNBnodes.GetN();i++)
	{
		LtePeNBDev[i] = PeNBDevs.Get(i)->GetObject<LteEnbNetDevice>();
		PeNBPhy[i] = LtePeNBDev[i]->GetPhy();
		PeNBPhy[i]->SetAttribute("TxPower", DoubleValue(30.0));
	}

	//Install and start applications on the UEs and the remoteHost
	cout << "Installing Applications" << endl;
	uint16_t dlPort = 1234;

	uint16_t ulPort = 2000;
	uint16_t otherPort = 3000;
	ApplicationContainer clientApps;
	ApplicationContainer serverApps;

	for (int i=0; i<number_enbs;i++)
	{
		for (uint32_t j = 0; j < UEnodes[i].GetN (); j++)
		{
		    ++ulPort;
		    ++otherPort;
			PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
			PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
			PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort));

		    serverApps.Add (dlPacketSinkHelper.Install (UEnodes[i].Get(j)));		//Receive packets from the remote host
			serverApps.Add (ulPacketSinkHelper.Install (remoteHost));				//Receive packets from the ues
			serverApps.Add (packetSinkHelper.Install (UEnodes[i].Get(j)));

			 NS_LOG_LOGIC ("installing UDP DL app for UE " << i);
			 UdpClientHelper dlClient (ueIpIface[i].GetAddress (j), dlPort);					//Uplink packet generator
			 dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
			 dlClient.SetAttribute ("MaxPackets", UintegerValue(1000000));

	          NS_LOG_LOGIC ("installing UDP UL app for UE " << i);
			 UdpClientHelper ulClient (remoteHostAddr, ulPort);									//downlink packet generator
			 ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
			 ulClient.SetAttribute ("MaxPackets", UintegerValue(1000000));


		      UdpClientHelper client (ueIpIface[i].GetAddress (j), otherPort);
		      client.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
		      client.SetAttribute ("MaxPackets", UintegerValue(1000000));


			 clientApps.Add (dlClient.Install (remoteHost));
			 clientApps.Add (ulClient.Install (UEnodes[i].Get(j)));


			 if ((i+1< number_enbs) && (j+1 < UEnodes[i].GetN ()))
			{
				clientApps.Add (client.Install (UEnodes[i+1].Get(j+1)));
		    }
     		  else //if ((i+1 > number_enbs) && (j+1 > UEnodes[i].GetN ()))
     		  {
				clientApps.Add (client.Install (UEnodes[i].Get(0)));
     		  }


		}
	}


	serverApps.Start(Seconds(0.001));
	clientApps.Start(Seconds(0.001));
	lteHelper->EnableTraces();

	std::ostringstream tag;
	cout << "Starting Simulation" << endl;

	/*****************************************************************************************************************
	 *
	 * Configure FlowMonitor
	 */
	FlowMonitorHelper flowMonHelper;
	Ptr<FlowMonitor> monitor = flowMonHelper.Install(remoteHost);
	for(int i = 0; i<number_enbs;i++)
	{
		monitor = flowMonHelper.Install(UEnodes[i]);
	}
	monitor->SetAttribute("DelayBinWidth", DoubleValue(0.0001));
	monitor->SetAttribute("JitterBinWidth", DoubleValue(0.0001));


	/*****************************************
	 *
	 * Run Simulation
	 *
	 *
	 ****************************************/
	Simulator::Stop (Seconds (simTime));

	AnimationInterface anim ("topology.xml");							//Graphical representation of the scenario
	std::string dlOutFname = "DlRlcStats";
	dlOutFname.append (tag.str ());
	std::string ulOutFname = "UlRlcStats";
	ulOutFname.append (tag.str ());

	int startTime = getMilliCount();
	DisplayCurrentTime();
	Simulator::Run();


	int duration = getMilliSpan(startTime);
	cout << endl << "Simulation finished in " << duration/1000.0 << " secs" << endl;



	Simulator::Destroy();

	/*****************************************
	 *
	 * Simulation Finished
	 *
	 ******************************************/

	monitor->SerializeToXmlFile("results_icic.xml", true, true);

return 0;
}

string
NumberToString (int num)
{
	ostringstream ss;
	ss << num;
	return ss.str();
}


// Print the simulation time with a progress status
void
DisplayCurrentTime(void) {
    int nsec = Simulator::Now().GetMilliSeconds();
    printf("%c[2K\r", 27); //clear line
    printf("t = %.002f s (%g %%)", nsec/1000.0, floor((double)nsec/(10*(double)simTime)));
    cout.flush();
    Simulator::Schedule(MilliSeconds(2.5*simTime), &DisplayCurrentTime);
}

// Return system time in milliseconds
int
getMilliCount(){
    timeb tb;
    ftime(&tb);
    int nCount = tb.millitm + (tb.time & 0xfffff) * 1000;
    return nCount;
}

// Compute difference between current system time and the input value
int
getMilliSpan(int nTimeStart){
    int nSpan = getMilliCount() - nTimeStart;
    if(nSpan < 0)
        nSpan += 0x100000 * 1000;
    return nSpan;
}

