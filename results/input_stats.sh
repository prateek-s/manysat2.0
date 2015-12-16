
for f in `ls *.out`;
do
    echo $f
    cat $f | awk '/variables/{print ; getline ; print ;}' >> input_results.txt
done
