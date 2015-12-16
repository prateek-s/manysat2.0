
for f in `ls *.out`;
do
    echo $f
    g=${f%.*}
    echo $g
    outfile=
    cat $f | awk '/ManySAT 2.2/{print ; for(i=0;i<6;i++)  {getline; if(i==0 || i>4) print; } print ""}' > t

    cat t | grep -v "\-\-\-" | sed 's/|//g' | sed 's/x//g' | awk '{print $5,$NF}' | sed 's/threads//g' | awk '{print $1}' |grep . |awk 'NR % 2==1 {o=$0; next} {print o "," $0}' | sort -n > $outfile
    
 cat t | grep -v "\-\-\-" | sed 's/|//g' | sed 's/x//g' | awk '{print $5,$2}' | grep -v threads | grep  "^[0-9]" | sort -n > $sharing_runtime

done

