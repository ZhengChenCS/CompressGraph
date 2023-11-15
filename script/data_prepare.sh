compress=../bin/compress
filter=../bin/filter
save_degree=../bin/save_degree
gene_rule_order=../bin/gene_rule_order
convert2ligra=../bin/convert2ligra
csr_vlist=../dataset/cnr-2000/origin/csr_vlist.bin
csr_elist=../dataset/cnr-2000/origin/csr_elist.bin

$save_degree $csr_vlist
mv degree.bin ../dataset/cnr-2000/origin/
$compress $csr_vlist $csr_elist
$filter csr_vlist.bin csr_elist.bin info.bin 16
$convert2ligra csr_vlist.bin csr_elist.bin > ../dataset/cnr-2000/ligra/cnr-2000.txt
$gene_rule_order csr_vlist.bin csr_elist.bin info.bin
mv *.bin ../dataset/cnr-2000/compress 
