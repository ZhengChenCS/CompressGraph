#!/bin/bash

# --- Safety Check ---
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <graph_name_in_dataset_dir_no_ext>"
    echo "Example: ./run_full_pipeline.sh ca-GrQc"
    exit 1
fi

# --- Configuration ---
GRAPH_NAME=$1
BIN_DIR="../bin"
DATA_DIR="../dataset"
GRAPH_EDGELIST="${DATA_DIR}/${GRAPH_NAME}.edgelist"

# Create a results directory for this graph
RESULTS_DIR="../${GRAPH_NAME}_results"
mkdir -p $RESULTS_DIR

if [ ! -f "$GRAPH_EDGELIST" ]; then
    echo "Error: Edgelist not found at $GRAPH_EDGELIST"
    exit 1
fi

echo "--- Processing $GRAPH_NAME ---"
cd $RESULTS_DIR # Run everything from this new directory

# --- Step 1: Edgelist to CSR ---
echo "Converting edgelist to CSR..."
# Pipe the edgelist into the converter. It outputs files in the current dir.
cat $GRAPH_EDGELIST | $BIN_DIR/edgelist2csr
# Creates csr_vlist.bin and csr_elist.bin

# --- Step 2: Compress ---
echo "Compressing graph..."
# Compress reads and *overwrites* its inputs, so we run it on our files
# We redirect stderr (2>) to a file to capture the stats
$BIN_DIR/compress csr_vlist.bin csr_elist.bin 2> compress_stats.txt
echo "Compression stats:"
cat compress_stats.txt

# --- Step 3: Filter (use default 16) ---
echo "Filtering rules..."
$BIN_DIR/filter csr_vlist.bin csr_elist.bin info.bin 16 > /dev/null 2>&1

# --- Step 4: Data Prep for Ligra ---
echo "Preparing data for Ligra..."
$BIN_DIR/convert2ligra csr_vlist.bin csr_elist.bin > ${GRAPH_NAME}.ligra
$BIN_DIR/save_degree csr_vlist.bin > ${GRAPH_NAME}.degree
$BIN_DIR/gene_rule_order csr_vlist.bin csr_elist.bin info.bin > ${GRAPH_NAME}.order

# --- Step 5: Run Analytics (BFS and PageRank) ---
echo "Running BFS..."
$BIN_DIR/bfs_cpu -r 1 ${GRAPH_NAME}.ligra

echo "Running PageRank..."
$BIN_DIR/pagerank_cpu -maxiters 10 -i info.bin -d ${GRAPH_NAME}.degree -o ${GRAPH_NAME}.order ${GRAPH_NAME}.ligra

echo "--- Finished $GRAPH_NAME ---"
echo "All results are in $RESULTS_DIR"
