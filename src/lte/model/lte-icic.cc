/*
 * lte-icic.cc
 *
 *  Created on: Feb 24, 2014
 *      Author: dermot
 */



#include <ns3/log.h>
#include <ns3/pointer.h>
#include <ns3/math.h>

#include <ns3/lte-icic.h>

#include <ns3/simulator.h>
#include <ns3/lte-amc.h>
#include <ns3/pf-ff-mac-scheduler.h>
#include <ns3/ff-mac-common.h>
#include <ns3/lte-vendor-specific-parameters.h>
#include <ns3/boolean.h>
#include <cfloat>
#include <set>
#include <ns3/lte-enb-rrc.h>

namespace ns3 {

LteICIC::LteICIC():
		m_currentUlHighInterferenceInformationList(1)
{
	isHII = false;

}

LteICIC::~LteICIC()
{

}
void
LteICIC::MenbHii(std::map <uint16_t, pfsFlowPerf_t> &m_flowStatsDl,
				  std::map<uint16_t, SbMeasResult_s> &m_a30CqiRxed,
				  int &rbgSize, int &rbgNum)
{

	m_currentUlHighInterferenceInformationList[0].ulHighInterferenceIndicationList.resize (rbgNum, false);
	//uint16_t cellId = 2; //MeNB cell ID

	uint8_t minThresh= 6;//Need to set a minimum threshold
	 isHII = false;
	for(int i = 0;i < rbgNum; ++i)
	{

		std::map<uint16_t, pfsFlowPerf_t>::iterator it;
		for(it= m_flowStatsDl.begin(); it != m_flowStatsDl.end();++it)
		{
			std::map<uint16_t, SbMeasResult_s>::iterator itCqi;
			itCqi = m_a30CqiRxed.find((*it).first);

			uint8_t cqi = 0x0F & (*itCqi).second.m_bwPart.m_cqi;

			if( cqi < minThresh)
			{
				//std::cout << (int)cqi << std::endl;
				//std::cout << "HII" << std::endl;
				m_currentUlHighInterferenceInformationList[0].ulHighInterferenceIndicationList[i] = true;
				isHII = true;
			}
			else
			{
				m_currentUlHighInterferenceInformationList[0].ulHighInterferenceIndicationList[i] = false;

			}

		}
	}
}


void
LteICIC::PenbHii(std::vector<bool> rbgmap, int rbgSize, int rbgNum)
{
	isHII = false;
	for(int i = 0; i < rbgNum; ++i)
	{
		if(m_currentUlHighInterferenceInformationList[0].ulHighInterferenceIndicationList[i]==true)
		{
			isHII = true;
			//std::cout << "HII " << isHII << std::endl;
			rbgmap[i] = true;
		}
	}

}

std::vector<bool>
LteICIC::getHii(void) const
{
	return m_currentUlHighInterferenceInformationList.data()->ulHighInterferenceIndicationList;
}

void
LteICIC::setHii(std::vector<bool> HighInterferenceInformationList)
{
	m_currentUlHighInterferenceInformationList[0].ulHighInterferenceIndicationList = HighInterferenceInformationList;
}

}

