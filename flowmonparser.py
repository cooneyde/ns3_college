from __future__ import division
import os
from multiprocessing import Pool
from operator import sub
import operator
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib import cm
import time
import functools
import flowmonparser
import sys
import itertools
try:
    from xml.etree import cElementTree as ElementTree
except ImportError:
    from xml.etree import ElementTree

def parse_time_ns(tm):
    if tm.endswith('ns'):
        return float(tm[:-2])
    raise ValueError(tm)



class FiveTuple(object):
    __slots__ = ['sourceAddress', 'destinationAddress', 'protocol', 'sourcePort', 'destinationPort']
    def __init__(self, el):
        self.sourceAddress = el.get('sourceAddress')
        self.destinationAddress = el.get('destinationAddress')
        self.sourcePort = int(el.get('sourcePort'))
        self.destinationPort = int(el.get('destinationPort'))
        self.protocol = int(el.get('protocol'))
        
class Histogram(object):
    __slots__ = 'bins', 'nbins', 'number_of_flows'
    def __init__(self, el=None):
        self.bins = []
        if el is not None:
            #self.nbins = int(el.get('nBins'))
            for bin in el.findall('bin'):
                self.bins.append( (float(bin.get("start")), float(bin.get("width")), int(bin.get("count"))) )

class Flow(object):
    __slots__ = ['flowId', 'delayMean', 'jitterMean', 'packetLossRatio', 'rxBitrate', 'txBitrate',
                 'fiveTuple', 'packetSizeMean', 'probe_stats_unsorted',
                 'hopCount', 'flowInterruptionsHistogram','flowDelayHistogram', 'rx_duration']
    def __init__(self, flow_el):
        self.flowId = int(flow_el.get('flowId'))
        rxPackets = long(flow_el.get('rxPackets'))
        txPackets = long(flow_el.get('txPackets'))
        tx_duration = float(float(flow_el.get('timeLastTxPacket')[:-2]) - float(flow_el.get('timeFirstTxPacket')[:-2]))*1e-9
        rx_duration = float(float(flow_el.get('timeLastRxPacket')[:-2]) - float(flow_el.get('timeFirstRxPacket')[:-2]))*1e-9
        self.rx_duration = rx_duration
        self.probe_stats_unsorted = []
        if rxPackets:
            self.hopCount = float(flow_el.get('timesForwarded')) / rxPackets + 1
        else:
            self.hopCount = -1000
        if rxPackets:
            self.delayMean = float(flow_el.get('delaySum')[:-2]) / rxPackets * 1e-9
            self.packetSizeMean = float(flow_el.get('rxBytes')) / rxPackets
	    self.jitterMean = float(flow_el.get('jitterSum')[:-2]) /rxPackets * 1e-9
        else:
            self.delayMean = 0
            self.packetSizeMean = 0
            self.jitterMean = 0
        if rx_duration > 0:
            self.rxBitrate = long(flow_el.get('rxBytes'))*8 / rx_duration
        else:
            self.rxBitrate = 0
        if tx_duration > 0:
            self.txBitrate = long(flow_el.get('txBytes'))*8 / tx_duration
	    
        else:
            self.txBitrate = 0
	
        lost = float(flow_el.get('lostPackets'))
        #print "rxBytes: %s; txPackets: %s; rxPackets: %s; lostPackets: %s" % (flow_el.get('rxBytes'), txPackets, rxPackets, lost)
        if rxPackets == 0:
            self.packetLossRatio = 0
        else:
            self.packetLossRatio = (lost / (rxPackets + lost))

        interrupt_hist_elem = flow_el.find("flowInterruptionsHistogram")
        if interrupt_hist_elem is None:
            self.flowInterruptionsHistogram = None
        else:
            self.flowInterruptionsHistogram = Histogram(interrupt_hist_elem)
            
        delay_hist_elem = flow_el.find("delayHistogram")
        if delay_hist_elem is None:
            self.flowDelayHistogram = None
        else:
            self.flowDelayHistogram = Histogram(delay_hist_elem)


class ProbeFlowStats(object):
    __slots__ = ['probeId', 'packets', 'bytes', 'delayFromFirstProbe']

class Simulation(object):
    def __init__(self, simulation_el):
        self.flows = []
        FlowClassifier_el, = simulation_el.findall("Ipv4FlowClassifier")
        flow_map = {}
        for flow_el in simulation_el.findall("FlowStats/Flow"):
            flow = Flow(flow_el)
            flow_map[flow.flowId] = flow
            self.flows.append(flow)
        for flow_cls in FlowClassifier_el.findall("Flow"):
            flowId = int(flow_cls.get('flowId'))
            flow_map[flowId].fiveTuple = FiveTuple(flow_cls)

        for probe_elem in simulation_el.findall("FlowProbes/FlowProbe"):
            probeId = int(probe_elem.get('index'))
            for stats in probe_elem.findall("FlowStats"):
                flowId = int(stats.get('flowId'))
                s = ProbeFlowStats()
                s.packets = int(stats.get('packets'))
                s.bytes = long(stats.get('bytes'))
                s.probeId = probeId
                if s.packets > 0:
                    s.delayFromFirstProbe =  parse_time_ns(stats.get('delayFromFirstProbeSum')) / float(s.packets)
                else:
                    s.delayFromFirstProbe = 0
                flow_map[flowId].probe_stats_unsorted.append(s)


