#!/bin/sh
lli=${LLVMINTERP-lli}
exec $lli \
    /home/dhuff/Halide-HLS/apps/hls_examples/camera_ready_synthesis/app_files/big_apps_32_real/conv2d/conv2d_div39/hls_target/.autopilot/db/a.g.bc ${1+"$@"}
