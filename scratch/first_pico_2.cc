#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/config-store-module.h"
#include "ns3/buildings-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"
#include <iomanip>
#include <ios>
#include <string>
#include <vector>
#include "ns3/netanim-module.h"

#include "ns3/constant-position-mobility-model.h"
#include "ns3/constant-velocity-mobility-model.h"


using namespace ns3;


//CODE UPDATED AS AT 8-01-2014
int main (int argc, char *argv[])
{
	
	int PeNB_radius = 40;
	int no_PeNBs = 18;
	int no_UEs = no_PeNBs*2;

	//HELPER MODULES
	Ptr <LteHelper> lteHelper = CreateObject<LteHelper> ();								//LTE Helper module
	Ptr <EpcHelper> epcHelper = CreateObject<EpcHelper> ();								//EPC Network rprovides a means for simulation of end-to-end IP connectivity over the lte model
	Ptr<LteHexGridEnbTopologyHelper> lteHexGridEnbTopologyHelper = CreateObject<LteHexGridEnbTopologyHelper> ();
	InternetStackHelper internet;										//Aggregates IP/TCP/UDP functionality to existing nodes
	PointToPointHelper p2pHelper;										//Used in the creation of a point to point network
	Ipv4AddressHelper ipv4Helper;										//Helper for IPV4 address assignments
	MobilityHelper mobility;
	//______________________________________________________________________________________________________________
	
	
	Ptr<ListPositionAllocator> remoteHostPosition;
	Ptr<ListPositionAllocator> pgwPosition;								//pgw=packet data network gateway
	
	
	
	lteHelper->SetEpcHelper(epcHelper);									//This is used to setup the EPC network in conjunction with the LTE radio network
	//Set a path loss model for an urban environment?
	NodeContainer PeNBs;
	NodeContainer UE;
	
	PeNBs.Create(no_PeNBs);												//Create PeNBs Nodes
	UE.Create(no_UEs);													//Create UE Nodes

	//SET MOBILITY MODEL OF PeNBs
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");	//Set the PBSs to remain stationary
	mobility.Install(PeNBs);											//Install the mobility model
	//__________________________________________________________________________________________________________________________________________________
	
	//SET MOBILITY MODEL OF UEs
	//Use the Random Walk Mobility Model Here 
	
	Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Mode", StringValue ("Time"));
	Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Time", StringValue ("2s"));
	Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
	Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Bounds", StringValue ("0|200|0|200"));
	
	mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue ("160.0"),
                                 "Y", StringValue ("160.0"),
                                 "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=160]"));
                                 //RandomDiscPositionAllocator randomly sets a position for each UE within a disc centred at (160,160) with radius 160
	mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Time"),				//The condition to change speed and direction (either distance or time)
                             "Time", StringValue ("2s"),				//Delay defines how long until the UE changes direction and speed
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),	//speed in m/s
                             "Bounds", StringValue ("0|320|0|320"));	//Sets the region in which the UEs may move
                             				
	
    mobility.Install (UE);					
    //____________________________________________________________________________________________________________________________
	
	//INSTALL LTE DEVICES TO THE UEs
	NetDeviceContainer UE_devs;											//Instantiates a net device for each UE
	lteHelper->SetUeAntennaModelType("ns3::IsotropicAntennaModel");
	UE_devs = lteHelper->InstallUeDevice(UE);							//Installs the LTE Protocol Stack on the UEs
	
		
	//SETS UP THE PeNBS IN A 3 SECTOR HEXAGONAL GRID
	//Hexagonal site contains 3 sectors, each with a PeNB
	lteHexGridEnbTopologyHelper->SetLteHelper(lteHelper);				//Set to Create EnbNetDevices
	
	//SET ATTRIBUTES OF THE TOPOLOGY
	lteHexGridEnbTopologyHelper->SetAttribute("InterSiteDistance", DoubleValue(2*PeNB_radius));		//Distance between pico base stations set at 2*radius
	lteHexGridEnbTopologyHelper->SetAttribute("MinX", DoubleValue(2*2*PeNB_radius));				//The X co-ordinate where the HexGrid starts
	lteHexGridEnbTopologyHelper->SetAttribute("MinY", DoubleValue(0));								//The Y co-ordinate where the HexGrid starts
	lteHexGridEnbTopologyHelper->SetAttribute("GridWidth", UintegerValue(2));						//The number of sites in even rows
	lteHexGridEnbTopologyHelper->SetAttribute("SectorOffset", DoubleValue(5));
	
	Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue(30));								//Sets transmission power to 30dBm
	
	
	lteHelper->SetEnbAntennaModelType("ns3::ParabolicAntennaModel");
	//lteHelper->SetEnbAntennaModelAttribute("beamwidth", DoubleValue(60));		//not needed in pico?
	lteHelper->SetEnbAntennaModelAttribute("MaxAttenuation", DoubleValue(20.0));
	lteHelper->SetEnbDeviceAttribute("DlEarfcn", UintegerValue(100));								//DLEarfcn - Downlink Evolved Absolute Radio Frequency Channel Number
	lteHelper->SetEnbDeviceAttribute("UlEarfcn", UintegerValue(18100));
	lteHelper->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(25));
	lteHelper->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(25));
	
	
	NetDeviceContainer PeNB_devs;
