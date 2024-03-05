make

if [ "$#" -ne 3 ]; then
    strategy=0
    mode=0
else
    strategy=$2
    mode=$3
fi

mkdir -p log outputs

echo "Command: ./legal inputs/${1}.txt $strategy $mode | tee log/${1}.log"
./legal -i inputs/${1}.txt -f ${1} -s $strategy -m $mode | tee log/${1}.log

echo 
echo "Drawing output:"
echo outputs/${1}_init.png
echo outputs/${1}_solution.png
echo outputs/${1}_solution_fp.png
python3 utils/draw_tile_layout.py outputs/${1}_init.txt outputs/${1}_init.png 0
python3 utils/draw_tile_layout.py outputs/${1}_legal.txt outputs/${1}_solution.png 0
python3 utils/draw_full_floorplan.py outputs/${1}_legal_fp.txt outputs/${1}_solution_fp.png 1

# eog outputs/${1}_solution.png &
# eog outputs/${1}_solution_fp.png &
# eog outputs/${1}_init.png &