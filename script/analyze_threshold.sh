#!/bin/bash

# --- Configuration ---
THRESHOLDS=(8 16 32 64 128)
BIN_DIR="../bin"
DATA_DIR="../dataset"

# --- Source File Paths ---
# This is the graph created by data_prepare.sh (Ligra format)
LIGRA_GRAPH_FILE="${DATA_DIR}/cnr-2000/ligra/cnr-2000.txt" # Path from bfs.sh
# These are the original CSR binaries found in the /compress subdirectory
ORIGINAL_VLIST="../dataset/cnr-2000/compress/csr_vlist.bin"
ORIGINAL_ELIST="../dataset/cnr-2000/compress/csr_elist.bin"
ORIGINAL_INFO="../dataset/cnr-2000/compress/info.bin"

# Create a temporary directory for our test files
TEMP_DIR="../temp_analysis"
mkdir -p $TEMP_DIR

# --- Check for baseline files ---
# Ensure the .bin files exist in the correct subdirectory
if [ ! -f "$ORIGINAL_VLIST" ]; then
    echo "Error: Original files (vlist.bin, etc.) not found in ../dataset/cnr-2000/compress/" # Updated error message path
    echo "Please run 'bash data_prepare.sh' once to generate them."
    rm -rf $TEMP_DIR
    exit 1
fi

# Get the size of the original uncompressed CSR graph for comparison
BASELINE_SIZE=$(($(stat -c%s "$ORIGINAL_VLIST") + $(stat -c%s "$ORIGINAL_ELIST")))

echo "Baseline Uncompressed CSR Size: $BASELINE_SIZE bytes"
echo "--- Starting Analysis ---"
echo "threshold,filtered_size_bytes,size_ratio,bfs_time_seconds"

# --- Main Loop ---
for t in "${THRESHOLDS[@]}"; do
    echo -n "$t," # Print threshold

    # 1. Copy original binaries to temp dir
    cp "$ORIGINAL_VLIST" "$TEMP_DIR/vlist.bin"
    cp "$ORIGINAL_ELIST" "$TEMP_DIR/elist.bin"
    cp "$ORIGINAL_INFO" "$TEMP_DIR/info.bin"

    # 2. Run filter with the current threshold
    # We run this from inside TEMP_DIR so it finds the files
    (cd $TEMP_DIR && $BIN_DIR/filter vlist.bin elist.bin info.bin $t) > /dev/null 2>&1

    # 3. Measure filtered size
    FILTERED_SIZE=$(($(stat -c%s "$TEMP_DIR/vlist.bin") + $(stat -c%s "$TEMP_DIR/elist.bin")))
    echo -n "$FILTERED_SIZE,"

    # Calculate and print size ratio
    # Ensure bc is installed: sudo apt install bc
    SIZE_RATIO=$(echo "scale=4; $FILTERED_SIZE / $BASELINE_SIZE" | bc)
    echo -n "$SIZE_RATIO,"

    # 4. Prepare Ligra data from the new filtered binaries
    # Output the .ligra file to the temp directory
    $BIN_DIR/convert2ligra "$TEMP_DIR/vlist.bin" "$TEMP_DIR/elist.bin" > "$TEMP_DIR/graph.ligra"

    # 5. Run and time BFS (run 3 times, get 3rd run's time)
    # Pass the .ligra file generated in the temp dir
    BFS_TIME=$($BIN_DIR/bfs_cpu -r 3 "$TEMP_DIR/graph.ligra" | grep "Running time" | tail -n 1 | cut -d' ' -f4)
    echo "$BFS_TIME" # Last value, print newline

done

# --- Cleanup ---
rm -rf $TEMP_DIR
echo "--- Analysis Complete ---"
