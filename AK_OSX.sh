#!/bin/bash

bench=$1
outfile=$2
seed=$3

shift 3
TotalEvaluations=1000
alpha=1
beta=5
ph_max=6
ph_min=0.01
rho=0.01
TotalAnts=30


while [ $# != 0 ]; do
    flag="$1"
    case "$flag" in
        -TotalAnts) if [ $# -gt 1 ]; then
              arg="$2"
              shift
              TotalAnts=$arg
            fi
            ;;
        -alpha) if [ $# -gt 1 ]; then
              arg="$2"
              shift
              alpha=$arg
            fi
            ;;
        -beta) if [ $# -gt 1 ]; then
              arg="$2"
              shift
              beta=$arg
            fi
	    ;;
        -ph_max) if [ $# -gt 1 ]; then
              arg="$2"
              shift
              ph_max=$arg
            fi
	    ;;
        -rho) if [ $# -gt 1 ]; then
              arg="$2"
              shift
              rho=$arg
            fi
	    ;;
        *) echo "Unrecognized flag or argument: $flag"
            ;;
        esac
    shift
done

rm -rf ${outfile}


#echo "./AntKnapsack_OSX ${bench} ${seed} ${TotalAnts} ${TotalEvaluations} ${alpha} ${beta} ${ph_max} ${ph_min} ${rho} >> ${outfile}"
./AntKnapsack_OSX ${bench} ${seed} ${TotalAnts} ${TotalEvaluations} ${alpha} ${beta} ${ph_max} ${ph_min} ${rho} >> ${outfile}
#done
