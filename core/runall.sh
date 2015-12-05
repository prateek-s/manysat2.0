#!/bin/bash

category="app" #crafted random
sharing=100
parallelism=8

#for f in `ls -Sr /home/sat/sc14-$category/*.cnf` ; 
for f in $(cat "$category"_small);
do 
    #xz --decompress $f
    echo $f
    #g=${f%.*} #remove extension
    g=$(basename $f)
    start=`date +%s`
    timeout 1000 ./manysat2.0 -limitEx=$sharing  -ncores=$parallelism $f 1>>../results/$category/$g.out 
    end=`date +%s`
    diff=$((end-start))
    echo "$f , $diff" >> "$category"_results_"$sharing"_"$parallelism"
done 


