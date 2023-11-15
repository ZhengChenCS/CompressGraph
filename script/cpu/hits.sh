exec=../../bin/hits_cpu
file=../../dataset/cnr-2000/ligra/cnr-2000.txt
info=../../dataset/cnr-2000/compress/info.bin
order=../../dataset/cnr-2000/compress/order.bin

$exec -maxiters 10 -i $info -o $order $file