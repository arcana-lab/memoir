#!/bin/bash -e

OUTPUT_FILE="${1}"
shift

touch ${OUTPUT_FILE}
echo -n "" > ${OUTPUT_FILE}

SLOC="scc -c"

declare -A PASSES=(["DEE"]="slice_*" \
                   ["DFE"]="dead_field_elimination" \
                   ["FE"]="field_elision" \
                   ["RIE"]="key_folding" \
                   ["SSAConstruction"]="mut_to_immut" \
                   ["SSADestruction"]="ssa_destruction" \
                  )

printf "\e[3m%-20s\e[0m ┃ \e[3m%-10s\e[0m\n" "Pass" "SLOC" >> ${OUTPUT_FILE}
for i in $(seq 1 21) ; do printf "━"  >> ${OUTPUT_FILE}; done
printf "╋" >> ${OUTPUT_FILE}
for i in $(seq 1 11) ; do printf "━" >> ${OUTPUT_FILE} ; done
printf "\n" >> ${OUTPUT_FILE}
for KEY in "${!PASSES[@]}"
do
    printf "%-20s ┃ " "${KEY}" >> ${OUTPUT_FILE}
    scc -c ${PASSES[${KEY}]} | awk -f code_complexity.awk >> ${OUTPUT_FILE}
done

cat ${OUTPUT_FILE}
