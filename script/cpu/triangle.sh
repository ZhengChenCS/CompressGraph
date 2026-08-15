#!/bin/bash

# Navigate to the script's directory
cd "$(dirname "$0")"

# Run the triangle counting executable (with -r 1 for one round)
../../bin/triangle_cpu -r 1 ../../dataset/cnr-2000/ligra/cnr-2000.txt
