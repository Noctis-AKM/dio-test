#!/bin/bash

PERF_DATA=$1
FLAME_DIR=/root/FlameGraph

perf script -i $PERF_DATA &> perf.unfold
$FLAME_DIR/stackcollapse-perf.pl perf.unfold &> perf.folded
$FLAME_DIR/flamegraph.pl perf.folded > perf.svg
