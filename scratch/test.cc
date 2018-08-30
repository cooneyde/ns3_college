/*
 * test.cc
 *
 *  Created on: Feb 20, 2014
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

using namespace ns3;
using namespace std;

string NumberToString(int num);

int main(int argc, char *argv[])
{
	int PeNB_radius = 40;
	int number_PeNB = 2;
	int number_MeNB = 1;
	int number_PUEs = 1;		//number of UEs per PeNB
	int number_MUEs = 1;
	int number_UEs_Per_eNB = 1;
	int number_UEs = (number_PeNB + number_MeNB) * number_UEs_Per_eNB;
	//uint32_t UEmobility_size = 2;	//used for the maximum size of for loops dealing with the UE node groups

	//Helper Modules
	Ptr <LteHelper> lteHelper = CreateObject<LteHelper> ();	//LTE Helper module
	Ptr<EpcHelper> epcHelper = CreateObject<EpcHelper> ();	//EPC Helper Module - EPC Network provides a means for simulation of end-to-end IP connectivity over the lte model
	lteHelper->SetEpcHelper(epcHelper);
	lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");


	MobilityHelper ENBmobility[2], mobility;							//Array slot 0 for MeNB mobility, Array slot 1 for PeNB mobilit

	// Create Protocol Stacks
	Ipv4StaticRoutingHelper ipv4RoutingHelper;

	//Define the pgw/RemoteHost nodes
	Ptr<Node> pgw = epcHelper->GetPgwNode();
	Ptr<ListPositionAllocator> pgwPosition = CreateObject<ListPositionAllocator>();
	pgwPosition->Add(Vector(75,75,0));
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.SetPositionAllocator(pgwPosition);
	mobility.Install(pgw);

	//Create a single remoteHost
	NodeContainer remoteHostContainer;
	remoteHostContainer.Create(1);
	Ptr<Node> remoteHost = remoteHostContainer.Get(0);

	//Install the internet protocol stack on the remotehost
	InternetStackHelper internet;
	internet.Install(remoteHostContainer);


	Ptr<ListPositionAllocator> remoteHostPosition = CreateObject<ListPositionAllocator>();
	remoteHostPosition->Add(Vector(50,50,0));
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.SetPositionAllocator(remoteHostPosition);
	mobility.Install(remoteHostContainer);

	PointToPointHelper p2p;
	p2p.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1000Gb/s")));
	p2p.SetDeviceAttribute("Mtu", UintegerValue(1500));
	p2p.SetChannelAttribute("Delay", TimeValue(MilliSeconds(10)));

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
	NodeContainer UEnodes[number_UEs];
	cout << "Ip Address of remoteHost" << remoteHostAddr.Get()  << endl;
	MeNBnodes.Create(number_MeNB);
	PeNBnodes.Create(number_PeNB);


	UEnodes[0].Create(number_PUEs);
	UEnodes[1].Create(number_PUEs);
	UEnodes[2].Create(number_PUEs);
	UEnodes[3].Create(number_MUEs);				//UE to be associated with the MeNB

	MobilityHelper UEmobility[number_UEs];	//Sets up 2 mobility models for each set of PBS UEs

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
			PeNBposition[i].x = (MeNBposition.x + PeNBposition[i-1].x) * 0.75 + PeNB_radius;
			PeNBposition[i].y= (MeNBposition.y + PeNBposition[i-1].y) * 0.5 + PeNB_radius;
		}
		else
		{
			PeNBposition[i].x = (MeNBposition.x * 0.66) + PeNB_radius;
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

	//Set up the mobility model for the UEs
	for(int i=0; i<number_UEs;i++)
	{
		cout << i << endl;
		if (i==3)
		{
			UEmobility[i].SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
						                   "X", StringValue(NumberToString(MeNBposition.x)),
						                   "Y", StringValue(NumberToString(MeNBposition.y)),
						                   "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=250]"));
						                   //RandomDiscPositionAllocator randomly sets a position for each UE within a disc centred at (160,160) with radius 160
			UEmobility[i].SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
						                    "Mode", StringValue ("Time"),				//The condition to change speed and direction (either distance or time)
						                    "Time", StringValue ("0.2s"),					//Delay defines how long until the UE changes direction and speed
						                    "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=0.833]"),	//speed in m/s
						                    "Bounds", StringValue ("-320|320|-320|320"));		//Sets the region in which the UEs may move
			UEmobility[i].Install (UEnodes[i]);	//Apply the mobility model to the UEs
		}
		else
		{
			UEmobility[i].SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
												"X", StringValue(NumberToString(PeNBposition[i].x)),
												"Y", StringValue(NumberToString(PeNBposition[i].y)),
												"Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=40]"));
												//RandomDiscPositionAllocator randomly sets a position for each UE within a disc centred at (160,160) with radius 160
			UEmobility[i].SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
											"Mode", StringValue ("Time"),				//The condition to change speed and direction (either distance or time)
											"Time", StringValue ("0.2s"),					//Delay defines how long until the UE changes direction and speed
											"Speed", StringValue ("ns3::ConstantRandomVariable[Constant=0.833]"),	//speed in m/s
											"Bounds", StringValue ("-320|320|-320|320"));		//Sets the region in which the UEs may move
			UEmobility[i].Install (UEnodes[i]);	//Apply the mobility model to the UEs
		}
	}
	//________________________________________________________________________________________________

	//Create NetDeviceContainers
	NetDeviceContainer MeNBDevs = lteHelper->InstallEnbDevice (MeNBnodes);
	NetDeviceContainer PeNBDevs = lteHelper->InstallPEnbDevice (PeNBnodes, MeNBnodes);

	NetDeviceContainer UEDevs[number_UEs];


	//Set up DataRadioBearer
	enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
	EpsBearer bearer (q);
	Ipv4InterfaceContainer ueIpIface;

	//Install UE Devices onto UE nodes, and attach to PeNB
	for(int i = 0; i < number_UEs; i++)
	{
		UEDevs[i] = lteHelper->InstallUeDevice(UEnodes[i]);
        cout << i << "UE Devs installing" << endl;
		internet.Install(UEnodes[i]);
		ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (UEDevs[i]));
		lteHelper->ActivateDedicatedEpsBearer(UEDevs[i].Get(0), EpsBearer(EpsBearer::GBR_CONV_VOICE), EpcTft::Default());

		for (uint32_t j = 0; j < UEnodes[i].GetN (); j++)
		{

		      Ptr<Node> ueNode = UEnodes[i].Get (j);
		      // Set the default gateway for the UE
		      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
		      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
		      if (i < 3)
		      {

		      	  lteHelper->Attach (UEDevs[i].Get(j), PeNBDevs.Get(i));
		      }
		      else
		      {
		    	  lteHelper->Attach (UEDevs[i].Get(j), MeNBDevs.Get(0));
		      }

		}
	}

	cout << "Finished installing uedevs" << endl;

	for(uint32_t i=0; i <PeNBnodes.GetN();i++)
	{
		//internet.Install(PeNBnodes.Get(i));
		cout << "node: " << i << " and node:" << i+1 << endl;
		Ptr<Node> MeNBnode1 = MeNBnodes.Get(0);
		Ptr<Node> PeNBnode2 = PeNBnodes.Get(i);
		cout << MeNBnode1->GetObject<Ipv4> () << endl;
		cout << PeNBnode2->GetObject<Ipv4> () << endl;
		lteHelper->AddX2Interface(MeNBnode1, PeNBnode2);

	}


	//MeNB Physical Layer Control
	Ptr<LteEnbNetDevice> LteMeNBDev = MeNBDevs.Get(0)->GetObject<LteEnbNetDevice>();
	Ptr<LteEnbPhy> MeNBPhy = LteMeNBDev->GetPhy();
	MeNBPhy->SetAttribute("TxPower", DoubleValue(46.0));

	//PeNB Physical Layer Control
	for(uint32_t i=0; i<PeNBnodes.GetN();i++)
	{
		Ptr <LteEnbNetDevice> LtePeNBDev = PeNBDevs.Get(i)->GetObject<LteEnbNetDevice>();
		Ptr <LteEnbPhy> PeNBPhy = LtePeNBDev->GetPhy();
		PeNBPhy->SetAttribute("TxPower", DoubleValue(30.0));
	}

	cout << "Transmit powers set...." << endl;

	Simulator::Stop (Seconds (15));
	AnimationInterface anim ("topology.xml");							//Graphical representation of the scenario

	Simulator::Run();

	//write your simulation here?

	Simulator::Destroy();



return 0;
}

string
NumberToString (int num)
{
	ostringstream ss;
	ss << num;
	return ss.str();
}



