#!/usr/bin/env python
"""
@file    randomTrips.py
@author  Daniel Krajzewicz
@author  Jakob Erdmann
@author  Michael Behrisch
@date    2010-03-06
@version $Id$

Generates random trips for the given network.

SUMO, Simulation of Urban MObility; see http://sumo-sim.org/
Copyright (C) 2010-2014 DLR (http://www.dlr.de/) and contributors

This file is part of SUMO.
SUMO is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.
"""

import os, sys, random, bisect, datetime, subprocess
import math
import optparse
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import sumolib.net


def get_options():
    optParser = optparse.OptionParser()
    optParser.add_option("-n", "--net-file", dest="netfile",
                            help="define the net file (mandatory)")
    optParser.add_option("-o", "--output-trip-file", dest="tripfile",
                         default="trips.trips.xml", help="define the output trip filename")
    optParser.add_option("-r", "--route-file", dest="routefile",
                         help="generates route file with duarouter")
    optParser.add_option("-t", "--trip-id-prefix", dest="tripprefix",
                         default="", help="prefix for the trip ids")
    optParser.add_option("-a", "--trip-parameters", dest="trippar",
                         default="", help="additional trip parameters")
    optParser.add_option("-b", "--begin", type="float", default=0, help="begin time")
    optParser.add_option("-e", "--end", type="float", default=3600, help="end time (default 3600)")
    optParser.add_option("-p", "--period", type="float", default=1, help="repetition period (default 1)")
    optParser.add_option("-s", "--seed", type="int", help="random seed")
    optParser.add_option("-l", "--length", action="store_true",
                         default=False, help="weight edge probability by length")
    optParser.add_option("-L", "--lanes", action="store_true",
                         default=False, help="weight edge probability by number of lanes")
    optParser.add_option("--speed-exponent", type="float", dest="speed_exponent",
                         default=0.0, help="weight edge probability by speed^<FLOAT> (default 0)")
    optParser.add_option("--fringe-factor", type="float", dest="fringe_factor",
                         default=1.0, help="multiply weight of fringe edges by <FLOAT> (default 1")
    optParser.add_option("--fringe-threshold", type="float", dest="fringe_threshold",
                         default=0.0, help="only consider edges with speed above <FLOAT> as fringe edges (default 0)")
    optParser.add_option("--min-distance", type="float", dest="min_distance",
                         default=0.0, help="require start and end edges for each trip to be at least <FLOAT> m appart (default 0)")
    optParser.add_option("-c", "--vclass", 
                         help="only from and to edges which permit <vClass>")
    optParser.add_option("-v", "--verbose", action="store_true",
                         default=False, help="tell me what you are doing")
    (options, args) = optParser.parse_args()
    if not options.netfile:
        optParser.print_help()
        sys.exit()
    return options



# euclidean distance between two coordinates in the plane
def euclidean(a, b):
    return math.sqrt((a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2)

# assigns a weight to each edge using weight_fun and then draws from a discrete
# distribution with these weights
class RandomEdgeGenerator:
    def __init__(self, net, weight_fun):
        self.net = net
        self.cumulative_weights = []
        self.total_weight = 0
        for edge in self.net._edges:
            #print edge.getID(), weight_fun(edge)
            self.total_weight += weight_fun(edge)
            self.cumulative_weights.append(self.total_weight)

    def get(self):
        r = random.random() * self.total_weight
        index = bisect.bisect(self.cumulative_weights, r)
        return self.net._edges[index]


class RandomTripGenerator:
    def __init__(self, source_generator, sink_generator):
        self.source_generator = source_generator
        self.sink_generator = sink_generator

    def get_trip(self, min_distance, maxtries=100):
        for i in range(maxtries):
            source_edge = self.source_generator.get()
            sink_edge = self.sink_generator.get()
            distance = euclidean(source_edge.getFromNode().getCoord(), 
                    sink_edge.getToNode().getCoord())
            if distance >= min_distance:
                return source_edge, sink_edge
        raise Exception("no trip found after %s tries" % (type, maxtries))


def get_prob_fun(options, fringe_bonus, fringe_forbidden):
    def edge_probability(edge):
        if options.vclass and not edge.allows(options.vclass):
            return 0
        if edge.is_fringe(getattr(edge, fringe_forbidden)):
            return 0
        prob = 1
        if options.length:
            prob *= edge.getLength()
        if options.lanes:
            prob *= edge.getLaneNumber()
        prob *= (edge.getSpeed() ** options.speed_exponent)
        if (options.fringe_factor != 1.0 and
                edge.getSpeed() > options.fringe_threshold and
                edge.is_fringe(getattr(edge, fringe_bonus))):
            prob *= options.fringe_factor
        return prob
    return edge_probability


def main(options):
    if options.seed:
        random.seed(options.seed)

    net = sumolib.net.readNet(options.netfile)
    if options.min_distance > net.getBBoxDiameter():
        sys.exit("Cannot find trips with min-distance %s for net with diamter %s" % (
            options.min_distance, self.net_diameter))


    edge_generator = RandomTripGenerator(
            RandomEdgeGenerator(net, get_prob_fun(options, "_incoming", "_outgoing")),
            RandomEdgeGenerator(net, get_prob_fun(options, "_outgoing", "_incoming")))

    idx = 0
    with open(options.tripfile, 'w') as fouttrips:
        print >> fouttrips, """<?xml version="1.0"?>
<!-- generated on %s by $Id$ -->
<trips>""" % datetime.datetime.now()
        depart = options.begin
        while depart < options.end:
            label = "%s%s" % (options.tripprefix, idx)
            source_edge, sink_edge = edge_generator.get_trip(options.min_distance)
            print >> fouttrips, '    <trip id="%s" depart="%.2f" from="%s" to="%s" %s/>' % (
                    label, depart, source_edge.getID(), sink_edge.getID(), options.trippar)
            idx += 1
            depart += options.period
        fouttrips.write("</trips>")

    if options.routefile:
        subprocess.call(['duarouter', '-n', options.netfile, '-t', options.tripfile, '-o', options.routefile, '--ignore-errors',
            '--begin', str(options.begin), '--end', str(options.end)])


if __name__ == "__main__":
    main(get_options())
