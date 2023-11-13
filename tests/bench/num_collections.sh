#!/bin/bash -e

OUTPUT_FILE="${1}"
shift
BENCHES="${@}"

OPT=memoir-load

touch ${OUTPUT_FILE}
echo -n "" > ${OUTPUT_FILE}
printf "%-20s ┃ %-10s ┃ %-10s ┃ %-10s\n" "Benchmark" "Source" "MemOIR" "Binary" >> ${OUTPUT_FILE}

for i in $(seq 1 21) ; do printf "━" >> ${OUTPUT_FILE} ; done
printf "╋" >> ${OUTPUT_FILE}
for i in $(seq 1 12) ; do printf "━" >> ${OUTPUT_FILE} ; done
printf "╋" >> ${OUTPUT_FILE}
for i in $(seq 1 12) ; do printf "━" >> ${OUTPUT_FILE} ; done
printf "╋" >> ${OUTPUT_FILE}
for i in $(seq 1 12) ; do printf "━" >> ${OUTPUT_FILE} ; done
printf "\n" >> ${OUTPUT_FILE}

for BENCH in ${BENCHES}
do
    printf "%-20s ┃ " "${BENCH}" >> ${OUTPUT_FILE}
    IR_FILE=${BENCH}/build/normalized.bc
    2>&1 ${OPT} --memoir-stats --mut2immut --memoir-stats --ssa-destruction --memoir-stats ${IR_FILE} -disable-output | awk -f num_collections.awk >> ${OUTPUT_FILE}
done

cat ${OUTPUT_FILE}