/*
	std::cout<< "There are " << PeNBs.GetN() << "pico cells" << std::endl;
	for(uint32_t u=0; u<PeNBs.GetN();u++)
	{
		lteHelper->SetEnbAntennaModelType("ns3::ParabolicAntennaModel");
		lteHelper->SetEnbDeviceAttribute("DlEarfcn", UintegerValue((u%2)*50));								//DLEarfcn - Downlink Evolved Absolute Radio Frequency Channel Number
		lteHelper->SetEnbDeviceAttribute("UlEarfcn", UintegerValue((u%2)*50 + 18000));
		lteHelper->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(25));
		lteHelper->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(25));
		std::cout<< u << std::endl;
	}
*/


	std::cout<< "Installing PeNB Devices" << std::endl;
	PeNB_devs = lteHexGridEnbTopologyHelper->SetPositionAndInstallEnbDevice(PeNBs);		//Position the nodes on a hexagonal grid and install the corresponding ENB devices
	//lteHelper->AddX2Interface(PeNBs);
	//____________________________________________________________________________________________________________________________________________________________________
	
	std::cout<<"Attaching UEs to Enbs" << std::endl;
	lteHelper->AttachToClosestEnb(UE_devs, PeNB_devs);					//NB: not getting past this line of code
	
	
	std::cout<< "Setting up EPC"<<std::endl;
	//EPC
	Ptr<Node> remoteHost; 												//Creates a single remote host
	NodeContainer remoteHostContainer;



	remoteHostContainer.Create(1);
	internet.Install(remoteHostContainer);								//Install the internet stack onto the remotehost
	remoteHost = remoteHostContainer.Get(0);							//Gets the pointer to the first node
	//set remotehost mobility
	remoteHostPosition = CreateObject<ListPositionAllocator>();
	remoteHostPosition->Add(Vector(160,320,0));
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.SetPositionAllocator(remoteHostPosition);
	mobility.Install(remoteHost);
	
	
	



	//Create PGW
	
	Ptr<Node> pgw=epcHelper->GetPgwNode();
	//SET MOBILITY FOR THE PGW
	pgwPosition = CreateObject<ListPositionAllocator> ();
	pgwPosition->Add(Vector(160,240,0));
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.SetPositionAllocator(pgwPosition);
	mobility.Install(pgw);
	
	//LINK REMOTEHOSTS TO THE PGW	
	p2pHelper.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
	p2pHelper.SetDeviceAttribute("Mtu", UintegerValue(1500));
	p2pHelper.SetChannelAttribute("Delay", TimeValue(Seconds(0)));
	
	NetDeviceContainer internetDevices = p2pHelper.Install(remoteHost, pgw);
	
	//SET IP ADDRESSES

	ipv4Helper.SetBase("1.0.0.0","255.0.0.0");
	//Assign ip for remotehost and pgw
	Ipv4InterfaceContainer internetIPInterfaces;
	internetIPInterfaces = ipv4Helper.Assign(internetDevices);
	Ipv4Address remoteHostAddr;
	remoteHostAddr = internetIPInterfaces.GetAddress(1);
	Ipv4StaticRoutingHelper ipv4RoutingHelper;
	Ptr<Ipv4StaticRouting> remoteHostStaticRouting;
	remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4> ());
	remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"),1);
		
	
	
	
	
	AnimationInterface anim ("topology.xml");							//Graphical representation of the scenario
	std::cout<<"Program terminated" << std::endl;
	return 0;
	
}
