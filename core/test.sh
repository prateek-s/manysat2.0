#!/bin/bash
f=/home/prateeks/sat/sc14-app/aes_32_3_keyfind_1.cnf
./manysat2.0 -limitEx=10 -ncores=4 $f

#1>>../results/$category/$g.out
