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

int main (int argc, char *argv[])
{
	//Helper Modules
	
	Ptr <LteHelper> lteHelper;
	lteHelper = CreateObject<LteHelper> ();								//LTE Helper module
	
	MobilityHelper mobility;											//Mobility Helper
	Ptr<ListPositionAllocator> remoteHostPosition;
	Ptr<ListPositionAllocator> pgwPosition;								//pgw=packet data network gateway
	
	
	Ptr <EpcHelper> epcHelper;
	epcHelper = CreateObject<EpcHelper> ();								//EPC Network rprovides a means for simulation of end-to-end IP connectivity over the lte model
	lteHelper->SetEpcHelper(epcHelper);									//This is used to setup the EPC network in conjunction with the LTE radio network
	
	//EPC
	Ptr<Node> remoteHost; 												//Creates a single remote host
	NodeContainer remoteHostContainer;
	remoteHostContainer.Create(1);
	remoteHost = remoteHostContainer.Get(0);							//Gets the pointer to the first node
	//set remotehost mobility
	remoteHostPosition = CreateObject<ListPositionAllocator>();
	remoteHostPosition->Add(Vector(160,320,0));
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.SetPositionAllocator(remoteHostPosition);
	mobility.Install(remoteHost);
	
	//Install the internet stack onto the remotehost
	InternetStackHelper internet;
	internet.Install(remoteHostContainer);
	
	//Create PGW
	Ptr<Node> pgw;
	pgw=epcHelper->GetPgwNode();
	//set mobility
	pgwPosition = CreateObject<ListPositionAllocator> ();
	pgwPosition->Add(Vector(160,240,0));
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.SetPositionAllocator(pgwPosition);
	mobility.Install(pgw);
	
	//Create Internet
	PointToPointHelper p2pHelper;
	p2pHelper.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
	p2pHelper.SetDeviceAttribute("Mtu", UintegerValue(1500));
	p2pHelper.SetChannelAttribute("Delay", TimeValue(Seconds(0)));
	//Install InternetDeviced into the pgw and remotehost
	NetDeviceContainer internetDevices;
	internetDevices = p2pHelper.Install(pgw, remoteHost);
	//Set IP address
	Ipv4AddressHelper ipv4Helper;
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
		
	// Create a set of Pico Base Stations
	//uint32_t no_PeNB = 12;
	
	//NodeContainer PeNBs;
	//PeNBs.Create(no_PeNB);
	
	// Create UEs
	//uint32_t no_UEs = no_PeNB*30;
	//NodeContainer UE;
	//UE.Create(no_UEs);
	
	
	// Create PeNBs
	uint32_t PeNB_radius = 40;
	uint32_t no_PeNBs = 12;
	NodeContainer PeNBs;
	PeNBs.Create(no_PeNBs);												//Create 12 PeNB objects
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");	//Set the PBSs to remain stationary
	mobility.Install(PeNBs);												//Install the mobility model
	
	
	//Set up the PeNBs in a 3 secctor hexagonal grid
	Ptr<LteHexGridEnbTopologyHelper> lteHexGridEnbTopologyHelper = CreateObject<LteHexGridEnbTopologyHelper> ();
	
	lteHexGridEnbTopologyHelper->SetLteHelper(lteHelper);				//Set to Create EnbNetDevices
	
	//Set Attributes
	lteHexGridEnbTopologyHelper->SetAttribute("InterSiteDistance", DoubleValue(2*PeNB_radius));		//Distance between pico base stations set at 2*radius
	lteHexGridEnbTopologyHelper->SetAttribute("MinX", DoubleValue(2*2*PeNB_radius));				//The X co-ordinate where the HexGrid starts
	lteHexGridEnbTopologyHelper->SetAttribute("MinY", DoubleValue(0));								//The Y co-ordinate where the HexGrid starts
	lteHexGridEnbTopologyHelper->SetAttribute("GridWidth", UintegerValue(1));						//The number of sites in even rows
	lteHexGridEnbTopologyHelper->SetAttribute("SectorOffset", DoubleValue(0.5));
	
	Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue(30));								//Sets transmission power to 30dBm
	
	lteHelper->SetEnbAntennaModelType("ns3::ParabolicAntennaModel");
	//lteHelper->SetEnbAntennaModelAttribute("beamwidth", DoubleValue(70));		//not needed in pico?
	lteHelper->SetEnbAntennaModelAttribute("MaxAttenuation", DoubleValue(20.0));
	
	lteHelper->SetEnbDeviceAttribute("DlEarfcn", UintegerValue(100));
	lteHelper->SetEnbDeviceAttribute("UlEarfcn", UintegerValue(18100));
	lteHelper->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(25));
	lteHelper->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(25));
	
	NetDeviceContainer PeNB_devs;
	PeNB_devs = lteHexGridEnbTopologyHelper->SetPositionAndInstallEnbDevice(PeNBs);
	
	AnimationInterface anim ("topology.xml");
	
	
	
	
/*	
	// SET UP MOBILITY MODELS

	//
	
	//Set mobility of the PeNBs
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

	uint32_t i=0;
	for (uint32_t i=0;i<no_PeNB;i++)
	{
		MobilityHelper PeNBMobility;
		positionAlloc->Add(Vector(20+i,20+i,0));
		PeNBMobility.SetPositionAllocator(positionAlloc);
		PeNBMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
		PeNBMobility.Install(PeNBs.Get(i));			//PeNBs.get returns a node pointer stored in this container at a given index
			
	}
	*/
    
    /* Question: Is there a way of finding out the position of the PeNBs
       as a way of ensuring that the position of the nodes are being set 
       correctly
    */
    
  /*
	for(i=0;i<no_UEs;i++)
	{
		std::cout<<i<<std::endl;
		
		MobilityHelper UEMobility;
		
		// X represents x-coordinate of the centre of random disc
		// Y represents y-coordinate of the centre of random disc    
		// RHO represents a random variable representing the position in a random disc
		UEMobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                         "X", StringValue ("20.0"),
                                         "Y", StringValue ("20.0"),
                                         "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=1]"));

		 UEMobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                                                "Mode", StringValue ("Time"),
                                                "Time", StringValue ("2s"),
                                                "Speed", StringValue ("ns3::UniformRandomVariable[Min=1|Max=1]"),
                                                "Bounds", StringValue ("0|100|0|100"));
        UEMobility.Install(UE.Get(i));
		
	}
	*/
	
}
