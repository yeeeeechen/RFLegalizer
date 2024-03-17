mkdir -p best

GFP_BIN_PATH="../../Global_Floorplanning"
INPUT_PATH="${GFP_BIN_PATH}/inputs/${1}-input.txt"
GFP_OUTPUT_PATH="./best/${1}_global.txt"
LEGAL_RESULTS_PATH="./best"

python3 find_best.py $1 | tee findbest.txt
punishment=$(awk '/Best Punishment:/ {print $3}' findbest.txt)
legal_mode=$(awk '/Legal Mode:/ {print $3}' findbest.txt)
config=$(awk '/Config:/ {print $2}' findbest.txt)

${GFP_BIN_PATH}/global_floorplan -i ${INPUT_PATH} -o ${GFP_OUTPUT_PATH} -p $punishment -a 1.9 \
    | tee best/${1}_global.log

../legal -i ${GFP_OUTPUT_PATH} -o ${LEGAL_RESULTS_PATH} -f ${1} -m ${legal_mode} -c ../${config} \
| tee best/${1}_legal.log

python3 draw_tile_layout.py ${LEGAL_RESULTS_PATH}/${1}_init.txt best/${1}_init.png 0
python3 draw_tile_layout.py ${LEGAL_RESULTS_PATH}/${1}_legal.txt best/${1}_solution.png 0
python3 draw_full_floorplan.py ${LEGAL_RESULTS_PATH}/${1}_legal_fp.txt best/${1}_legal.png 0
python3 draw_full_floorplan.py ${LEGAL_RESULTS_PATH}/${1}_legal_fp.txt best/${1}_legal_conn.png 1


python3 ${GFP_BIN_PATH}/utils/draw_rect_layout.py ${GFP_OUTPUT_PATH} best/${1}_global.png 
python3 ${GFP_BIN_PATH}/utils/draw_rect_layout.py ${GFP_OUTPUT_PATH} best/${1}_global_conn.png -l