CXX = g++ 
FLAGS = -std=c++17
CXXFLAGS += -Wall -Wno-unused-function -Wno-write-strings -Wno-sign-compare

SRC_PATH = ./src
LEGAL_SRC_PATH = $(SRC_PATH)/legal
FP_SRC_PATH = $(SRC_PATH)/fp
BOOST_PATH = ./boost_1_82_0

INCLUDES += -I$(BOOST_PATH) -I$(SRC_PATH)

all: legal
debug: legal_debug

OPTFLAGS = -O3
DEBUGFLAGS = -g
# DEBUGFLAGS = -gdwarf-4
 
# If there are new source files, add them here
LEGAL_SRC := \
	DFSLConfig.cpp DFSLegalizer.cpp LFLegaliser.cpp LFUnits.cpp \
	Tessera.cpp Tile.cpp 

FP_SRC := \
	cSException.cpp globalResult.cpp \
	cord.cpp line.cpp rectangle.cpp doughnutPolygon.cpp tile.cpp \
	cornerStitching.cpp rectilinear.cpp connection.cpp \
	floorplan.cpp

SRC += $(patsubst %, $(LEGAL_SRC_PATH)/%, $(LEGAL_SRC))
SRC += $(patsubst %, $(FP_SRC_PATH)/%, $(FP_SRC))
SRC += $(SRC_PATH)/main.cpp

OBJ_ALL = $(patsubst %.cpp, %_all.o, $(SRC))
OBJ_DEBUG = $(patsubst %.cpp, %_db.o, $(SRC))


legal: $(OBJ_ALL)
	@echo "Building binary: " $(notdir $@)
	$(CXX) $(FLAGS) $(OPTFLAGS) $(INCLUDES) $(CXXFLAGS) $^ -o $@

legal_debug: $(OBJ_DEBUG)
	@echo "Building binary: " $(notdir $@)
	$(CXX) $(FLAGS) $(OPTFLAGS) $(INCLUDES) $(CXXFLAGS) $^ -o $@

%_all.o: %.cpp
	@echo "Compiling: " $(notdir $@)
	$(CXX) $(FLAGS) -c $(OPTFLAGS) $(INCLUDES) $(CXXFLAGS) $< -o $@

%_db.o: %.cpp
	@echo "Compiling: " $(notdir $@)
	$(CXX) $(FLAGS) -c $(DEBUGFLAGS) $(INCLUDES) $(CXXFLAGS) $< -o $@


clean:
	@echo "Cleaning up..."
	rm -rf legal legal_debug $(LEGAL_SRC_PATH)/*.o $(FP_SRC_PATH)/*.o 
