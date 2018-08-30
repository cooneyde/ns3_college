/*
 * lte-icic.h
 *
 *  Created on: Feb 24, 2014
 *      Author: dermot
 */

#ifndef LTE_ICIC_H_
#define LTE_ICIC_H_

#include <ns3/nstime.h>
#include <ns3/object.h>
#include <ns3/packet.h>
#include <ns3/lte-enb-cmac-sap.h>
#include <ns3/lte-mac-sap.h>
#include <ns3/lte-pdcp-sap.h>
#include <ns3/epc-x2-sap.h>
#include <ns3/epc-enb-s1-sap.h>
#include <ns3/lte-enb-cphy-sap.h>
#include <ns3/lte-rrc-sap.h>
#include <ns3/traced-callback.h>
#include <ns3/event-id.h>
#include <ns3/lte-enb-rrc.h>
#include <ns3/ff-mac-common.h>
#include <ns3/pf-ff-mac-scheduler.h>
#include <ns3/lte-common.h>
#include <ns3/ff-mac-csched-sap.h>
#include <ns3/ff-mac-sched-sap.h>
#include <ns3/ff-mac-scheduler.h>
#include <ns3/lte-amc.h>
#include <ns3/radio-bearer-stats-calculator.h>
#include <vector>
#include <map>
#include <set>

namespace ns3 {

struct pfsFlowPerf_t;

class LteICIC : public Object
{
public:


	friend class LteEnbRrc;
	friend class PfFfMacScheduler;
	bool isHII;
	std::vector <EpcX2Sap::UlInterferenceOverloadIndicationItem>  m_currentUlInterferenceOverloadIndicationList;
	std::vector <EpcX2Sap::UlHighInterferenceInformationItem>  m_currentUlHighInterferenceInformationList;
	EpcX2Sap::RelativeNarrowbandTxBand m_currentRelativeNarrowbandTxBand;
	/*brief Constructor
	 *
	 * Constructor
	*/
	LteICIC ();

	/**
	* Destructor
	*/
	virtual ~LteICIC ();


	/*
	 * Reads the RBGmap for any resource blocks that have a CQI below a certain threshold and
	 * sets that location in m_currentHighInterferenceInformationItem to true
	 */
	void
	MenbHii(std::map <uint16_t, pfsFlowPerf_t> &m_flowStatsDl,
			std::map <uint16_t, SbMeasResult_s> &m_a30CqiRxed,
			 std::vector<bool> &rbgMap, int &rbgSize, int &rbgNum);

	/*
	 * Reads the RBGmap for any resource blocks that have a CQI below a certain threshold and
	 * sets that location in m_currentHighInterferenceInformationItem to true
	 */
	void PenbHii(std::vector<bool> rbgmap, int rbgSize, int rbgNum);

	std::vector<bool> getHii(void) const;
	void setHii(std::vector<bool> HighInterferenceInformationList);
};



}
#endif /* LTE_ICIC_H_ */