def main(argv):
   
    
    
    file_obj = open(argv[1])
    print "Reading XML file ",
    logFilename="log+results.txt"
    logFile=openFile(logFilename,"w")
    sys.stdout.flush()        
    level = 0
    sim_list = []
    for event, elem in ElementTree.iterparse(file_obj, events=("start", "end")):
        if event == "start":
            level += 1
        if event == "end":
            level -= 1
            if level == 0 and elem.tag == 'FlowMonitor':
                sim = Simulation(elem)
                sim_list.append(sim)
                elem.clear() # won't need this any more
                sys.stdout.write(".")
                sys.stdout.flush()
    print " done."
    dljitterresult = []
    dldelaysresult = []
    uldelaysresult = []
    throughputresult = []
    packetloss = []
    for sim in sim_list:
        for flow in sim.flows:
            t = flow.fiveTuple
	    packetloss.append(flow.packetLossRatio*100)
	    if t.sourceAddress.find("7.0.0"):
		uldelaysresult.append(flow.delayMean*1e3)
	  
            if t.sourceAddress.find("1.0.0"):
		dljitterresult.append(flow.jitterMean*1e3)    	
		dldelaysresult.append(flow.delayMean*1e3)
		throughputresult.append(flow.rxBitrate*1e-3)
            proto = {6: 'TCP', 17: 'UDP'} [t.protocol]
            print "FlowID: %i (%s %s/%s --> %s/%i)" % \
                (flow.flowId, proto, t.sourceAddress, t.sourcePort, t.destinationAddress, t.destinationPort)
            print "\tTX bitrate: %.2f kbit/s" % (flow.txBitrate*1e-3,)
            print "\tRX bitrate: %.2f kbit/s" % (flow.rxBitrate*1e-3,)
            print "\tMean Delay: %.2f ms" % (flow.delayMean*1e3,)
            print "\tPacket Loss Ratio: %.2f %%" % (flow.packetLossRatio*100)
		
	    #logFile.write( "FlowID: %i (%s %s/%s --> %s/%i) \n" % \
            #	(flow.flowId, proto, t.sourceAddress, t.sourcePort, t.destinationAddress, t.destinationPort))
            #logFile.write( "\tTX bitrate: %.2f kbit/s \n" % (flow.txBitrate*1e-3,))
            #logFile.write( "\tRX bitrate: %.2f kbit/s \n" % (flow.rxBitrate*1e-3,))
            #logFile.write( "\tMean Delay: %.2f ms \n" % (flow.delayMean*1e3,))
            #logFile.write( "\tPacket Loss Ratio: %.2f %% \n" % (flow.packetLossRatio*100))
		
	    logFile.write("%i %s %s/%s --> %s/%i %.2f  %.2f  %.2f %.2f \n" % (flow.flowId, proto, t.sourceAddress, t.sourcePort, t.destinationAddress, t.destinationPort, flow.txBitrate*1e-3, flow.rxBitrate*1e-3, flow.delayMean*1e3, flow.packetLossRatio*100))

    plt.figure()
    plt.hist(dldelaysresult, bins=100)
    plt.xlim([0, 500])
    plt.title("Downlink Delay Histogram")
    plt.xlabel("Delay (ms)")
    plt.ylabel("Number of Flows")
    plt.savefig("DLDelayResult.png")

 
    plt.figure()
    plt.hist(uldelaysresult, bins=100)
    plt.xlim([0, 100])
    plt.title("Uplink Delay Histogram")
    plt.xlabel("Delay (ms)")
    plt.ylabel("Number of Flows")
    plt.savefig("ULDelayResult.png")

    plt.figure()
    plt.hist(dljitterresult, bins=100)
    plt.xlim([0, 100])
    plt.title("Downlink Jitter Histogram")
    plt.xlabel("Jitter (ms)")
    plt.ylabel("Number of Flows")
    plt.savefig("DlJitterResult.png")

    plt.figure()
    plt.hist(throughputresult, bins=100)
    plt.xlim([0, 1000])
    plt.title("Downlink throughput Histogram")
    plt.xlabel("throughput (kbps)")
    plt.ylabel("Number of Flows")
    plt.savefig("throughput.png")

    plt.figure()
    plt.hist(packetloss, bins=100)
   # plt.xlim([0, 100000000])
    plt.title("Packet Loss Histogram")
    plt.xlabel("Packet Loss Ratio %")
    plt.ylabel("Number of Flows")
    plt.savefig("PacketLoss.png")


#    print "\t Delay: %0.2f" % (mean(dldelaysresult))



def openFile(filename, mode):
    
    try:
      file = open(filename, mode) #Data sent from ONU 0, file 0
    except IOError:
      print "Could not open file:", filename
      quit()

    return file
  
if __name__ == '__main__':
    main(sys.argv)
