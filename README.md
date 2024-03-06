# Rectilinear Floorplan Legalizer

Legalization stage of [Soft Block Floorplanner](https://github.com/hankshyu/LocalFloorplanning).

## Prerequisites
1. Make sure your `gcc/g++` supports C++17. 
2. Make sure to download the [Boost library (1.82.0)](https://www.boost.org/users/history/version_1_82_0.html) to somewhere on your machine.
3. Use `pip3` to install `numpy` and `matplotlib`.
```bash
pip3 install numpy
pip3 install matplotlib
```

## Installation

1. Clone the git repo.
```bash
git clone https://github.com/yeeeeechen/RFLegalizer.git
```

2. Modify the Makefile. In line 6, change the boost path to your own Boost path.
```Makefile
BOOSTPATH = ./boost_1_82_0    # example
```

3. Compile the floorplanner.
```bash
make
```

## Usage

### Legalization Testing:
```bash
./runscript.sh <case name> [legal strategy 0-4 (default:0)] [legal mode 0-3 (default:0)]
```
eg. `./runscript.sh case01 0 1`

Legalization strategies are a set of configs that changes the legalization behavior:
- Strategy 0: Default configs
- Strategy 1: Prioritizes area flowing to larger area blocks
- Strategy 2: Prioritizes solving utilization constraint
- Strategy 3: Prioritizes solving aspect ratio constraint
- Strategy 4: Encourages block -> block flow


Legalization modes affects the order in which overlaps are resolved:
- mode 0: resolve area big -> area small
- mode 1: resolve area small -> area big
- mode 2: resolve overlaps near center -> outer edge
- mode 3: completely random

### Custom Configs
If you want to use custom configs, create a config with the following syntax:
```conf
# myConfigs.conf
# This is comment
[config 1] = [value]
[config 2]=[value]
...
```
And specify the path to this file in the command line using the `-c [config path]` option;

The higher configs labeled `*Weight` are, the more a move will be punished if that metric is worse. Eg. if a block->block move (let's say Block A and Block B) causes Block A's utilization to be lower, then that move will be punished by an amount proportional to `(1.0 - (block A util)) * BBFromUtilWeight`.

The more negative configs labelled `*PosRein` are, the more a move will be rewarded if that metric is improved. Eg. if a block->whitespace (let's say block A) move causes block A's utilization to be higher, then that move will be rewarded by an amount proportional to `((old block A util) - (new block A util)) * BWUtilPosRein`.

Further details can be found in the function `DFSLegalizer::getEdgeCost()` and file `DFSLConfig.cpp`.