#!/bin/bash
f=/home/sat/sc14-app/gss-18-s100.cnf
./manysat2.0 -limitEx=10 -ncores=4 $f

#1>>../results/$category/$g.out
