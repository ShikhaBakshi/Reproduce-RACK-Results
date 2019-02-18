#!/bin/bash

# RACK on
mkdir rack/http
cd rack/http
mkdir rack_on
cd ../..
for var in 1 3 5 7 10 12 15 18 21 28 35 43 50 70 120 200 800 1200 3000 5000 7000 13000 17000 25000 30000
	    do
	       NS_GLOBAL_VALUE="RngRun=$var" ./waf --run "scratch/web-traffic --rack=true --dsack=true"
	       mv rack/http/dumbbell-http  rack/http/rack_on/rng$var
	       rm -f rack/http/dumbbell-http
	    done


cd rack/http
mkdir rack_off
cd ../..
for var in 1 3 5 7 10 12 15 18 21 28 35 43 50 70 120 200 800 1200 3000 5000 7000 13000 17000 25000 30000
	    do
	       NS_GLOBAL_VALUE="RngRun=$var" ./waf --run "scratch/web-traffic"
	       mv rack/http/dumbbell-http  rack/http/rack_off/rng$var
	       rm -f rack/http/dumbbell-http
	    done
