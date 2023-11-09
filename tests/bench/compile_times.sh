#!/bin/bash -e

IR_FILE="${1}"
shift
OUTPUT_FILE="${1}"
shift

OPT=memoir-load

touch ${OUTPUT_FILE}
echo "config,opt,run,time" > ${OUTPUT_FILE}

# MemOIR O0
for i in $(seq 1 10) ; do
    echo -n "memoir,O0,${i}," >> ${OUTPUT_FILE}
    2>&1 ${OPT} -time-passes --mut2immut --ssa-destruction ${IR_FILE} -disable-output | awk -f compile_times.awk >> ${OUTPUT_FILE}
done

# MemOIR O3
for i in $(seq 1 10) ; do
    echo -n "memoir,O3,${i}," >> ${OUTPUT_FILE}
    2>&1 ${OPT} -time-passes --mut2immut --memoir-fe --memoir-kf --memoir-dfe --ssa-destruction -O3 ${IR_FILE} -disable-output | awk -f compile_times.awk >> ${OUTPUT_FILE}
done

# LLVM O0
for i in $(seq 1 10) ; do
    echo -n "llvm,O0,${i}," >> ${OUTPUT_FILE}
    2>&1 opt -time-passes -O0 ${IR_FILE} -disable-output | awk -f compile_times.awk >> ${OUTPUT_FILE}
done

# LLVM O3
for i in $(seq 1 10) ; do
    echo -n "llvm,O3,${i}," >> ${OUTPUT_FILE}
    2>&1 opt -time-passes -O3 ${IR_FILE} -disable-output | awk -f compile_times.awk >> ${OUTPUT_FILE}
done
