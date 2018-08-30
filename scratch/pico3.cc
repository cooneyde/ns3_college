/*
 * pico3.cc
 *
 *  Created on: 14 Jan 2014
 *      Author: dermot
 */

//currently works with original ltehelper class....adaptations made will cause sigsegv error

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

using namespace ns3;
using namespace std;

int main(int argc, char *argv[])
{
	//PeNB denotes the pico base station
	Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue(46));								//Sets transmission power to 30dBm
	Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue(10));
	Config::SetDefault ("ns3::LteUePhy::NoiseFigure", DoubleValue(9));
	Config::SetDefault("ns3::LteEnbPhy::NoiseFigure", DoubleValue(5));

	int PeNB_radius = 40;
	int number_PeNBs = 27;
	int number_UEs = number_PeNBs*2;

	//Helper Modules
	Ptr <LteHelper> lteHelper = CreateObject<LteHelper> ();	//LTE Helper module
	Ptr<EpcHelper> epcHelper = CreateObject<EpcHelper> ();	//EPC Helper Module - EPC Network provides a means for simulation of end-to-end IP connectivity over the lte model
	Ptr<LteHexGridEnbTopologyHelper> lteHexGridEnbTopologyHelper = CreateObject<LteHexGridEnbTopologyHelper> ();
	InternetStackHelper internet;							//Aggregates IP/TCP/UDP functionality to existing nodes
	//PointToPointHelper p2pHelper;							//Used in the creation of a point to point network
	//Ipv4AddressHelper ipv4Helper;							//Helper for IPV4 address assignments
	MobilityHelper mobility;
	//______________________________________________________________________________

	//DataRate dlDataRate = DataRate("1Mbps");

	lteHelper->SetEpcHelper (epcHelper);
	lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");
    lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::FriisSpectrumPropagationLossModel"));
    lteHelper->SetSpectrumChannelType ("ns3::MultiModelSpectrumChannel");


	//NODE CREATION
	NodeContainer UEnodes;				//Keeps track of UE node pointers
	NodeContainer PeNBnodes;			//Keeps track of PeNB node pointers
	//Default Scheduler is Proportional Fair scheduling

	PeNBnodes.Create(number_PeNBs);		//Creates N Pico nodes with pointers appended to each
	UEnodes.Create(number_UEs);			//Creates M UE nodes with pointers appended to them
	//________________________________________________________________________________________________________

	//SET THE MOBILITY OF THE PeNBS AND THEIR TOPOLOGY
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");		//Set the PBSs to remain stationary
	mobility.Install(PeNBnodes);											//Install the mobility model
	lteHexGridEnbTopologyHelper->SetLteHelper(lteHelper);				//Set to Create EnbNetDevices

	//SET ATTRIBUTES OF THE TOPOLOGY
	lteHexGridEnbTopologyHelper->SetAttribute("InterSiteDistance", DoubleValue(2*PeNB_radius));		//Distance between pico base stations set at 2*radius
	lteHexGridEnbTopologyHelper->SetAttribute("MinX", DoubleValue(2*2*PeNB_radius));				//The X co-ordinate where the HexGrid starts
	lteHexGridEnbTopologyHelper->SetAttribute("MinY", DoubleValue(0));								//The Y co-ordinate where the HexGrid starts
	lteHexGridEnbTopologyHelper->SetAttribute("GridWidth", UintegerValue(2));						//The number of sites in even rows
	lteHexGridEnbTopologyHelper->SetAttribute("SectorOffset", DoubleValue(5));
	lteHelper->SetEnbAntennaModelType("ns3::ParabolicAntennaModel");
	//lteHelper->SetEnbAntennaModelAttribute("beamwidth", DoubleValue(60));		//not needed in pico?
	lteHelper->SetEnbAntennaModelAttribute("MaxAttenuation", DoubleValue(20.0));
	lteHelper->SetEnbDeviceAttribute("DlEarfcn", UintegerValue(100));								//DLEarfcn - Downlink Evolved Absolute Radio Frequency Channel Number
	lteHelper->SetEnbDeviceAttribute("UlEarfcn", UintegerValue(18100));
	lteHelper->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(25));
	lteHelper->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(25));

	NetDeviceContainer PeNBltedevs;
	PeNBltedevs = lteHexGridEnbTopologyHelper->SetPositionAndInstallEnbDevice(PeNBnodes);		//Position the nodes on a hexagonal grid and install the corresponding ENB devices
	cout << "PeNB Devices installed" << endl;
	//_________________________________________________________________________________________________________


	//SET THE MOBILITY OF THE UEs
	Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Mode", StringValue ("Time"));
	Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Time", StringValue ("2s"));
	Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
	Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Bounds", StringValue ("0|200|0|200"));

	mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
	                                "X", StringValue ("200.0"),
	                                "Y", StringValue ("100.0"),
	                                "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=100]"));
	                                //RandomDiscPositionAllocator randomly sets a position for each UE within a disc centred at (160,160) with radius 160
	mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
	                            "Mode", StringValue ("Time"),				//The condition to change speed and direction (either distance or time)
	                            "Time", StringValue ("2s"),					//Delay defines how long until the UE changes direction and speed
	                            "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),	//speed in m/s
	                            "Bounds", StringValue ("0|320|0|200"));		//Sets the region in which the UEs may move


	mobility.Install (UEnodes);
	//_________________________________________________________________________________________________________

	Ptr<Node> pgw = epcHelper->GetPgwNode();

	//Create RemoteHosts
	NodeContainer remoteHostsContainer;
	remoteHostsContainer.Create(number_UEs);	//Each UE is given a different RemoteHost
	internet.Install(remoteHostsContainer);		//Install the internet stack onto the remotehost
	Ptr<ListPositionAllocator> remoteHostPosition;
	remoteHostPosition = CreateObject<ListPositionAllocator>();
	remoteHostPosition->Add(Vector(160,320,0));
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.SetPositionAllocator(remoteHostPosition);
	mobility.Install(remoteHostsContainer);

	//Link the remoteHosts to the pgw (S1 I think)
	for (int i=0;i<number_UEs;i++)
	{
		PointToPointHelper p2pHelper;
		p2pHelper.SetDeviceAttribute("DataRate", DataRateValue (DataRate("100Gb/s")));
		p2pHelper.SetDeviceAttribute("Mtu", UintegerValue(1500));
		p2pHelper.SetChannelAttribute("Delay", TimeValue(MilliSeconds(10)));

		NetDeviceContainer internetDevices = p2pHelper.Install(remoteHostsContainer.Get(i), pgw);

		Ipv4AddressHelper ipv4Helper;
		char buf[512];
		sprintf(buf, "50.50.%d.0",i+1);
		ipv4Helper.SetBase(buf, "255.255.255.0");
		ipv4Helper.Assign(internetDevices);
	}

	//Add link from remotes to the UEs
	Ipv4StaticRoutingHelper ipv4RoutingHelper;
	for(uint32_t i=0; i<remoteHostsContainer.GetN();i++)
	{
		Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting(remoteHostsContainer.Get(i)->GetObject<Ipv4> ());
		remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.1"),1);
	}

	//Install LTE devices to the UEs
	lteHelper->SetUeAntennaModelType("ns3::IsotropicAntennaModel");
	NetDeviceContainer UEltedevs = lteHelper->InstallUeDevice(UEnodes);
	cout << "LTE devices installed on UEs" << endl;

	//Install X2 Interface
	lteHelper->AddX2Interface(PeNBnodes);	//Creates an X2 interface between all PeNB nodes

	//Install the internet stack on the UEs
	internet.Install(UEnodes);
	Ipv4InterfaceContainer ueIPInterface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(UEltedevs));
	for(uint32_t i=0;i<UEnodes.GetN();i++)
	{
		Ptr<Node> ueNode = UEnodes.Get(i);
		//Set the default gateway for the ue
		Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
		ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(),1);

		lteHelper->ActivateDedicatedEpsBearer(UEltedevs.Get(i), EpsBearer(EpsBearer::GBR_CONV_VOICE), EpcTft::Default());
		//EpcTft implements EPS Bearer traffic flow template (TFT), the set of all packet filters associated with the eps bearer
		//EpcTft::Default() creates a TFT to match any traffic
	}
	cout << "Attaching to ENB" << endl;
	//Attach UEs to the closest ENB
	lteHelper->AttachToClosestEnb(UEltedevs, PeNBltedevs);
	cout << "Devices attached to Enbs" << endl;
    lteHelper->EnableTraces ();		//Enable traces just before a simulation starts
	AnimationInterface anim ("topology.xml");							//Graphical representation of the scenario
    Simulator::Stop (Seconds (1.05));

     Simulator::Run ();

     // GtkConfigStore config;
     // config.ConfigureAttributes ();

     Simulator::Destroy ();




return 0;
}


