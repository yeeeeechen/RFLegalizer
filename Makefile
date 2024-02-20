CXX = g++ 
FLAGS = -std=c++17
CXXFLAGS += -Wall -Wno-unused-function -Wno-write-strings -Wno-sign-compare

SRCPATH = ./src
BOOSTPATH =    	# your boost path here
INCLUDES += -I$(BOOSTPATH)

all: legal
debug: legal_debug

OPTFLAGS = -O3
DEBUGFLAGS = -g
# DEBUGFLAGS = -gdwarf-4

SRC := \
	src/DFSLConfig.cpp src/DFSLegalizer.cpp src/LFLegaliser.cpp src/LFUnits.cpp \
	src/Tessera.cpp src/Tile.cpp src/main.cpp

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
	rm -rf legal legal_debug ./src/*.o 
