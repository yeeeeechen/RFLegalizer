set -v
make
./test_legal inputs/legaltest03.txt 0 1 | tee log/legaltest03.log
python3 utils/draw_tile_layout.py outputs/legal_0_1.txt outputs/testlegal_solution.png
python3 utils/draw_tile_layout.py outputs/phase2.txt outputs/testlegal_init.png

eog outputs/testlegal_solution.png &
eog outputs/testlegal_init.png &