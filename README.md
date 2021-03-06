# Reproduce results of RACK

This repository contains source code related to Recent Acknowledgment (RACK) paper published in the Proceedings of ACM WNS3 2019.

## Steps to reproduce results:

1. Clone this repo:
```
git clone https://github.com/ShikhaBakshi/Reproduce-RACK-Results
```

2. Configure and build the cloned repo.
```
./waf configure --enable-tests
./waf
```

3. Run the script results.sh
```
./results.sh
```

On running this, a folder named ```"rack"``` will be created in the main ns-3 directory containing all the results.

Results for Figure 5 in the paper will be generated in ```rack/no-reordering/rack/Traces``` <br>
Results for Figure 6 in the paper will be generated in ```rack/no-reordering/no-rack/Traces``` <br>
Results for Figure 7 and 8 in the paper will be generated in ```rack/reordering/no-rack/Traces```  and ```rack/reordering/rack/Traces```

4. Run the script http.sh

On running this script two folders ```rack_on``` and ```rack-off``` will be created. These folders contain the results of the
```Web Traffic Scenario``` run with 25 different seed values in both RACK enabled and RACK disabled scenario.

Steps To generate CDF graphs:

a. Each simulation result folder contains ```client-RTT``` files for all the five clients. We need to take the average of all the RTT columns (column 2) of all the 25 simulations for each client individually. <br>
b. We do this for both RACK ON and RACK OFF cases. <br>
c. At the end we generate the CDF graphs with the data obtained by averaging the RTTs for each client in both the scenarios
and overlap them.

5. To generate the results of Test 1 and Test 2, run the below command:

```
./test.py --suite="tcp-rack-test"
```
It runs both test cases described in the paper and generates the trace files ```rack-on.plotme``` and ```rack-off.plotme``` corresponding to Figure 4 of the paper.
