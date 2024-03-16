min=$2
interval=$3
max=$4
ar=$5

legal_mode=2

conf_path="./configs/default.conf"

maxOverlap=2.5

GFP_BIN_PATH="../Global_Floorplanning"
GFP_RESULTS_PATH="./records/global"
LEGAL_RESULTS_PATH="./records/legal"
CSV_DIR_PATH="./records/csv"

INPUT_PATH=${GFP_BIN_PATH}/inputs/${1}-input.txt
GFP_LOG_PATH=${GFP_RESULTS_PATH}/${1}_global.log
GFP_OUTPUT_PATH=${GFP_RESULTS_PATH}/${1}.txt
LEGAL_LOG_PATH=${LEGAL_RESULTS_PATH}/${1}_legal.log
CSV_PATH=${CSV_DIR_PATH}/${1}.csv

mkdir -p ${GFP_RESULTS_PATH} ${LEGAL_RESULTS_PATH} ${CSV_DIR_PATH}
make

for ((i = 0; i <= $3; i++)); do
    punishment=$(python3 -c "print($min + ($max - $min) * $i / ${3})")
    ${GFP_BIN_PATH}/global_floorplan -i ${INPUT_PATH} -o ${GFP_OUTPUT_PATH} -p $punishment -a $ar \
        | tee ${GFP_LOG_PATH}
    overlap=$(awk '/Overlap Ratio/ {print $5}' ${GFP_LOG_PATH} | sed 's/%//')
    
    # bash only allows integer comparison smoge
    if (( $(echo $overlap $maxOverlap | awk '{if ($1 < $2) print 1;}') ))
    then 
        echo "Legalizing"
        ./legal -i ${GFP_OUTPUT_PATH} -o ${LEGAL_RESULTS_PATH} -f ${1} -m ${legal_mode} -c ${conf_path} \
        | tee ${LEGAL_LOG_PATH}
    else 
        echo "Overlap too high, skipping"
        echo TOOMUCHOVERLAP > ${LEGAL_LOG_PATH}
    fi
    echo "-------------------------------------------------"
    python3 utils/record.py ${1} ${GFP_LOG_PATH} ${LEGAL_LOG_PATH} ${CSV_PATH}
done

# ./fulltest.sh case01 0.005 50 0.105 1.9 