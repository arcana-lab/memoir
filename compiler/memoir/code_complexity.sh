#!/bin/bash -e

# Parse input.
OUTPUT_FILE="${1}"
shift

# Get the directory of this script.
SOURCE=${BASH_SOURCE[0]}
while [ -L "${SOURCE}" ]; do # resolve $SOURCE until the file is no longer a symlink
  DIR=$( cd -P "$( dirname "${SOURCE}" )" >/dev/null 2>&1 && pwd )
  SOURCE=$(readlink "${SOURCE}")
  # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
  [[ ${SOURCE} != /* ]] && SOURCE=$DIR/$SOURCE 
done
SCRIPT_DIR=$( cd -P "$( dirname "${SOURCE}" )" >/dev/null 2>&1 && pwd )

# Setup output file.
touch ${OUTPUT_FILE}
echo -n "" > ${OUTPUT_FILE}

# Declare passes to evaluate.
declare -A PASSES=(["DEE"]="slice_*" \
                   ["DFE"]="dead_field_elimination" \
                   ["FE"]="field_elision" \
                   ["RIE"]="key_folding" \
                   ["SSA Construction"]="mut_to_immut" \
                   ["SSA Destruction"]="ssa_destruction" \
                  )

# Create table.
printf "\e[3m%-20s\e[0m ┃ \e[3m%-10s\e[0m\n" "Pass" "SLOC" >> ${OUTPUT_FILE}
for i in $(seq 1 21) ; do printf "━"  >> ${OUTPUT_FILE}; done
printf "╋" >> ${OUTPUT_FILE}
for i in $(seq 1 11) ; do printf "━" >> ${OUTPUT_FILE} ; done
printf "\n" >> ${OUTPUT_FILE}
for KEY in "${!PASSES[@]}"
do
    printf "%-20s ┃ " "${KEY}" >> ${OUTPUT_FILE}
    scc -c ${SCRIPT_DIR}/${PASSES[${KEY}]} | awk -f ${SCRIPT_DIR}/code_complexity.awk >> ${OUTPUT_FILE}
done

# Print output to display.
cat ${OUTPUT_FILE}
