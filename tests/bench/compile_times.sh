#!/bin/bash -e

OUTPUT_FILE="${1}"
shift
NUM_RUNS="${1}"
shift
BENCHMARKS="${@}"

OPT=memoir-load

touch ${OUTPUT_FILE}
echo "" > ${OUTPUT_FILE}

TMP_FILE=$(mktemp)
echo -n "" > ${TMP_FILE}

function median {
    INPUT_FILE="${1}"

    sort ${INPUT_FILE} | \
        awk '
{
  a[NR]=$1
}
END {
  m=int(NR/2); 
  if (NR % 2)
   printf "%-10f", a[m-1]
  else
   printf "%-10f", (a[m-1]+a[m])/2
}
'
}

printf "%-20s ┃ %-20s\n" "" "# Collections" >> ${OUTPUT_FILE}
printf "%-20s ┃ %-23s ┃ %-23s\n" "" "MemOIR" "LLVM" >> ${OUTPUT_FILE}
printf "%-20s ┃ %-10s ┃ %-10s ┃ %-10s ┃ %-10s\n" "Benchmark" "O0" "O3" "O0" "O3" >> ${OUTPUT_FILE}
for i in $(seq 1 21) ; do printf "━" >> ${OUTPUT_FILE} ; done
printf "╋" >> ${OUTPUT_FILE}
for i in $(seq 1 12) ; do printf "━" >> ${OUTPUT_FILE} ; done
printf "╋" >> ${OUTPUT_FILE}
for i in $(seq 1 12) ; do printf "━" >> ${OUTPUT_FILE} ; done
printf "╋" >> ${OUTPUT_FILE}
for i in $(seq 1 12) ; do printf "━" >> ${OUTPUT_FILE} ; done
printf "╋" >> ${OUTPUT_FILE}
for i in $(seq 1 12) ; do printf "━" >> ${OUTPUT_FILE} ; done
printf "\n" >> ${OUTPUT_FILE}


for BENCH in ${BENCHMARKS}
do
    BENCH_NAME="$(basename ${BENCH})"
    
    IR_FILE=${BENCH}/build/normalized.bc

    # MemOIR O0
    for i in $(seq 1 ${NUM_RUNS}) ; do
        2>&1 ${OPT} -time-passes --mut2immut --ssa-destruction ${IR_FILE} -disable-output | awk -f compile_times.awk >> ${TMP_FILE}
    done

    MEMOIR_O0_MEDIAN="$(median ${TMP_FILE})"
    echo "" > ${TMP_FILE}
    
    # MemOIR O3
    for i in $(seq 1 ${NUM_RUNS}) ; do
        2>&1 ${OPT} -time-passes --mut2immut --memoir-fe --memoir-kf --memoir-dfe --ssa-destruction -O3 ${IR_FILE} -disable-output | awk -f compile_times.awk >> ${TMP_FILE}
    done

    MEMOIR_O3_MEDIAN="$(median ${TMP_FILE})"
    echo "" > ${TMP_FILE}
    
    # LLVM O0
    for i in $(seq 1 ${NUM_RUNS}) ; do
        2>&1 opt -time-passes -O0 ${IR_FILE} -disable-output | awk -f compile_times.awk >> ${TMP_FILE}
    done

    LLVM_O0_MEDIAN="$(median ${TMP_FILE})"
    echo "" > ${TMP_FILE}
    
    # LLVM O3
    for i in $(seq 1 ${NUM_RUNS}) ; do
        2>&1 opt -time-passes -O3 ${IR_FILE} -disable-output | awk -f compile_times.awk >> ${TMP_FILE}
    done

    LLVM_O3_MEDIAN="$(median ${TMP_FILE})"
    echo "" > ${TMP_FILE}

    printf "%-20s ┃ %-10s ┃ %-10s ┃ %-10s ┃ %-10s\n" "${BENCH_NAME}" "${MEMOIR_O0_MEDIAN}" "${MEMOIR_O3_MEDIAN}" "${LLVM_O0_MEDIAN}" "${LLVM_O3_MEDIAN}" >> ${OUTPUT_FILE}

done
