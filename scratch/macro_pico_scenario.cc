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

using namespace ns3;
using namespace std;

string NumberToString(int num);


//Wireless side only
int main(int argc, char *argv[])
{
	int PeNB_radius = 40;
	int number_PeNB = 2;
	int number_MeNB = 1;
	int number_PUEs = 4;										//number of UEs per PeNB
	uint32_t UEmobility_size = 2;								//used for the maximum size of for loops dealing with the UE node groups


	//Helper Modules
	Ptr <LteHelper> lteHelper = CreateObject<LteHelper> ();		//LTE Helper module
	//Ptr<EpcHelper> epcHelper = CreateObject<EpcHelper> ();	//EPC Helper Module - EPC Network provides a means for simulation of end-to-end IP connectivity over the lte model
	//lteHelper->SetEpcHelper(epcHelper);
	lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");


	MobilityHelper ENBmobility[2];							//Array slot 0 for MeNB mobility, Array slot 1 for PeNB mobilit



	NodeContainer MeNBnodes, PeNBnodes; //Create MeNB and PeNB node containers
	NodeContainer UEnodes[2];

	MeNBnodes.Create(number_MeNB);
	PeNBnodes.Create(number_PeNB);

	UEnodes[0].Create(number_PUEs);
	UEnodes[1].Create(number_PUEs);

	MobilityHelper UEmobility[UEmobility_size];	//Sets up 2 mobility models for each set of PBS UEs

	//Set mobility model of the PeNB and MeNB nodes
	Ptr<ListPositionAllocator> MacropositionAlloc = CreateObject<ListPositionAllocator> ();
	MacropositionAlloc->Add(Vector(150.0, 150.0, 10.0));

	ENBmobility[0].SetPositionAllocator(MacropositionAlloc);
	ENBmobility[0].SetMobilityModel("ns3::ConstantPositionMobilityModel");
	ENBmobility[0].Install (MeNBnodes);

	double pico_x[2], pico_y[2];	//Pico coordinate arrays
	// iterate through the number of pico nodes to set their mobility and positions

	//Set up the x,y coordinates for the pico base stations
	for(int i=0;i<2;i++){
		if (i-1>-1){
			pico_x[i] = (150+ pico_y[i-1])*0.5+PeNB_radius;
			pico_y[i] = (150+pico_y[i-1])*0.5 + PeNB_radius;
		}
		else{
			pico_x[i] = 0.5*150+PeNB_radius;
			pico_y[i] = 150*0.5 + PeNB_radius;
		}
	}
	//_______________________________________________________________

	// Set up PeNB mobility models and install the model onto the nodes
	for(uint32_t i=0; i<PeNBnodes.GetN();i++){
		Ptr<ListPositionAllocator> PicopositionAlloc = CreateObject<ListPositionAllocator> ();
		//int range = 250 - (-250)+1;	//250 denotes the radius of the macro cell
		//pico_x[i] = 150 + (rand() %range + (-250));	//Random positioning of the pico cell
		//pico_y[i] = 150 + (rand() %range + (-250));	//Random positioning of the pico cell
		//cout << "x:" << pico_x[i] << " y: " << pico_y[i] << endl;
		PicopositionAlloc->Add(Vector(pico_x[i], pico_y[i], 10.0));
		ENBmobility[1].SetMobilityModel("ns3::ConstantPositionMobilityModel");
		ENBmobility[1].SetPositionAllocator(PicopositionAlloc);
		ENBmobility[1].Install(PeNBnodes.Get(i));
	}

	//Set up the mobility model for the UEs
	for(uint32_t i=0; i<UEmobility_size;i++)
	{
		UEmobility[i].SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
			                                "X", StringValue(NumberToString(pico_x[i])),
			                                "Y", StringValue(NumberToString(pico_y[i])),
			                                "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=40]"));
			                                //RandomDiscPositionAllocator randomly sets a position for each UE within a disc centred at (160,160) with radius 160
		UEmobility[i].SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
			                            "Mode", StringValue ("Time"),				//The condition to change speed and direction (either distance or time)
			                            "Time", StringValue ("0.2s"),					//Delay defines how long until the UE changes direction and speed
			                            "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=0.833]"),	//speed in m/s
			                            "Bounds", StringValue ("-320|320|-320|320"));		//Sets the region in which the UEs may move
		UEmobility[i].Install (UEnodes[i]);	//Apply the mobility model to the UEs
	}
	//________________________________________________________________________________________________

	//Create NetDeviceContainers
	NetDeviceContainer PeNBDevs = lteHelper->InstallEnbDevice (PeNBnodes);
	NetDeviceContainer MeNBDevs = lteHelper->InstallEnbDevice(MeNBnodes);
	NetDeviceContainer UEDevs[UEmobility_size];


	//Set up DataRadioBearer
	enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
	EpsBearer bearer (q);
	//Install UE Devices onto ue nodes, and attach to PeNB
	for(uint32_t i=0; i<UEmobility_size;i++){
		UEDevs[i] = lteHelper->InstallUeDevice(UEnodes[i]);
		lteHelper->AttachToClosestEnb(UEDevs[i], PeNBDevs);
	    lteHelper->ActivateDataRadioBearer (UEDevs[i], bearer);
	}

	//MeNB Physical Layer Control
	Ptr<LteEnbNetDevice> LteMeNBDev = MeNBDevs.Get(0)->GetObject<LteEnbNetDevice>();
	Ptr<LteEnbPhy> MeNBPhy = LteMeNBDev->GetPhy();
	MeNBPhy->SetAttribute("TxPower", DoubleValue(46.0));

	//PeNB Physical Layer Control
	for(uint32_t i=0; i<PeNBnodes.GetN();i++){
		Ptr <LteEnbNetDevice> LtePeNBDev = PeNBDevs.Get(i)->GetObject<LteEnbNetDevice>();
		Ptr <LteEnbPhy> PeNBPhy = LtePeNBDev->GetPhy();
		PeNBPhy->SetAttribute("TxPower", DoubleValue(30.0));
		//cout << "PeNB Transmit power: "<< PeNBPhy->GetTxPower() << endl;
	}
	//cout << "MeNB Transmit power: "<<MeNBPhy->GetTxPower() << endl;



	Simulator::Stop (Seconds (5));
	AnimationInterface anim ("topology.xml");							//Graphical representation of the scenario

	Simulator::Run();

	//write your simulation here?

	Simulator::Destroy();







return 0;
}

string NumberToString (int num)
{
	ostringstream ss;
	ss << num;
	return ss.str();
}

