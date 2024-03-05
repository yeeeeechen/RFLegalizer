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

LEGAL_SRC_PATH = src/legal
FP_SRC_PATH = src/fp
 
# If there are new source files, add them here
LEGAL_SRC := \
	$(LEGAL_SRC_PATH)/DFSLConfig.cpp $(LEGAL_SRC_PATH)/DFSLegalizer.cpp $(LEGAL_SRC_PATH)/LFLegaliser.cpp $(LEGAL_SRC_PATH)/LFUnits.cpp \
	$(LEGAL_SRC_PATH)/Tessera.cpp $(LEGAL_SRC_PATH)/Tile.cpp $(LEGAL_SRC_PATH)/main.cpp

FP_SRC := \


SRC += LEGAL_SRC
SRC += FP_SRC

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
