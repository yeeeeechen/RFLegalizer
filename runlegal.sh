make

if [ "$#" -ne 3 ]; then
    strategy=0
    mode=0
else
    strategy=$2
    mode=$3
fi

echo "Command: ./legal inputs/${1}.txt $strategy $mode | tee log/${1}.log"
./legal inputs/${1}.txt $strategy $mode | tee log/${1}.log

echo 
echo "Drawing output:"
echo outputs/${1}_solution.png
echo outputs/${1}_init.png
python3 utils/draw_tile_layout.py outputs/${1}_legal.txt outputs/${1}_solution.png
python3 utils/draw_tile_layout.py outputs/${1}_phase2.txt outputs/${1}_init.png

eog outputs/${1}_solution.png &
eog outputs/${1}_init.png &