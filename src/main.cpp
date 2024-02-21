#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cfloat>
#include <stdio.h>

#include "LFUnits.h"
#include "Tile.h"
#include "Tessera.h"
#include "LFLegaliser.h"
#include "DFSLegalizer.h"


int main(int argc, char const *argv[]) {

    if (argc < 2){
        std::cout << "Usage: " << argv[0] << " <filename> [legal strategy 0-4] [legal mode 0-3]\n";
        return 1;
    }

    int legalStrategy;
    if (argc >= 3){
        legalStrategy = atoi(argv[2]);
    }
    else {
        legalStrategy = 0;
    }

    int legalMode;
    if (argc >= 4){
        legalMode = atoi(argv[3]);
    }
    else {
        legalMode = 0;
    }

    LFLegaliser *legaliser = nullptr;
    double bestHpwl = DBL_MAX;
    std::string filename = argv[1];

    // find filename
    std::size_t casenameStart = filename.find_last_of("/\\");
    std::size_t casenameEnd = filename.find_last_of(".");
    if (casenameStart == std::string::npos){
        casenameStart = 0;
    }
    else {
        casenameStart++;
    }
    size_t casenameLength;
    if (casenameEnd <= casenameStart || casenameEnd == std::string::npos){
        casenameLength = std::string::npos;
    }
    else {
        casenameLength = casenameEnd - casenameStart;
    }
    std::string casename = filename.substr(casenameStart, casenameLength);
    std::cout << " casename: " << casename << '\n';
    
    legaliser = new LFLegaliser((len_t) 1000, (len_t) 1000);

    std::cout << "Reading input from " + filename + "...\n";
    legaliser->initFromGlobalFile(filename);
    legaliser->detectfloorplanningOverlaps();

    std::cout << "Splitting overlaps ...";
    legaliser->splitTesseraeOverlaps();
    std::cout << "done!" << std::endl;

    std::cout << "Painting All Tesserae to Canvas" << std::endl;
    legaliser->arrangeTesseraetoCanvas();
    
    std::cout << "Start combinable tile search, ";
    std::vector <std::pair<Tile *, Tile *>> detectMergeTile;
    legaliser->detectCombinableBlanks(detectMergeTile);
    std::cout << detectMergeTile.size() << " candidates found" << std::endl << std::endl;
    for(std::pair<Tile *, Tile *> tp : detectMergeTile){
        legaliser->visualiseAddMark(tp.first);
        legaliser->visualiseAddMark(tp.second);
    }

    legaliser->visualiseRemoveAllmark();
    while(!detectMergeTile.empty()){
        std::cout << "Merging Pair: #" << detectMergeTile.size() << std::endl;
        detectMergeTile[0].first->show(std::cout);
        detectMergeTile[0].second->show(std::cout);

        legaliser->combineVerticalMergeableBlanks(detectMergeTile[0].first, detectMergeTile[0].second);
        detectMergeTile.clear();
        legaliser->detectCombinableBlanks(detectMergeTile);
    }


    legaliser->outputTileFloorplan("outputs/" + casename + "_init.txt", casename);

    std::cout << std::endl << std::endl;
    DFSL::DFSLegalizer dfsl;

    LFLegaliser legalizedFloorplan(*(legaliser));
    dfsl.setOutputLevel(3);
    dfsl.initDFSLegalizer(&(legalizedFloorplan));

    double storeOBAreaWeight;
    double storeOBUtilWeight;
    double storeOBAspWeight;
    double storeOBUtilPosRein;
    double storeBWUtilWeight;
    double storeBWAspWeight;
    double storeBWUtilPosRein;
    double storeBBAreaWeight;
    double storeBBFromUtilWeight;
    double storeBBFromUtilPosRein;    
    double storeBBToUtilWeight;
    double storeBBToUtilPosRein;
    double storeBBAspWeight;
    double storeBBFlatCost;
    
    dfsl.config.resetDefault();
    
    if (legalStrategy == 0){
        std::cout << "legalStrategy = 0, using default configs\n";
        // default configs
    }
    else if (legalStrategy == 1){
        // prioritize area 
        std::cout << "legalStrategy = 1, prioritizing area\n";
        dfsl.config.OBAreaWeight = storeOBAreaWeight = 1400.0;
        dfsl.config.OBUtilWeight = storeOBUtilWeight = 750.0;
        dfsl.config.OBAspWeight = storeOBAspWeight = 80.0;

        dfsl.config.BWUtilWeight = storeBWUtilWeight = 750.0;
        dfsl.config.BWAspWeight = storeBWAspWeight = 80.0;
    }
    else if (legalStrategy == 2){
        // prioritize util
        std::cout << "legalStrategy = 2, prioritizing utilization\n";
        dfsl.config.OBAreaWeight = storeOBAreaWeight  = 700.0;
        dfsl.config.OBUtilWeight = storeOBUtilWeight  = 900.0;
        dfsl.config.OBAspWeight = storeOBAspWeight = 40.0;
        dfsl.config.BWUtilWeight = storeBWUtilWeight = 2000.0;
        dfsl.config.BWAspWeight = storeBWAspWeight = 40.0;
        dfsl.config.BWUtilPosRein = storeBWUtilPosRein = -1000.0;
    }
    else if (legalStrategy == 3){
        // prioritize aspect ratio
        std::cout << "legalStrategy = 3, prioritizing aspect ratio\n";
        dfsl.config.OBAreaWeight = storeOBAreaWeight  = 750.0;
        dfsl.config.OBUtilWeight = storeOBUtilWeight  = 1100.0;
        dfsl.config.OBAspWeight = storeOBAspWeight = 300.0;
        dfsl.config.BWUtilWeight = storeBWUtilWeight = 1000.0;
        dfsl.config.BWAspWeight = storeBWAspWeight = 800.0;
    }
    else if (legalStrategy == 4){
        // favor block -> block flow more
        std::cout << "legalStrategy = 4, prioritizing block -> block flow\n";
        dfsl.config.OBAreaWeight = storeOBAreaWeight  = 1000.0;
        dfsl.config.OBUtilWeight = storeOBUtilWeight  = 1250.0;
        dfsl.config.OBAspWeight = storeOBAspWeight = 400.0;

        dfsl.config.BWUtilWeight = storeBWUtilWeight = 1800.0;
        dfsl.config.BWAspWeight = storeBWAspWeight = 700.0;

        dfsl.config.BBAreaWeight = storeBBAreaWeight = 100.0;
        dfsl.config.BBFromUtilWeight = storeBBFromUtilWeight = 450.0;
        dfsl.config.BBToUtilWeight = storeBBToUtilWeight = 500.0;
        dfsl.config.BBAspWeight = storeBBAspWeight = 10.0;
        dfsl.config.BBFlatCost = storeBBFlatCost = -30;
    }
    std::cout << "Legalization mode: " << legalMode << std::endl;

    DFSL::RESULT legalResult = dfsl.legalize(legalMode);

    std::cout << "DSFL DONE\n";
    if (legalResult == DFSL::RESULT::SUCCESS){
        std::cout << "Checking legality..." << std::endl;
        for (Tessera* tess: legalizedFloorplan.softTesserae){
            if (!tess->isLegal()){
                std::cout << tess->getName() << " is not legal!" << std::endl;
            }
        }
    } 
    else if (legalResult == DFSL::RESULT::CONSTRAINT_FAIL ) {
        std::cout << "Constraints FAIL, restarting process...\n" << std::endl;
    } 
    else {
        std::cout << "Impossible to solve, restarting process...\n" << std::endl;
    }

    legalizedFloorplan.outputTileFloorplan("outputs/" + casename + "_legal_" + std::to_string(legalStrategy) + "_" + std::to_string(legalMode) + ".txt", casename);
    
    double finalScore = legalizedFloorplan.calculateHPWL();
    printf("Final Score = %12.6f\n", finalScore);
    if (finalScore < bestHpwl){
        bestHpwl = finalScore;
        std::cout << "Best Hpwl found\n";
        legalizedFloorplan.outputTileFloorplan("outputs/" + casename + "_legal.txt", casename);
        legalizedFloorplan.outputFullFloorplan("outputs/" + casename + "_legal_fp.txt", casename);
    }
    
    printf("best hpwl = %12.6f\n", bestHpwl);
}