exec=../../bin/pagerank_cpu
file=../../dataset/cnr-2000/ligra/cnr-2000.txt
info=../../dataset/cnr-2000/compress/info.bin
degree=../../dataset/cnr-2000/origin/degree.bin
order=../../dataset/cnr-2000/compress/order.bin

$exec -maxiters 10 -i $info -d $degree -o $order $file